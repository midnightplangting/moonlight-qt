#include "computermanager.h"
#include "boxartmanager.h"
#include "nvhttp.h"
#include "nvpairingmanager.h"
#include "ApiService.h"
#include "Logger.h"

#include <Limelight.h>
#include <QtEndian>

#include <QThread>
#include <QThreadPool>
#include <QCoreApplication>

#include <random>
#include <QJsonDocument>      // 解析 JSON
#include <QJsonArray>
#include <QJsonObject>
#include "UserSession.h"      // （若之前已包含可忽略）
#include <atomic>             // 线程一次性标记
#include <QDateTime>
#include <QUuid>

#define SER_HOSTS "hosts"
#define SER_HOSTS_BACKUP "hostsbackup"

class PcMonitorThread : public QThread
{
    Q_OBJECT

#define TRIES_BEFORE_OFFLINING 2
#define POLLS_PER_APPLIST_FETCH 10

public:
    // PcMonitorThread(NvComputer* computer)
    //     : m_Computer(computer)
    // {
    //     setObjectName("Polling thread for " + computer->name);
    // }
    PcMonitorThread(NvComputer* computer, ComputerManager* mgr)
        : m_Computer(computer), m_Manager(mgr) {
        setObjectName("Polling thread for " + computer->name);
    }

private:
    bool tryPollComputer(NvAddress address, bool& changed)
    {
        NvHTTP http(address, 0, m_Computer->serverCert);

        QString serverInfo;
        try {
            serverInfo = http.getServerInfo(NvHTTP::NvLogLevel::NVLL_NONE, true);
        } catch (...) {
            return false;
        }

        NvComputer newState(http, serverInfo);

        // Ensure the machine that responded is the one we intended to contact
        if (m_Computer->uuid != newState.uuid) {
            qInfo() << "Found unexpected PC" << newState.name << "looking for" << m_Computer->name;
            return false;
        }

        changed = m_Computer->update(newState);
        return true;
    }

    bool updateAppList(bool& changed)
    {
        NvHTTP http(m_Computer);

        QVector<NvApp> appList;

        try {
            appList = http.getAppList();
            if (appList.isEmpty()) {
                return false;
            }
        } catch (...) {
            return false;
        }

        QWriteLocker lock(&m_Computer->lock);
        changed = m_Computer->updateAppList(appList);
        return true;
    }

    void run() override
    {
        qInfo() << "[PcMonitorThread] run() started for"
                << m_Computer->name << "@" ;

        // Always fetch the applist the first time
        int pollsSinceLastAppListFetch = POLLS_PER_APPLIST_FETCH;

        while (!isInterruptionRequested()) {
            bool stateChanged = false;
            bool online = false;
            bool wasOnline = m_Computer->state == NvComputer::CS_ONLINE;
            for (int i = 0; i < (wasOnline ? TRIES_BEFORE_OFFLINING : 1) && !online; i++) {
                for (auto& address : m_Computer->uniqueAddresses()) {
                    if (isInterruptionRequested()) {
                        return;
                    }

                    if (tryPollComputer(address, stateChanged)) {
                        if (!wasOnline) {
                            qInfo() << m_Computer->name << "is now online at" << m_Computer->activeAddress.toString();
                        }
                        online = true;
                        break;
                    }
                }
            }

            // Check if we failed after all retry attempts
            // Note: we don't need to acquire the read lock here,
            // because we're on the writing thread.
            if (!online && m_Computer->state != NvComputer::CS_OFFLINE) {
                qInfo() << m_Computer->name << "is now offline";
                m_Computer->state = NvComputer::CS_OFFLINE;
                stateChanged = true;
            }

            // Grab the applist if it's empty or it's been long enough that we need to refresh
            pollsSinceLastAppListFetch++;
            if (m_Computer->state == NvComputer::CS_ONLINE &&
                    m_Computer->pairState == NvComputer::PS_PAIRED &&
                    (m_Computer->appList.isEmpty() || pollsSinceLastAppListFetch >= POLLS_PER_APPLIST_FETCH)) {
                // Notify prior to the app list poll since it may take a while, and we don't
                // want to delay onlining of a machine, especially if we already have a cached list.
                if (stateChanged) {
                    emit computerStateChanged(m_Computer);
                    stateChanged = false;
                }

                if (updateAppList(stateChanged)) {
                    pollsSinceLastAppListFetch = 0;
                }
            }

            if (stateChanged) {
                // Tell anyone listening that we've changed state
                emit computerStateChanged(m_Computer);
            }

            // Sync order info periodically
            m_Manager->checkOrderStatus(m_Computer);

            // Wait a bit to poll again, but do it in 100 ms chunks
            // so we can be interrupted reasonably quickly.
            // FIXME: QWaitCondition would be better.
            for (int i = 0; i < 30 && !isInterruptionRequested(); i++) {
                QThread::msleep(100);
            }
        }
    }

signals:
   void computerStateChanged(NvComputer* computer);

private:
    NvComputer* m_Computer;
    ComputerManager* m_Manager;
};

ComputerManager::ComputerManager(StreamingPreferences* prefs)
    : m_Prefs(prefs),
      m_PollingRef(0),
      m_MdnsBrowser(nullptr),
      m_CompatFetcher(nullptr),
      m_NeedsDelayedFlush(false)
{
    QSettings settings;

    // If there's a hosts backup copy, we must have failed to commit
    // a previous update before exiting. Restore the backup now.
    int hosts = settings.beginReadArray(SER_HOSTS_BACKUP);
    if (hosts == 0) {
        // If there's no host backup, read from the primary location.
        settings.endArray();
        hosts = settings.beginReadArray(SER_HOSTS);
    }

    // Inflate our hosts from QSettings
    for (int i = 0; i < hosts; i++) {
        settings.setArrayIndex(i);
        NvComputer* computer = new NvComputer(settings);
        m_KnownHosts[computer->uuid] = computer;
        m_LastSerializedHosts[computer->uuid] = *computer;
    }
    settings.endArray();

    // Fetch latest compatibility data asynchronously
    m_CompatFetcher.start();

    // Start the delayed flush thread to handle saveHosts() calls
    m_DelayedFlushThread = new DelayedFlushThread(this);
    m_DelayedFlushThread->start();

    // 定时同步订单状态，确保串流过程中也能获取最新状态
    m_OrderTimer.setInterval(300000);
    connect(&m_OrderTimer, &QTimer::timeout, this, &ComputerManager::syncOrderDevices);

    // To quit in a timely manner, we must block additional requests
    // after we receive the aboutToQuit() signal. This is necessary
    // because NvHTTP uses aboutToQuit() to abort requests in progress
    // while quitting, however this is a one time signal - additional
    // requests would not be aborted and block termination.
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &ComputerManager::handleAboutToQuit);

}

ComputerManager::~ComputerManager()
{
    // Stop the delayed flush thread before acquiring the lock in write mode
    // to avoid deadlocking with a flush that needs the lock in read mode.
    {
        // Wake the delayed flush thread
        m_DelayedFlushThread->requestInterruption();
        m_DelayedFlushCondition.wakeOne();

        // Wait for it to terminate (and finish any pending flush)
        m_DelayedFlushThread->wait();
        delete m_DelayedFlushThread;

        // Delayed flushes should have completed by now
        Q_ASSERT(!m_NeedsDelayedFlush);
    }

    QWriteLocker lock(&m_Lock);

    // Delete machines that haven't been resolved yet
    while (!m_PendingResolution.isEmpty()) {
        MdnsPendingComputer* computer = m_PendingResolution.first();
        delete computer;
        m_PendingResolution.removeFirst();
    }

    // Delete the browser to stop discovery
    delete m_MdnsBrowser;
    m_MdnsBrowser = nullptr;

    // Interrupt polling
    for (ComputerPollingEntry* entry : m_PollEntries) {
        entry->interrupt();
    }

    // Delete all polling entries (and associated threads)
    for (ComputerPollingEntry* entry : m_PollEntries) {
        delete entry;
    }

    // Destroy all NvComputer objects now that polling is halted
    for (NvComputer* computer : m_KnownHosts) {
        delete computer;
    }
}

void DelayedFlushThread::run() {
    for (;;) {
        // Wait for a delayed flush request or an interruption
        {
            QMutexLocker locker(&m_ComputerManager->m_DelayedFlushMutex);

            while (!QThread::currentThread()->isInterruptionRequested() && !m_ComputerManager->m_NeedsDelayedFlush) {
                m_ComputerManager->m_DelayedFlushCondition.wait(&m_ComputerManager->m_DelayedFlushMutex);
            }

            // Bail without flushing if we woke up for an interruption alone.
            // If we have both an interruption and a flush request, do the flush.
            if (!m_ComputerManager->m_NeedsDelayedFlush) {
                Q_ASSERT(QThread::currentThread()->isInterruptionRequested());
                break;
            }

            // Reset the delayed flush flag to ensure any racing saveHosts() call will set it again
            m_ComputerManager->m_NeedsDelayedFlush = false;

            // Update the last serialized hosts map under the delayed flush mutex
            m_ComputerManager->m_LastSerializedHosts.clear();
            for (const NvComputer* computer : m_ComputerManager->m_KnownHosts) {
                // Copy the current state of the NvComputer to allow us to check later if we need
                // to serialize it again when attribute updates occur.
                QReadLocker computerLock(&computer->lock);
                m_ComputerManager->m_LastSerializedHosts[computer->uuid] = *computer;
            }
        }

        // Perform the flush
        {
            QSettings settings;

            // First, write to the backup location
            settings.beginWriteArray(SER_HOSTS_BACKUP);
            {
                QReadLocker lock(&m_ComputerManager->m_Lock);
                int i = 0;
                for (const NvComputer* computer : m_ComputerManager->m_KnownHosts) {
                    settings.setArrayIndex(i++);
                    computer->serialize(settings, false);
                }
            }
            settings.endArray();

            // Next, write to the primary location
            settings.remove(SER_HOSTS);
            settings.beginWriteArray(SER_HOSTS);
            {
                QReadLocker lock(&m_ComputerManager->m_Lock);
                int i = 0;
                for (const NvComputer* computer : m_ComputerManager->m_KnownHosts) {
                    settings.setArrayIndex(i++);
                    computer->serialize(settings, true);
                }
            }
            settings.endArray();

            // Finally, delete the backup copy
            settings.remove(SER_HOSTS_BACKUP);
        }
    }
}

void ComputerManager::saveHosts()
{
    Q_ASSERT(m_DelayedFlushThread != nullptr && m_DelayedFlushThread->isRunning());

    // Punt to a worker thread because QSettings on macOS can take ages (> 500 ms)
    // to persist our host list to disk (especially when a host has a bunch of apps).
    QMutexLocker locker(&m_DelayedFlushMutex);
    m_NeedsDelayedFlush = true;
    m_DelayedFlushCondition.wakeOne();
}

QHostAddress ComputerManager::getBestGlobalAddressV6(QVector<QHostAddress> &addresses)
{
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv6Protocol) {
            if (address.isInSubnet(QHostAddress("fe80::"), 10)) {
                // Link-local
                continue;
            }

            if (address.isInSubnet(QHostAddress("fec0::"), 10)) {
                qInfo() << "Ignoring site-local address:" << address;
                continue;
            }

            if (address.isInSubnet(QHostAddress("fc00::"), 7)) {
                qInfo() << "Ignoring ULA:" << address;
                continue;
            }

            if (address.isInSubnet(QHostAddress("2002::"), 16)) {
                qInfo() << "Ignoring 6to4 address:" << address;
                continue;
            }

            if (address.isInSubnet(QHostAddress("2001::"), 32)) {
                qInfo() << "Ignoring Teredo address:" << address;
                continue;
            }

            return address;
        }
    }

    return QHostAddress();
}

void ComputerManager::startPolling()
{
    QWriteLocker lock(&m_Lock);

    if (++m_PollingRef > 1) {
        return;
    }

    if (m_Prefs->enableMdns) {
        // Start an MDNS query for GameStream hosts
        m_MdnsServer.reset(new QMdnsEngine::Server());
        m_MdnsBrowser = new QMdnsEngine::Browser(m_MdnsServer.data(), "_nvstream._tcp.local.");
        connect(m_MdnsBrowser, &QMdnsEngine::Browser::serviceAdded,
                this, [this](const QMdnsEngine::Service& service) {
            qInfo() << "Discovered mDNS host:" << service.hostname();

            MdnsPendingComputer* pendingComputer = new MdnsPendingComputer(m_MdnsServer, service);
            connect(pendingComputer, &MdnsPendingComputer::resolvedHost,
                    this, &ComputerManager::handleMdnsServiceResolved);
            m_PendingResolution.append(pendingComputer);
        });
    }
    else {
        qWarning() << "mDNS is disabled by user preference";
    }

    // Start polling threads for each known host
    QMapIterator<QString, NvComputer*> i(m_KnownHosts);
    while (i.hasNext()) {
        i.next();
        startPollingComputer(i.value());
    }

    if (!m_OrderTimer.isActive())
        m_OrderTimer.start();
}

// Must hold m_Lock for write
void ComputerManager::startPollingComputer(NvComputer* computer)
{
    if (m_PollingRef == 0) {
        return;
    }

    ComputerPollingEntry* pollingEntry;

    if (!m_PollEntries.contains(computer->uuid)) {
        pollingEntry = m_PollEntries[computer->uuid] = new ComputerPollingEntry();

    }
    else {
        pollingEntry = m_PollEntries[computer->uuid];
    }

    if (!pollingEntry->isActive()) {
        PcMonitorThread* thread = new PcMonitorThread(computer, this);
        connect(thread, &PcMonitorThread::computerStateChanged,
                this, &ComputerManager::handleComputerStateChanged);
        pollingEntry->setActiveThread(thread);
        thread->start();
    }
}

void ComputerManager::handleMdnsServiceResolved(MdnsPendingComputer* computer,
                                                QVector<QHostAddress>& addresses)
{
    QHostAddress v6Global = getBestGlobalAddressV6(addresses);
    bool added = false;

    // Add the host using the IPv4 address
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            // NB: We don't just call addNewHost() here with v6Global because the IPv6
            // address may not be reachable (if the user hasn't installed the IPv6 helper yet
            // or if this host lacks outbound IPv6 capability). We want to add IPv6 even if
            // it's not currently reachable.
            addNewHost(NvAddress(address, computer->port()), true, NvAddress(v6Global, computer->port()));
            added = true;
            break;
        }
    }

    if (!added) {
        // If we get here, there wasn't an IPv4 address so we'll do it v6-only
        for (const QHostAddress& address : addresses) {
            if (address.protocol() == QAbstractSocket::IPv6Protocol) {
                // Use a link-local or site-local address for the "local address"
                if (address.isInSubnet(QHostAddress("fe80::"), 10) ||
                        address.isInSubnet(QHostAddress("fec0::"), 10) ||
                        address.isInSubnet(QHostAddress("fc00::"), 7)) {
                    addNewHost(NvAddress(address, computer->port()), true, NvAddress(v6Global, computer->port()));
                    break;
                }
            }
        }
    }

    m_PendingResolution.removeOne(computer);
    computer->deleteLater();
}

void ComputerManager::saveHost(NvComputer *computer)
{
    // If no serializable properties changed, don't bother saving hosts
    QMutexLocker lock(&m_DelayedFlushMutex);
    QReadLocker computerLock(&computer->lock);
    if (!m_LastSerializedHosts.value(computer->uuid).isEqualSerialized(*computer)) {
        // Queue a request for a delayed flush to QSettings outside of the lock
        computerLock.unlock();
        lock.unlock();
        saveHosts();
    }
}

void ComputerManager::handleComputerStateChanged(NvComputer* computer)
{
    // Apply order information if available
    QString key = deviceKey(computer);
    {
        QReadLocker rlock(&m_OrderLock);
        if (m_DeviceInfo.contains(key)) {
            const DeviceInfo info = m_DeviceInfo.value(key);
            QWriteLocker wlock(&computer->lock);
            if (!computer->orderStartedAt.isValid())
                computer->orderStartedAt = info.startedAt;
            if (!computer->orderEndedAt.isValid())
                computer->orderEndedAt = info.endedAt;
            if (computer->orderBitrate == 0.0)
                computer->orderBitrate = info.bitrate;
            if (computer->orderId == 0)
                computer->orderId = info.orderId;
            computer->orderStatus = info.status;
            computer->orderBillingType = info.billingType;
        }
    }
    emit computerStateChanged(computer);

    if (computer->pendingQuit && computer->currentGameId == 0) {
        computer->pendingQuit = false;
        emit quitAppCompleted(QVariant());
    }

    // Save updates to this host
    saveHost(computer);
}

void ComputerManager::checkOrderStatus(NvComputer* computer)
{
    QString key = deviceKey(computer);

    int status = 0;
    qint64 orderId = 0;
    {
        QReadLocker rlock(&m_OrderLock);
        if (m_DeviceInfo.contains(key))
        {
            const DeviceInfo info = m_DeviceInfo.value(key);
            status = info.status;
            orderId = info.orderId;
        }
    }

    {
        QWriteLocker wlock(&computer->lock);
        computer->orderStatus = status;
        if (computer->orderId == 0)
            computer->orderId = orderId;
    }

    if (status == 3 || status == 4) {
        quitRunningApp(computer);
        emit orderStatusException(status);
    }

    emit computerStateChanged(computer);
}

QVector<NvComputer*> ComputerManager::getComputers()
{
    QReadLocker lock(&m_Lock);

    // Return a sorted host list
    auto hosts = QVector<NvComputer*>::fromList(m_KnownHosts.values());
    std::stable_sort(hosts.begin(), hosts.end(), [](const NvComputer* host1, const NvComputer* host2) {
        return host1->name.toLower() < host2->name.toLower();
    });
    return hosts;
}

class DeferredHostDeletionTask : public QRunnable
{
public:
    DeferredHostDeletionTask(ComputerManager* cm, NvComputer* computer)
        : m_Computer(computer),
          m_ComputerManager(cm) {}

    void run()
    {
        ComputerPollingEntry* pollingEntry;

        // Only do the minimum amount of work while holding the writer lock.
        // We must release it before calling saveHosts().
        {
            QWriteLocker lock(&m_ComputerManager->m_Lock);

            pollingEntry = m_ComputerManager->m_PollEntries.take(m_Computer->uuid);

            m_ComputerManager->m_KnownHosts.remove(m_Computer->uuid);
        }

        // 发出信号通知 model 有主机被删
        emit m_ComputerManager->hostRemoved(m_Computer);

        // Persist the new host list with this computer deleted
        m_ComputerManager->saveHosts();

        // Delete the polling entry first. This will stop all polling threads too.
        delete pollingEntry;

        // Delete cached box art
        BoxArtManager::deleteBoxArt(m_Computer);

        // Finally, delete the computer itself. This must be done
        // last because the polling thread might be using it.
        delete m_Computer;
    }

private:
    NvComputer* m_Computer;
    ComputerManager* m_ComputerManager;
};

void ComputerManager::deleteHost(NvComputer* computer)
{
    // Punt to a worker thread to avoid stalling the
    // UI while waiting for the polling thread to die
    QThreadPool::globalInstance()->start(new DeferredHostDeletionTask(this, computer));
}

void ComputerManager::renameHost(NvComputer* computer, QString name)
{
    {
        QWriteLocker lock(&computer->lock);

        computer->name = name;
        computer->hasCustomName = true;
    }

    // Notify the UI of the state change
    handleComputerStateChanged(computer);
}

bool ComputerManager::isOrderDevice(NvComputer* computer)
{
    QString key = deviceKey(computer);

    {
        QReadLocker rlock(&m_OrderLock);
        if (m_DeviceInfo.contains(key)) {
            return m_DeviceInfo.value(key).billingType != 0;
        }
    }

    // 自动扫描的主机会走到这里，为其创建默认记录，计费类型为 0
    registerDeviceInfo(computer);
    return false;
}

void ComputerManager::clearOrderDevices()
{
    QList<NvComputer*> toDelete;
    {
        QReadLocker lock(&m_Lock);
        QReadLocker orderLock(&m_OrderLock);
        for (NvComputer* pc : m_KnownHosts) {
            QString key = deviceKey(pc);
            if (m_DeviceInfo.contains(key) && m_DeviceInfo.value(key).billingType != 0) {
                toDelete.append(pc);
            }
        }
    }

    for (NvComputer* pc : toDelete) {
        handleComputerStateChanged(pc);
        deleteHost(pc);
    }

    {
        QWriteLocker orderLock(&m_OrderLock);
        m_DeviceInfo.clear();
        m_OrderDeviceKeys.clear();
    }
}

void ComputerManager::dumpStoredComputers()
{
    QReadLocker lock(&m_Lock);
    QReadLocker orderLock(&m_OrderLock);

    LOG_INFO(QStringLiteral("[dumpStoredComputers] count=%1").arg(m_KnownHosts.size()));
    for (NvComputer* pc : m_KnownHosts) {
        QString key = deviceKey(pc);
        const DeviceInfo* info = m_DeviceInfo.contains(key) ? &m_DeviceInfo[key] : nullptr;
        Logger::logComputer(pc, info);
    }
}

void ComputerManager::clientSideAttributeUpdated(NvComputer* computer)
{
    // Notify the UI of the state change
    handleComputerStateChanged(computer);
}

void ComputerManager::handleAboutToQuit()
{
    QReadLocker lock(&m_Lock);

    // Interrupt polling threads immediately, so they
    // avoid making additional requests while quitting
    for (ComputerPollingEntry* entry : m_PollEntries) {
        entry->interrupt();
    }
}

class PendingPairingTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    PendingPairingTask(ComputerManager* computerManager, NvComputer* computer, QString pin)
        : m_ComputerManager(computerManager),
          m_Computer(computer),
          m_Pin(pin)
    {
        connect(this, &PendingPairingTask::pairingCompleted,
                computerManager, &ComputerManager::pairingCompleted);
    }

signals:
    void pairingCompleted(NvComputer* computer, QString error);

private:
    void run()
    {
        NvPairingManager pairingManager(m_Computer);

        try {
           NvPairingManager::PairState result = pairingManager.pair(m_Computer->appVersion, m_Pin, m_Computer->serverCert);
           switch (result)
           {
           case NvPairingManager::PairState::PIN_WRONG:
               emit pairingCompleted(m_Computer, tr("The PIN from the PC didn't match. Please try again."));
               break;
           case NvPairingManager::PairState::FAILED:
               if (m_Computer->currentGameId != 0) {
                   emit pairingCompleted(m_Computer, tr("You cannot pair while a previous session is still running on the host PC. Quit any running games or reboot the host PC, then try pairing again."));
               }
               else {
                   emit pairingCompleted(m_Computer, tr("Pairing failed. Please try again."));
               }
               break;
           case NvPairingManager::PairState::ALREADY_IN_PROGRESS:
               emit pairingCompleted(m_Computer, tr("Another pairing attempt is already in progress."));
               break;
           case NvPairingManager::PairState::PAIRED:
               // Persist the newly pinned server certificate for this host
               m_ComputerManager->saveHost(m_Computer);

               emit pairingCompleted(m_Computer, nullptr);
               break;
           }
        } catch (const GfeHttpResponseException& e) {
            emit pairingCompleted(m_Computer, tr("GeForce Experience returned error: %1").arg(e.toQString()));
        } catch (const QtNetworkReplyException& e) {
            emit pairingCompleted(m_Computer, e.toQString());
        }
    }

    ComputerManager* m_ComputerManager;
    NvComputer* m_Computer;
    QString m_Pin;
};

void ComputerManager::pairHost(NvComputer* computer, QString pin)
{
    // Auto send PIN for ordered/cloud devices
    QString addr = !computer->manualAddress.isNull() ? computer->manualAddress.address()
                                                    : computer->localAddress.address();
    quint16 port = !computer->manualAddress.isNull() ? computer->manualAddress.port()
                                                     : computer->localAddress.port();
    QString key = deviceKey(addr, port);

    bool orderDevice = false;
    qint64 orderId = 0;
    {
        QReadLocker rlock(&m_OrderLock);
        orderDevice = m_DeviceInfo.contains(key) && m_DeviceInfo.value(key).billingType != 0;
        if (orderDevice)
            orderId = m_DeviceInfo.value(key).orderId;
    }

    if (orderDevice || (computer->manualAddress.isNull() && !computer->remoteAddress.isNull())) {
        ApiService::PinRequest req;
        if (orderDevice)
            req.orderId = orderId;
        req.localIP = computer->localAddress.address();
        req.port = QString::number(port);
        req.name = computer->name;
        req.pinStr = pin;

        ApiService::sendPin(req,
                            [](bool) {},
                            [](QString err) {
                                qWarning() << "Failed to send PIN:" << err;
                            });
    }

    // Punt to a worker thread to avoid stalling the
    // UI while waiting for pairing to complete
    PendingPairingTask* pairing = new PendingPairingTask(this, computer, pin);
    QThreadPool::globalInstance()->start(pairing);
}

class PendingQuitTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    PendingQuitTask(ComputerManager* computerManager, NvComputer* computer)
        : m_Computer(computer)
    {
        connect(this, &PendingQuitTask::quitAppFailed,
                computerManager, &ComputerManager::quitAppCompleted);
    }

signals:
    void quitAppFailed(QString error);

private:
    void run()
    {
        NvHTTP http(m_Computer);

        try {
            if (m_Computer->currentGameId != 0) {
                http.quitApp();
            }
        } catch (const GfeHttpResponseException& e) {
            {
                QWriteLocker lock(&m_Computer->lock);
                m_Computer->pendingQuit = false;
            }
            if (e.getStatusCode() == 599) {
                // 599 is a special code we make a custom message for
                emit quitAppFailed(tr("The running game wasn't started by this PC. "
                                      "You must quit the game on the host PC manually or use the device that originally started the game."));
            }
            else {
                emit quitAppFailed(e.toQString());
            }
        } catch (const QtNetworkReplyException& e) {
            {
                QWriteLocker lock(&m_Computer->lock);
                m_Computer->pendingQuit = false;
            }
            emit quitAppFailed(e.toQString());
        }
    }

    NvComputer* m_Computer;
};

void ComputerManager::quitRunningApp(NvComputer* computer)
{
    QWriteLocker lock(&computer->lock);
    computer->pendingQuit = true;

    PendingQuitTask* quit = new PendingQuitTask(this, computer);
    QThreadPool::globalInstance()->start(quit);
}

void ComputerManager::stopPollingAsync()
{
    QWriteLocker lock(&m_Lock);

    Q_ASSERT(m_PollingRef > 0);
    if (--m_PollingRef > 0) {
        return;
    }

    if (m_OrderTimer.isActive())
        m_OrderTimer.stop();

    // Delete machines that haven't been resolved yet
    while (!m_PendingResolution.isEmpty()) {
        MdnsPendingComputer* computer = m_PendingResolution.first();
        computer->deleteLater();
        m_PendingResolution.removeFirst();
    }

    // Delete the browser and server to stop discovery and refresh polling
    delete m_MdnsBrowser;
    m_MdnsBrowser = nullptr;
    m_MdnsServer.reset();

    // Interrupt all threads, but don't wait for them to terminate
    for (ComputerPollingEntry* entry : m_PollEntries) {
        entry->interrupt();
    }
}

void ComputerManager::addNewHostManually(QString address)
{
    QUrl url = QUrl::fromUserInput("moonlight://" + address);
    if (url.isValid() && !url.host().isEmpty() && url.scheme() == "moonlight") {
        // If there wasn't a port specified, use the default
        addNewHost(NvAddress(url.host(), url.port(DEFAULT_HTTP_PORT)), false);
    }
    else {
        emit computerAddCompleted(false, false);
    }
}

class PendingAddTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    PendingAddTask(ComputerManager* computerManager, NvAddress address, NvAddress mdnsIpv6Address, bool mdns)
        : m_ComputerManager(computerManager),
          m_Address(address),
          m_MdnsIpv6Address(mdnsIpv6Address),
          m_Mdns(mdns),
          m_AboutToQuit(false)
    {
        connect(this, &PendingAddTask::computerAddCompleted,
                computerManager, &ComputerManager::computerAddCompleted);
        connect(this, &PendingAddTask::computerStateChanged,
                computerManager, &ComputerManager::handleComputerStateChanged);
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                this, &PendingAddTask::handleAboutToQuit);
    }

signals:
    void computerAddCompleted(QVariant success, QVariant detectedPortBlocking);

    void computerStateChanged(NvComputer* computer);

private:
    void handleAboutToQuit()
    {
        m_AboutToQuit = true;
    }

    QString fetchServerInfo(NvHTTP& http)
    {
        QString serverInfo;

        // Do nothing if we're quitting
        if (m_AboutToQuit) {
            return QString();
        }

        try {
            // There's a race condition between GameStream servers reporting presence over
            // mDNS and the HTTPS server being ready to respond to our queries. To work
            // around this issue, we will issue the request again after a few seconds if
            // we see a ServiceUnavailableError error.
            try {
                serverInfo = http.getServerInfo(NvHTTP::NVLL_VERBOSE);
            } catch (const QtNetworkReplyException& e) {
                if (e.getError() == QNetworkReply::ServiceUnavailableError) {
                    qWarning() << "Retrying request in 5 seconds after ServiceUnavailableError";
                    QThread::sleep(5);
                    serverInfo = http.getServerInfo(NvHTTP::NVLL_VERBOSE);
                    qInfo() << "Retry successful";
                }
                else {
                    // Rethrow other errors
                    throw e;
                }
            }
            return serverInfo;
        } catch (...) {
            if (!m_Mdns) {
                unsigned int portTestResult;

                if (m_ComputerManager->m_Prefs->detectNetworkBlocking) {
                    // We failed to connect to the specified PC. Let's test to make sure this network
                    // isn't blocking Moonlight, so we can tell the user about it.
                    portTestResult = LiTestClientConnectivity("qt.conntest.moonlight-stream.org", 443,
                                                              ML_PORT_FLAG_TCP_47984 | ML_PORT_FLAG_TCP_47989);
                }
                else {
                    portTestResult = 0;
                }

                emit computerAddCompleted(false, portTestResult != 0 && portTestResult != ML_TEST_RESULT_INCONCLUSIVE);
            }
            return QString();
        }
    }

    void run()
    {
        NvHTTP http(m_Address, 0, QSslCertificate());

        qInfo() << "Processing new PC at" << m_Address.toString() << "from" << (m_Mdns ? "mDNS" : "user") << "with IPv6 address" << m_MdnsIpv6Address.toString();

        // Perform initial serverinfo fetch over HTTP since we don't know which cert to use
        QString serverInfo = fetchServerInfo(http);
        if (serverInfo.isEmpty() && !m_MdnsIpv6Address.isNull()) {
            // Retry using the global IPv6 address if the IPv4 or link-local IPv6 address fails
            http.setAddress(m_MdnsIpv6Address);
            serverInfo = fetchServerInfo(http);
        }
        if (serverInfo.isEmpty()) {
            return;
        }

        // Create initial newComputer using HTTP serverinfo with no pinned cert
        NvComputer* newComputer = new NvComputer(http, serverInfo);

        // Check if we have a record of this host UUID to pull the pinned cert
        NvComputer* existingComputer;
        {
            QReadLocker lock(&m_ComputerManager->m_Lock);
            existingComputer = m_ComputerManager->m_KnownHosts.value(newComputer->uuid);
            if (existingComputer != nullptr) {
                http.setServerCert(existingComputer->serverCert);
            }
        }

        // Fetch serverinfo again over HTTPS with the pinned cert
        if (existingComputer != nullptr) {
            Q_ASSERT(http.httpsPort() != 0);
            serverInfo = fetchServerInfo(http);
            if (serverInfo.isEmpty()) {
                return;
            }

            // Update the polled computer with the HTTPS information
            NvComputer httpsComputer(http, serverInfo);
            newComputer->update(httpsComputer);
        }

        // Update addresses depending on the context
        if (m_Mdns) {
            // Only update local address if we actually reached the PC via this address.
            // If we reached it via the IPv6 address after the local address failed,
            // don't store the non-working local address.
            if (http.address() == m_Address) {
                newComputer->localAddress = m_Address;
            }

            // Get the WAN IP address using STUN if we're on mDNS over IPv4
            if (QHostAddress(newComputer->localAddress.address()).protocol() == QAbstractSocket::IPv4Protocol) {
                quint32 addr;
                int err = LiFindExternalAddressIP4("stun.moonlight-stream.org", 3478, &addr);
                if (err == 0) {
                    newComputer->setRemoteAddress(QHostAddress(qFromBigEndian(addr)));
                }
                else {
                    qWarning() << "STUN failed to get WAN address:" << err;
                }
            }

            if (!m_MdnsIpv6Address.isNull()) {
                Q_ASSERT(QHostAddress(m_MdnsIpv6Address.address()).protocol() == QAbstractSocket::IPv6Protocol);
                newComputer->ipv6Address = m_MdnsIpv6Address;
            }
        }
        else {
            newComputer->manualAddress = m_Address;
        }

        QHostAddress hostAddress(m_Address.address());
        bool addressIsSiteLocalV4 =
                hostAddress.isInSubnet(QHostAddress("10.0.0.0"), 8) ||
                hostAddress.isInSubnet(QHostAddress("172.16.0.0"), 12) ||
                hostAddress.isInSubnet(QHostAddress("192.168.0.0"), 16);

        {
            // Check if this PC already exists using opportunistic read lock
            m_ComputerManager->m_Lock.lockForRead();
            NvComputer* existingComputer = m_ComputerManager->m_KnownHosts.value(newComputer->uuid);

            // If it doesn't already exist, convert to a write lock in preparation for updating.
            //
            // NB: ComputerManager's lock protects the host map itself, not the elements inside.
            // Those are protected by their individual locks. Since we only mutate the map itself
            // when the PC doesn't exist, we need the lock in write-mode for that case only.
            if (existingComputer == nullptr) {
                m_ComputerManager->m_Lock.unlock();
                m_ComputerManager->m_Lock.lockForWrite();

                // Since we had to unlock to lock for write, someone could have raced and added
                // this PC before us. We have to check again whether it already exists.
                existingComputer = m_ComputerManager->m_KnownHosts.value(newComputer->uuid);
            }

            if (existingComputer != nullptr) {
                // Fold it into the existing PC
                bool changed = existingComputer->update(*newComputer);
                delete newComputer;

                // Drop the lock before notifying
                m_ComputerManager->m_Lock.unlock();

                // For non-mDNS clients, let them know it succeeded
                if (!m_Mdns) {
                    emit computerAddCompleted(true, false);
                }

                // Tell our client if something changed
                if (changed) {
                    qInfo() << existingComputer->name << "is now at" << existingComputer->activeAddress.toString();
                    emit computerStateChanged(existingComputer);
                }
            }
            else {
                // Store this in our active sets
                m_ComputerManager->m_KnownHosts[newComputer->uuid] = newComputer;

                // 为自动扫描的设备创建默认记录
                m_ComputerManager->registerDeviceInfo(newComputer);

                // Start polling if enabled (write lock required)
                m_ComputerManager->startPollingComputer(newComputer);

                // Drop the lock before notifying
                m_ComputerManager->m_Lock.unlock();

                // If this wasn't added via mDNS but it is a RFC 1918 IPv4 address and not a VPN,
                // go ahead and do the STUN request now to populate an external address.
                if (!m_Mdns && addressIsSiteLocalV4 && newComputer->getActiveAddressReachability() != NvComputer::RI_VPN) {
                    quint32 addr;
                    int err = LiFindExternalAddressIP4("stun.moonlight-stream.org", 3478, &addr);
                    if (err == 0) {
                        newComputer->setRemoteAddress(QHostAddress(qFromBigEndian(addr)));
                    }
                    else {
                        qWarning() << "STUN failed to get WAN address:" << err;
                    }
                }

                // For non-mDNS clients, let them know it succeeded
                if (!m_Mdns) {
                    emit computerAddCompleted(true, false);
                }

                // Tell our client about this new PC
                emit computerStateChanged(newComputer);
            }
        }
    }

    ComputerManager* m_ComputerManager;
    NvAddress m_Address;
    NvAddress m_MdnsIpv6Address;
    bool m_Mdns;
    bool m_AboutToQuit;
};

void ComputerManager::addNewHost(NvAddress address, bool mdns, NvAddress mdnsIpv6Address)
{
    // Punt to a worker thread to avoid stalling the
    // UI while waiting for serverinfo query to complete
    PendingAddTask* addTask = new PendingAddTask(this, address, mdnsIpv6Address, mdns);
    QThreadPool::globalInstance()->start(addTask);

}

// TODO: Use QRandomGenerator when we drop Qt 5.9 support
QString ComputerManager::generatePinString()
{
    std::uniform_int_distribution<int> dist(0, 9999);
    std::random_device rd;
    std::mt19937 engine(rd());

    return QString::asprintf("%04u", dist(engine));
}

void ComputerManager::syncOrderDevices()
{
    qint64 uid = UserSession::instance()->userId();
    if (uid == 0) {
        clearOrderDevices();
        return;
    }

    LOG_DEBUG("----------------------------------------");
    LOG_DEBUG(QStringLiteral("[ComputerManager::syncOrderDevices] uid=%1").arg(uid));

    ApiService::getAllDeviceOrderInfoByUserId(QString::number(uid),
            [this](QString json) {
                LOG_INFO(QStringLiteral("[syncOrderDevices result] %1").arg(json));
                updateOrderInfoFromJson(json);
            },
            [](QString err) {
                LOG_WARN(QStringLiteral("[syncOrderDevices] 请求失败: %1").arg(err));
            }
            );

    ApiService::getUserInfoById(QString::number(uid),
            [](QString data) {
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.value("code").toInt() == 200) {
                        QJsonObject userData = obj.value("data").toObject();
                        double balance = userData.value("balance").toDouble();
                        UserSession::instance()->setBalance(balance);
                    }
                }
            },
            [](QString err) {
                LOG_WARN(QStringLiteral("[getUserInfoById] 请求失败: %1").arg(err));
            }
            );
}

void ComputerManager::updateOrderInfoFromJson(const QString& json)
{
    LOG_DEBUG("----------------------------------------");
    LOG_DEBUG("[ComputerManager::updateOrderInfoFromJson]");

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        LOG_WARN(QStringLiteral("[updateOrderInfoFromJson] 无效的 JSON: %1").arg(json));
        return;
    }

    QJsonArray arr = doc["data"].toArray();
    QSet<QString> newKeys;
    QList<std::tuple<QString, quint16, QString>> newDevices;
    QSet<QString> removed;

    {
        // 写锁保护订单数据
        QWriteLocker wlock(&m_OrderLock);

        for (auto v : arr) {
            QJsonObject o = v.toObject();
            QString ip = o["ip"].toString();
            quint16 port = o["port"].toString().toUShort();
            QString key = deviceKey(ip, port);
            newKeys << key;

            // 保存订单附带的信息，供 UI 展示
            DeviceInfo info;
            QJsonObject orderObj = o["deviceOrderInfo"].toObject();
            info.orderId = orderObj["orderId"].toVariant().toLongLong();
            info.bitrate = orderObj["bitrate"].toDouble();
            info.startedAt = QDateTime::fromString(orderObj["startedAt"].toString(), Qt::ISODate);
            info.endedAt = QDateTime::fromString(orderObj["endedAt"].toString(), Qt::ISODate);
            // \u4fdd\u5b58 devicePriceId
            info.deviceGroupId = orderObj["devicePriceId"].toInt();
            info.status = orderObj["status"].toInt();
            info.billingType = orderObj["billingType"].toInt();
            m_DeviceInfo.insert(key, info);

            if (!m_OrderDeviceKeys.contains(key))
                newDevices.append({ip, port, o["name"].toString()});
        }

        removed = m_OrderDeviceKeys - newKeys;
        m_OrderDeviceKeys = newKeys;
    }

    // 添加新设备
    for (auto& d : newDevices) {
        LOG_INFO(QStringLiteral("[OrderSync] 新增设备 %1 %2 %3")
                     .arg(std::get<2>(d))
                     .arg(std::get<0>(d))
                     .arg(std::get<1>(d)));
        addNewHost(NvAddress(std::get<0>(d), std::get<1>(d)), false);
    }

    // 删除消失的设备（支持多个）
    if (!removed.isEmpty()) {
        QList<NvComputer*> toDelete;

        QReadLocker rlock(&m_Lock);
        for (NvComputer* pc : m_KnownHosts) {
            QString key = deviceKey(pc);

            if (removed.contains(key)) {
                LOG_INFO(QStringLiteral("[OrderSync] 标记删除设备 %1").arg(pc->name));
                toDelete.append(pc);
            }
        }

        // 延迟删除所有主机
        for (NvComputer* pc : toDelete) {
            handleComputerStateChanged(pc);  //  主动发信号刷新 UI
            deleteHost(pc);                 //  延迟删除（含线程清理）
        }
    }
}

void ComputerManager::allocateDevice(int deviceGroupId, int billingType)
{
    qint64 uid = UserSession::instance()->userId();
    if (uid == 0) {
        emit allocateDeviceFinished(false, QStringLiteral("Invalid user"));
        return;
    }

    QString reqId = QString::fromLatin1(QUuid::createUuid().toRfc4122().toHex());

    LOG_DEBUG("----------------------------------------");
    LOG_DEBUG(QStringLiteral("[ComputerManager::allocateDevice] uid=%1 group=%2 billing=%3 reqId=%4")
                      .arg(uid)
                      .arg(deviceGroupId)
                      .arg(billingType)
                      .arg(reqId));

    ApiService::allocateDevice(QString::number(uid),
                               QString::number(deviceGroupId),
                               QString::number(billingType),
                               reqId,
            [this](QString json) {
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
                if (!doc.isObject()) {
                    emit allocateDeviceFinished(false, QStringLiteral("Invalid response"));
                    return;
                }
                QJsonObject obj = doc.object();
                int code = obj.value("code").toInt();
                QString msg = obj.value("message").toString();
                bool data = obj.value("data").toBool();
                LOG_INFO(QStringLiteral("[allocateDevice result] code=%1 msg=%2 data=%3")
                             .arg(code)
                             .arg(msg)
                             .arg(data));
                if (code == 200 && data) {
                    emit allocateDeviceFinished(true, msg);
                } else {
                    emit allocateDeviceFinished(false, msg.isEmpty() ? QString::number(code) : msg);
                }
            },
            [this](QString err) {
                emit allocateDeviceFinished(false, err);
            }
        );
}

void ComputerManager::closeOrder(NvComputer* computer)
{
    QString key = deviceKey(computer);

    qint64 orderId = 0;
    {
        QReadLocker rlock(&m_OrderLock);
        if (!m_DeviceInfo.contains(key) || m_DeviceInfo.value(key).billingType == 0) {
            emit closeOrderFinished(false, QStringLiteral("Order ID not found"));
            return;
        }
        orderId = m_DeviceInfo.value(key).orderId;
    }

    LOG_DEBUG("----------------------------------------");
    LOG_DEBUG(QStringLiteral("[ComputerManager::closeOrder] orderId=%1").arg(orderId));

    ApiService::closeOrder(QString::number(orderId),
            [this, computer, key](QString json) {
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
                if (!doc.isObject()) {
                    emit closeOrderFinished(false, QStringLiteral("Invalid response"));
                    return;
                }
                QJsonObject obj = doc.object();
                int code = obj.value("code").toInt();
                QString msg = obj.value("message").toString();
                bool data = obj.value("data").toBool();
                LOG_INFO(QStringLiteral("[closeOrder result] code=%1 msg=%2 data=%3")
                             .arg(code)
                             .arg(msg)
                             .arg(data));
                if (code == 200 && data) {
                    {
                        QWriteLocker wlock(&m_OrderLock);
                        m_DeviceInfo.remove(key);
                        m_OrderDeviceKeys.remove(key);
                    }
                    deleteHost(computer);
                    emit closeOrderFinished(true, msg);
                } else {
                    emit closeOrderFinished(false, msg.isEmpty() ? QString::number(code) : msg);
                }
            },
            [this](QString err) {
                emit closeOrderFinished(false, err);
            });
}

void ComputerManager::getOrderDetailList()
{
    qint64 uid = UserSession::instance()->userId();
    if (uid == 0) {
        emit getOrderDetailListFinished(false, QStringLiteral("Invalid user"));
        return;
    }

    ApiService::getOrderDetailList(QString::number(uid),
            [this](QString json) {
                emit getOrderDetailListFinished(true, json);
            },
            [this](QString err) {
                emit getOrderDetailListFinished(false, err);
            });
}

void ComputerManager::rechargeOrder(qint64 orderId, int num, int billingType)
{
    LOG_DEBUG("----------------------------------------");
    LOG_DEBUG(QStringLiteral("[ComputerManager::rechargeOrder] orderId=%1 num=%2 billing=%3")
                      .arg(orderId)
                      .arg(num)
                      .arg(billingType));

    ApiService::rechargeOrder(QString::number(orderId),
                              QString::number(num),
                              QString::number(billingType),
            [this](QString json) {
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
                if (!doc.isObject()) {
                    emit rechargeOrderFinished(false, QStringLiteral("Invalid response"));
                    return;
                }
                QJsonObject obj = doc.object();
                int code = obj.value("code").toInt();
                QString msg = obj.value("message").toString();
                bool data = obj.value("data").toBool();
                LOG_INFO(QStringLiteral("[rechargeOrder result] code=%1 msg=%2 data=%3")
                             .arg(code)
                             .arg(msg)
                             .arg(data));
                if (code == 200 && data) {
                    emit rechargeOrderFinished(true, msg);
                } else {
                    emit rechargeOrderFinished(false, msg.isEmpty() ? QString::number(code) : msg);
                }
            },
            [this](QString err) {
                emit rechargeOrderFinished(false, err);
            });
}

int ComputerManager::getDeviceGroupIdByOrderId(qint64 orderId)
{
    QReadLocker rlock(&m_OrderLock);
    for (auto it = m_DeviceInfo.constBegin(); it != m_DeviceInfo.constEnd(); ++it) {
        if (it.value().orderId == orderId)
            return it.value().deviceGroupId;
    }
    return 0;
}

QString ComputerManager::deviceKey(const QString& ip, quint16 port) const
{
    return QString("%1:%2").arg(ip).arg(port);
}

QString ComputerManager::deviceKey(NvComputer* computer) const
{
    QString addr = !computer->manualAddress.isNull() ? computer->manualAddress.address()
                                                   : computer->localAddress.address();
    quint16 port = !computer->manualAddress.isNull() ? computer->manualAddress.port()
                                                    : computer->localAddress.port();
    return deviceKey(addr, port);
}

void ComputerManager::registerDeviceInfo(const QString& ip, quint16 port, const DeviceInfo& info)
{
    QString key = deviceKey(ip, port);
    QWriteLocker wlock(&m_OrderLock);
    if (!m_DeviceInfo.contains(key))
        m_DeviceInfo.insert(key, info);
}

void ComputerManager::registerDeviceInfo(NvComputer* computer)
{
    QString addr = !computer->manualAddress.isNull() ? computer->manualAddress.address()
                                                   : computer->localAddress.address();
    quint16 port = !computer->manualAddress.isNull() ? computer->manualAddress.port()
                                                    : computer->localAddress.port();
    registerDeviceInfo(addr, port, DeviceInfo());
}


#include "computermanager.moc"
