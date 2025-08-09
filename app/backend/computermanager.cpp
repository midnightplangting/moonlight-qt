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

        // 确保回应的主机就是我们想要联系的那台
        if (m_Computer->uuid != newState.uuid) {
            bool nameMatches = (m_Computer->name == newState.name);
            bool ipMatches = newState.uniqueAddresses().contains(address);

            if (m_Computer->uuid.isEmpty() || (nameMatches && ipMatches)) {
                bool uuidChanged = (m_Computer->uuid != newState.uuid);
                {
                    QWriteLocker lock(&m_Computer->lock);
                    m_Computer->uuid = newState.uuid;
                }
                changed = m_Computer->update(newState) || uuidChanged;
                return true;
            }

            LOG_WARN_T(QStringLiteral("[Polling] Found unexpected PC %1 while looking for %2")
                       .arg(newState.name, m_Computer->name));
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
        // 轮询线程的生命周期：
        // 1. 在 ComputerManager::startPollingComputer() 中创建并启动。
        // 2. stopPollingAsync() 或应用退出时通过 requestInterruption() 中断。
        // 3. 线程循环执行以下逻辑直至被中断。

        LOG_INFO_T(QStringLiteral("[PcMonitorThread] run() started for %1")
                   .arg(m_Computer->name));

        // 第一次必定获取应用列表
        int pollsSinceLastAppListFetch = POLLS_PER_APPLIST_FETCH;

        while (!isInterruptionRequested()) {
            LOG_DEBUG_T(QStringLiteral("[Polling] 开始轮询 %1").arg(m_Computer->name));
            bool stateChanged = false;
            bool online = false;
            bool wasOnline = m_Computer->state == NvComputer::CS_ONLINE;
            for (int i = 0; i < (wasOnline ? TRIES_BEFORE_OFFLINING : 1) && !online; i++) {
                for (auto& address : m_Computer->uniqueAddresses()) {
                    LOG_DEBUG_T(QStringLiteral("[Polling] 尝试 %1").arg(address.toString()));
                    if (isInterruptionRequested()) {
                        return;
                    }

                    if (tryPollComputer(address, stateChanged)) {
                        if (!wasOnline) {
                            LOG_INFO_T(QStringLiteral("[Polling] %1 is now online at %2")
                                       .arg(m_Computer->name,
                                            m_Computer->activeAddress.toString()));
                            Logger::logComputer(m_Computer);
                        }
                        online = true;
                        break;
                    }
                }
            }

            // 在所有重试后仍失败就认为离线
            // 注意：这里无需获取读锁，
            // 因为当前线程已经持有写锁
            if (!online && m_Computer->state != NvComputer::CS_OFFLINE) {
                LOG_INFO_T(QStringLiteral("[Polling] %1 is now offline").arg(m_Computer->name));
                m_Computer->state = NvComputer::CS_OFFLINE;
                stateChanged = true;
                Logger::logComputer(m_Computer);
            }

            // 如果应用列表为空或距离上次获取已够久则重新获取
            pollsSinceLastAppListFetch++;
            if (m_Computer->state == NvComputer::CS_ONLINE &&
                    m_Computer->pairState == NvComputer::PS_PAIRED &&
                    (m_Computer->appList.isEmpty() || pollsSinceLastAppListFetch >= POLLS_PER_APPLIST_FETCH)) {
                // 在获取应用列表前先通知，因为该操作可能较慢，
                // 避免延迟主机上线（即使已有缓存列表）
                if (stateChanged) {
                    emit computerStateChanged(m_Computer);
                    stateChanged = false;
                }

                if (updateAppList(stateChanged)) {
                    LOG_INFO_T(QStringLiteral("[Polling] 已刷新应用列表"));
                    pollsSinceLastAppListFetch = 0;
                }
            }

            if (stateChanged) {
                // 通知监听者主机状态已变化
                emit computerStateChanged(m_Computer);
            }

            // 定期同步订单信息
            m_Manager->checkOrderStatus(m_Computer);

            // 等待后再轮询，以100毫秒为粒度便于及时中断
            // FIXME: 使用 QWaitCondition 会更好
            for (int i = 0; i < 30 && !isInterruptionRequested(); i++) {
                QThread::msleep(100);
            }
            Logger::logComputer(m_Computer);
            LOG_DEBUG_T(QStringLiteral("[Polling] 本轮结束"));
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

    // 如果存在主机备份，说明上次更新未成功写入，现恢复备份
    int hosts = settings.beginReadArray(SER_HOSTS_BACKUP);
    if (hosts == 0) {
        // 如果没有备份，则从主存储读取
        settings.endArray();
        hosts = settings.beginReadArray(SER_HOSTS);
    }

    // 从 QSettings 还原主机列表
    for (int i = 0; i < hosts; i++) {
        settings.setArrayIndex(i);
        NvComputer* computer = new NvComputer(settings);
        m_KnownHosts[computer->uuid] = computer;
        m_LastSerializedHosts[computer->uuid] = *computer;
    }
    settings.endArray();

    // 异步获取最新兼容性数据
    m_CompatFetcher.start();

    // 启动延迟刷新线程处理 saveHosts()
    m_DelayedFlushThread = new DelayedFlushThread(this);
    m_DelayedFlushThread->start();

    // 定时同步订单状态，确保串流过程中也能获取最新状态
    m_OrderTimer.setInterval(30000);
    connect(&m_OrderTimer, &QTimer::timeout, this, &ComputerManager::syncOrderDevices);

    // 为了及时退出，收到 aboutToQuit() 信号后需阻止新的请求。
    // 因为 NvHTTP 会在该信号时中断进行中的请求，但该信号只发一次，
    // 后续的请求不会被终止，可能阻塞退出。
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &ComputerManager::handleAboutToQuit);

}

ComputerManager::~ComputerManager()
{
    // 在获取写锁前停止延迟刷新线程，
    // 避免与需要读锁的刷新操作发生死锁
    {
        // 唤醒延迟刷新线程
        m_DelayedFlushThread->requestInterruption();
        m_DelayedFlushCondition.wakeOne();

        // 等待线程结束并完成所有待刷新
        m_DelayedFlushThread->wait();
        delete m_DelayedFlushThread;

        // 此时所有延迟刷新应已完成
        Q_ASSERT(!m_NeedsDelayedFlush);
    }

    QWriteLocker lock(&m_Lock);

    // 删除尚未解析的主机
    while (!m_PendingResolution.isEmpty()) {
        MdnsPendingComputer* computer = m_PendingResolution.first();
        delete computer;
        m_PendingResolution.removeFirst();
    }

    // 删除浏览器以停止发现
    delete m_MdnsBrowser;
    m_MdnsBrowser = nullptr;

    // 中断轮询线程
    for (ComputerPollingEntry* entry : m_PollEntries) {
        entry->interrupt();
    }

    // 删除所有轮询项及其关联线程
    for (ComputerPollingEntry* entry : m_PollEntries) {
        delete entry;
    }

    // 轮询已停止，销毁所有 NvComputer 对象
    for (NvComputer* computer : m_KnownHosts) {
        delete computer;
    }
}

void DelayedFlushThread::run() {
    for (;;) {
        // 等待延迟刷新请求或线程中断
        {
            QMutexLocker locker(&m_ComputerManager->m_DelayedFlushMutex);

            while (!QThread::currentThread()->isInterruptionRequested() && !m_ComputerManager->m_NeedsDelayedFlush) {
                m_ComputerManager->m_DelayedFlushCondition.wait(&m_ComputerManager->m_DelayedFlushMutex);
            }

            // 如果仅因中断被唤醒则不刷新；若同时有刷新请求则执行刷新
            if (!m_ComputerManager->m_NeedsDelayedFlush) {
                Q_ASSERT(QThread::currentThread()->isInterruptionRequested());
                break;
            }

            // 重置延迟刷新标记，确保并发的 saveHosts() 能重新设置
            m_ComputerManager->m_NeedsDelayedFlush = false;

            // 在互斥锁下更新最近序列化的主机映射
            m_ComputerManager->m_LastSerializedHosts.clear();
            for (const NvComputer* computer : m_ComputerManager->m_KnownHosts) {
                // 复制当前 NvComputer 状态，便于后续属性变更时判断是否需要再次序列化
                QReadLocker computerLock(&computer->lock);
                m_ComputerManager->m_LastSerializedHosts[computer->uuid] = *computer;
            }
        }

        // 执行刷新操作
        {
            QSettings settings;

            // 首先写入备份位置
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

            // 接着写入主位置
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

            // 最后删除备份文件
            settings.remove(SER_HOSTS_BACKUP);
        }
    }
}

void ComputerManager::saveHosts()
{
    Q_ASSERT(m_DelayedFlushThread != nullptr && m_DelayedFlushThread->isRunning());

    // 由于 macOS 上 QSettings 写入可能非常慢（超过 500ms），
    // 因此在工作线程中执行以避免阻塞主线程
    QMutexLocker locker(&m_DelayedFlushMutex);
    m_NeedsDelayedFlush = true;
    m_DelayedFlushCondition.wakeOne();
}

QHostAddress ComputerManager::getBestGlobalAddressV6(QVector<QHostAddress> &addresses)
{
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv6Protocol) {
            if (address.isInSubnet(QHostAddress("fe80::"), 10)) {
                // 链路本地地址
                continue;
            }

            if (address.isInSubnet(QHostAddress("fec0::"), 10)) {
                LOG_INFO(QStringLiteral("Ignoring site-local address: %1").arg(address.toString()));
                continue;
            }

            if (address.isInSubnet(QHostAddress("fc00::"), 7)) {
                LOG_INFO(QStringLiteral("Ignoring ULA: %1").arg(address.toString()));
                continue;
            }

            if (address.isInSubnet(QHostAddress("2002::"), 16)) {
                LOG_INFO(QStringLiteral("Ignoring 6to4 address: %1").arg(address.toString()));
                continue;
            }

            if (address.isInSubnet(QHostAddress("2001::"), 32)) {
                LOG_INFO(QStringLiteral("Ignoring Teredo address: %1").arg(address.toString()));
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

    LOG_INFO("[ComputerManager] 开始启动轮询");

    if (++m_PollingRef > 1) {
        return;
    }

    if (m_Prefs->enableMdns) {
        // 开始对 GameStream 主机进行 mDNS 查询
        m_MdnsServer.reset(new QMdnsEngine::Server());
        m_MdnsBrowser = new QMdnsEngine::Browser(m_MdnsServer.data(), "_nvstream._tcp.local.");
        connect(m_MdnsBrowser, &QMdnsEngine::Browser::serviceAdded,
                this, [this](const QMdnsEngine::Service& service) {
            LOG_INFO(QStringLiteral("Discovered mDNS host: %1").arg(service.hostname()));

            MdnsPendingComputer* pendingComputer = new MdnsPendingComputer(m_MdnsServer, service);
            connect(pendingComputer, &MdnsPendingComputer::resolvedHost,
                    this, &ComputerManager::handleMdnsServiceResolved);
            m_PendingResolution.append(pendingComputer);
        });
    }
    else {
        qWarning() << "mDNS is disabled by user preference";
    }

    // 为每个已知主机启动轮询线程
    QMapIterator<QString, NvComputer*> i(m_KnownHosts);
    while (i.hasNext()) {
        i.next();
        startPollingComputer(i.value());
    }

    if (!m_OrderTimer.isActive())
        m_OrderTimer.start();
}

// 调用此函数前必须持有 m_Lock 的写锁
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
        LOG_INFO(QStringLiteral("[ComputerManager] 启动 %1 的轮询线程").arg(computer->name));
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

    // 先尝试使用 IPv4 地址添加主机
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            // 注意：此处不直接使用 v6Global 调用 addNewHost()，因为 IPv6 地址可能暂时不可达
            // （例如用户尚未安装 IPv6 辅助组件或主机不具备外网 IPv6 能力）。
            // 即便暂时不可达，也希望记录下 IPv6 地址。
            addNewHost(NvAddress(address, computer->port()), true, NvAddress(v6Global, computer->port()));
            added = true;
            break;
        }
    }

    if (!added) {
        // 如果没有 IPv4 地址，则只使用 IPv6 地址添加
        for (const QHostAddress& address : addresses) {
            if (address.protocol() == QAbstractSocket::IPv6Protocol) {
                // 将链路本地或站点本地地址作为“本地地址”保存
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
    // 若无可序列化属性变更，则无需保存主机信息
    QMutexLocker lock(&m_DelayedFlushMutex);
    QReadLocker computerLock(&computer->lock);
    if (!m_LastSerializedHosts.value(computer->uuid).isEqualSerialized(*computer)) {
        // 在释放锁后发送延迟写入请求
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

    // 返回已排序的主机列表
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

        // 持有写锁期间仅做最少的工作，
        // 调用 saveHosts() 前必须先释放锁
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
    // 在工作线程中执行，以免等待轮询线程结束时阻塞 UI
    QThreadPool::globalInstance()->start(new DeferredHostDeletionTask(this, computer));
}

void ComputerManager::renameHost(NvComputer* computer, QString name)
{
    {
        QWriteLocker lock(&computer->lock);

        computer->name = name;
        computer->hasCustomName = true;
    }

    // 通知 UI 状态已变更
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
    // 通知 UI 状态已变更
    handleComputerStateChanged(computer);
}

void ComputerManager::handleAboutToQuit()
{
    QReadLocker lock(&m_Lock);

    // 立即中断轮询线程，避免退出过程中继续发起请求
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
    // 若为订单/云设备则自动发送 PIN
    QString addr = !computer->manualAddress.isNull() ? computer->manualAddress.address()
                                                    : computer->localAddress.address();
    quint16 port = !computer->manualAddress.isNull() ? computer->manualAddress.port()
                                                     : computer->localAddress.port();
    QString key = deviceKey(computer);

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

    // 在工作线程中执行，以免等待配对完成时阻塞 UI
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
                // 状态码 599 需要返回自定义提示信息
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

    // 删除尚未解析完成的主机
    while (!m_PendingResolution.isEmpty()) {
        MdnsPendingComputer* computer = m_PendingResolution.first();
        computer->deleteLater();
        m_PendingResolution.removeFirst();
    }

    // 删除浏览器和服务器以停止发现并刷新轮询
    delete m_MdnsBrowser;
    m_MdnsBrowser = nullptr;
    m_MdnsServer.reset();

    // 中断所有线程，但不等待其结束
    for (ComputerPollingEntry* entry : m_PollEntries) {
        entry->interrupt();
    }
}

void ComputerManager::addNewHostManually(QString address)
{
    QUrl url = QUrl::fromUserInput("moonlight://" + address);
    if (url.isValid() && !url.host().isEmpty() && url.scheme() == "moonlight") {
        // 如果未指定端口，则使用默认端口
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
    PendingAddTask(ComputerManager* computerManager, NvAddress address,
                   NvAddress mdnsIpv6Address, bool mdns,
                   bool notifyOnFailure)
        : m_ComputerManager(computerManager),
          m_Address(address),
          m_MdnsIpv6Address(mdnsIpv6Address),
          m_Mdns(mdns),
          m_NotifyOnFailure(notifyOnFailure),
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

        // 若正在退出则直接返回
        if (m_AboutToQuit) {
            return QString();
        }

        try {
            // GameStream 通过 mDNS 报告在线状态与 HTTPS 服务就绪之间存在竞争条件，
            // 因此若收到 ServiceUnavailableError，则等待数秒后重试请求
            try {
                serverInfo = http.getServerInfo(NvHTTP::NVLL_VERBOSE);
            } catch (const QtNetworkReplyException& e) {
                if (e.getError() == QNetworkReply::ServiceUnavailableError) {
                    qWarning() << "Retrying request in 5 seconds after ServiceUnavailableError";
                    QThread::sleep(5);
                    serverInfo = http.getServerInfo(NvHTTP::NVLL_VERBOSE);
                    LOG_INFO("Retry successful");
                }
                else {
                    // 其他错误继续抛出
                    throw e;
                }
            }
            return serverInfo;
        } catch (...) {
            if (!m_Mdns && m_NotifyOnFailure) {
                unsigned int portTestResult = 0;

                if (m_ComputerManager->m_Prefs->detectNetworkBlocking) {
                    // 无法连接指定 PC，测试网络是否阻止 Moonlight，以便提示用户
                    portTestResult = LiTestClientConnectivity("qt.conntest.moonlight-stream.org", 443,
                                                              ML_PORT_FLAG_TCP_47984 | ML_PORT_FLAG_TCP_47989);
                }

                emit computerAddCompleted(false,
                                         portTestResult != 0 &&
                                         portTestResult != ML_TEST_RESULT_INCONCLUSIVE);
            }
            return QString();
        }
    }

    void run()
    {
        NvHTTP http(m_Address, 0, QSslCertificate());

        LOG_INFO(QStringLiteral("Processing new PC at %1 from %2 with IPv6 address %3")
                 .arg(m_Address.toString(),
                      m_Mdns ? QStringLiteral("mDNS") : QStringLiteral("user"),
                      m_MdnsIpv6Address.toString()));

        // 首先通过 HTTP 获取服务器信息，此时尚未确定证书
        QString serverInfo = fetchServerInfo(http);
        if (serverInfo.isEmpty() && !m_MdnsIpv6Address.isNull()) {
            // 如果 IPv4 或链路本地 IPv6 地址失败，则尝试使用全局 IPv6 地址重试
            http.setAddress(m_MdnsIpv6Address);
            serverInfo = fetchServerInfo(http);
        }
        if (serverInfo.isEmpty()) {
            return;
        }

        // 使用 HTTP 获取的服务器信息创建初始的 newComputer，不使用固定证书
        NvComputer* newComputer = new NvComputer(http, serverInfo);

        // 检查是否已有该主机 UUID 的记录以便获取固定证书
        NvComputer* existingComputer;
        {
            QReadLocker lock(&m_ComputerManager->m_Lock);
            existingComputer = m_ComputerManager->m_KnownHosts.value(newComputer->uuid);
            if (existingComputer != nullptr) {
                http.setServerCert(existingComputer->serverCert);
            }
        }

        // 使用固定证书再通过 HTTPS 获取服务器信息
        if (existingComputer != nullptr) {
            Q_ASSERT(http.httpsPort() != 0);
            serverInfo = fetchServerInfo(http);
            if (serverInfo.isEmpty()) {
                return;
            }

            // 使用 HTTPS 获取的信息更新 newComputer
            NvComputer httpsComputer(http, serverInfo);
            newComputer->update(httpsComputer);
        }

        // 根据不同情况更新地址信息
        if (m_Mdns) {
            // 仅在实际通过该地址访问成功时更新本地地址；
            // 若最终通过 IPv6 地址访问，则不保存不可用的本地地址
            if (http.address() == m_Address) {
                newComputer->localAddress = m_Address;
            }

            // 如果是通过 IPv4 的 mDNS，使用 STUN 获取公网地址
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
            // 使用读锁检查该 PC 是否已存在
            m_ComputerManager->m_Lock.lockForRead();
            NvComputer* existingComputer = m_ComputerManager->m_KnownHosts.value(newComputer->uuid);

            // 若不存在则转为写锁以便更新。
            // 注意：ComputerManager 的锁仅保护主机列表本身，
            // 其中的元素由各自锁保护。因此仅在新增 PC 时需要写锁。
            if (existingComputer == nullptr) {
                m_ComputerManager->m_Lock.unlock();
                m_ComputerManager->m_Lock.lockForWrite();

                // 解锁后再获取写锁期间可能有其他线程添加了该主机，因此需要再次检查
                existingComputer = m_ComputerManager->m_KnownHosts.value(newComputer->uuid);
            }

            if (existingComputer != nullptr) {
                // 将数据合并到已存在的主机中
                bool changed = existingComputer->update(*newComputer);
                delete newComputer;

                // 通知之前先释放锁
                m_ComputerManager->m_Lock.unlock();

                // 若不是通过 mDNS 添加，通知调用方成功
                if (!m_Mdns) {
                    emit computerAddCompleted(true, false);
                }

                // 如果有变化则通知客户端
                if (changed) {
                    LOG_INFO(QStringLiteral("%1 is now at %2")
                             .arg(existingComputer->name,
                                  existingComputer->activeAddress.toString()));
                    emit computerStateChanged(existingComputer);
                }
            }
            else {
                // 将其加入活动主机列表
                m_ComputerManager->m_KnownHosts[newComputer->uuid] = newComputer;

                // 为自动扫描的设备创建默认记录
                m_ComputerManager->registerDeviceInfo(newComputer);

                // 如果已启用则启动该主机的轮询（需要写锁）
                m_ComputerManager->startPollingComputer(newComputer);

                // 通知前先释放锁
                m_ComputerManager->m_Lock.unlock();

                // 如果不是通过 mDNS 添加且属于 RFC1918 IPv4 地址且非 VPN，
                // 立即执行 STUN 请求以获取外网地址
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

                // 若不是通过 mDNS 添加，通知调用方成功
                if (!m_Mdns) {
                    emit computerAddCompleted(true, false);
                }

                // 通知客户端发现了新的主机
                emit computerStateChanged(newComputer);
            }
        }
    }

    ComputerManager* m_ComputerManager;
    NvAddress m_Address;
    NvAddress m_MdnsIpv6Address;
    bool m_Mdns;
    bool m_NotifyOnFailure;
    bool m_AboutToQuit;
};

void ComputerManager::addNewHost(NvAddress address, bool mdns,
                                 NvAddress mdnsIpv6Address,
                                 bool notifyOnFailure)
{
    // 在工作线程中执行，避免等待服务器信息查询时阻塞 UI
    PendingAddTask* addTask = new PendingAddTask(this, address,
                                                 mdnsIpv6Address, mdns,
                                                 notifyOnFailure);
    QThreadPool::globalInstance()->start(addTask);

}

// TODO: 等不再兼容 Qt 5.9 时改用 QRandomGenerator
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
            QString name = o["name"].toString();
            QString key = deviceKey(name);
            newKeys << key;

            // 保存订单附带的信息，供 UI 展示
            DeviceInfo info;
            QJsonObject orderObj = o["deviceOrderInfo"].toObject();
            info.orderId = orderObj["orderId"].toVariant().toLongLong();
            info.bitrate = orderObj["bitrate"].toDouble();
            info.startedAt = QDateTime::fromString(orderObj["startedAt"].toString(), Qt::ISODate);
            info.endedAt = QDateTime::fromString(orderObj["endedAt"].toString(), Qt::ISODate);
            // 记录设备组 ID(devicePriceId) 和设备 ID
            info.deviceGroupId = orderObj["devicePriceId"].toInt();
            info.deviceId = orderObj["deviceId"].toInt();
            info.status = orderObj["status"].toInt();
            info.billingType = orderObj["billingType"].toInt();
            m_DeviceInfo.insert(key, info);

            bool knownHost = false;
            {
                QReadLocker hostLock(&m_Lock);
                for (NvComputer* pc : m_KnownHosts) {
                    if (deviceKey(pc) == key) {
                        knownHost = true;
                        break;
                    }
                }
            }
            if (!knownHost)
                newDevices.append({ip, port, name});
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
        addNewHost(NvAddress(std::get<0>(d), std::get<1>(d)),
                    false, NvAddress(), false);
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

void ComputerManager::restartSunshine(qint64 orderId)
{
    LOG_DEBUG("----------------------------------------");
    LOG_DEBUG(QStringLiteral("[ComputerManager::restartSunshine] orderId=%1")
                      .arg(orderId));

    ApiService::restartSunshine(QString::number(orderId),
            [this](QString json) {
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
                if (!doc.isObject()) {
                    emit restartSunshineFinished(false, QStringLiteral("Invalid response"));
                    return;
                }
                QJsonObject obj = doc.object();
                int code = obj.value("code").toInt();
                QString msg = obj.value("message").toString();
                bool data = obj.value("data").toBool();
                LOG_INFO(QStringLiteral("[restartSunshine result] code=%1 msg=%2 data=%3")
                             .arg(code)
                             .arg(msg)
                             .arg(data));
                if (code == 200 && data) {
                    emit restartSunshineFinished(true, msg);
                } else {
                    emit restartSunshineFinished(false, msg.isEmpty() ? QString::number(code) : msg);
                }
            },
            [this](QString err) {
                emit restartSunshineFinished(false, err);
            });
}

void ComputerManager::getDevicePriceList(int deviceId)
{
    QString token = UserSession::instance()->token();
    ApiService::getDevicePriceList(token,
                                   QString::number(deviceId),
            [this](QString json) {
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
                if (!doc.isObject()) {
                    emit getDevicePriceListFinished(false, {}, QStringLiteral("Invalid response"));
                    return;
                }
                QJsonObject obj = doc.object();
                int code = obj.value("code").toInt();
                QString msg = obj.value("message").toString();
                QVariantList list;
                if (code == 200) {
                    QJsonArray arr = obj.value("data").toArray();
                    for (const QJsonValue& v : arr)
                        list.append(v.toVariant());
                    emit getDevicePriceListFinished(true, list, msg);
                } else {
                    emit getDevicePriceListFinished(false, list, msg.isEmpty() ? QString::number(code) : msg);
                }
            },
            [this](QString err) {
                emit getDevicePriceListFinished(false, {}, err);
            });
}

bool ComputerManager::fetchAppListSync(NvComputer* computer, bool* changed)
{
    NvHTTP http(computer);
    QVector<NvApp> appList;

    try {
        appList = http.getAppList();
        if (appList.isEmpty()) {
            return false;
        }
    } catch (...) {
        return false;
    }

    QWriteLocker lock(&computer->lock);
    bool listChanged = computer->updateAppList(appList);
    if (changed)
        *changed = listChanged;
    return true;
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

int ComputerManager::getDeviceIdByOrderId(qint64 orderId)
{
    QReadLocker rlock(&m_OrderLock);
    for (auto it = m_DeviceInfo.constBegin(); it != m_DeviceInfo.constEnd(); ++it) {
        if (it.value().orderId == orderId)
            return it.value().deviceId;
    }
    return 0;
}

QString ComputerManager::deviceKey(const QString& name) const
{
    return name;
}

QString ComputerManager::deviceKey(NvComputer* computer) const
{
    return computer->name;
}

void ComputerManager::registerDeviceInfo(const QString& name, const DeviceInfo& info)
{
    QString key = deviceKey(name);
    QWriteLocker wlock(&m_OrderLock);
    if (!m_DeviceInfo.contains(key))
        m_DeviceInfo.insert(key, info);
}

void ComputerManager::registerDeviceInfo(NvComputer* computer)
{
    registerDeviceInfo(computer->name, DeviceInfo());
}


#include "computermanager.moc"
