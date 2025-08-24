#include "computermodel.h"

#include <QThreadPool>
#include "../backend/Logger.h"

ComputerModel::ComputerModel(QObject* object)
    : QAbstractListModel(object) {}

void ComputerModel::initialize(ComputerManager* computerManager)
{
    m_ComputerManager = computerManager;
    connect(m_ComputerManager, &ComputerManager::computerStateChanged,
            this, &ComputerModel::handleComputerStateChanged);
    connect(m_ComputerManager, &ComputerManager::pairingCompleted,
            this, &ComputerModel::handlePairingCompleted);
    connect(m_ComputerManager, &ComputerManager::hostRemoved,
            this, &ComputerModel::handleComputerRemoved);

    beginResetModel();
    m_Computers = m_ComputerManager->getComputers();
    LOG_INFO(QStringLiteral("[ComputerModel] 初始化完成，数量=%1").arg(m_Computers.size()));
    endResetModel();

}

QVariant ComputerModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Q_ASSERT(index.row() < m_Computers.count());

    NvComputer* computer = m_Computers[index.row()];
    QReadLocker lock(&computer->lock);

    switch (role) {
    case NameRole:
        return computer->name;
    case OnlineRole:
        return computer->state == NvComputer::CS_ONLINE;
    case PairedRole:
        return computer->pairState == NvComputer::PS_PAIRED;
    case BusyRole:
        return computer->currentGameId != 0;
    case WakeableRole:
        return !computer->macAddress.isEmpty();
    case StatusUnknownRole:
        return computer->state == NvComputer::CS_UNKNOWN;
    case ServerSupportedRole:
        return computer->isSupportedServerVersion;
    case DetailsRole: {
        QString state, pairState;

        switch (computer->state) {
        case NvComputer::CS_ONLINE:
            state = tr("Online");
            break;
        case NvComputer::CS_OFFLINE:
            state = tr("Offline");
            break;
        default:
            state = tr("Unknown");
            break;
        }

        switch (computer->pairState) {
        case NvComputer::PS_PAIRED:
            pairState = tr("Paired");
            break;
        case NvComputer::PS_NOT_PAIRED:
            pairState = tr("Unpaired");
            break;
        default:
            pairState = tr("Unknown");
            break;
        }

        return tr("Name: %1").arg(computer->name) + '\n' +
               tr("Status: %1").arg(state) + '\n' +
               tr("Active Address: %1").arg(computer->activeAddress.toString()) + '\n' +
               tr("UUID: %1").arg(computer->uuid) + '\n' +
               tr("Local Address: %1").arg(computer->localAddress.toString()) + '\n' +
               tr("Remote Address: %1").arg(computer->remoteAddress.toString()) + '\n' +
               tr("IPv6 Address: %1").arg(computer->ipv6Address.toString()) + '\n' +
               tr("Manual Address: %1").arg(computer->manualAddress.toString()) + '\n' +
               tr("MAC Address: %1").arg(computer->macAddress.isEmpty() ? tr("Unknown") : QString(computer->macAddress.toHex(':'))) + '\n' +
               tr("Pair State: %1").arg(pairState) + '\n' +
               tr("Running Game ID: %1").arg(computer->state == NvComputer::CS_ONLINE ? QString::number(computer->currentGameId) : tr("Unknown")) + '\n' +
               tr("HTTPS Port: %1").arg(computer->state == NvComputer::CS_ONLINE ? QString::number(computer->activeHttpsPort) : tr("Unknown"));
    }
    case StartedAtRole:
        return computer->orderStartedAt.isValid() ? QVariant(computer->orderStartedAt.toMSecsSinceEpoch()) : QVariant();
    case EndedAtRole:
        return computer->orderEndedAt.isValid() ? QVariant(computer->orderEndedAt.toMSecsSinceEpoch()) : QVariant();
    case BitrateRole:
        return computer->orderBitrate;
    case OrderStatusRole:
        return computer->orderStatus;
    case BillingTypeRole:
        return computer->orderBillingType;
    case OrderIdRole:
        return computer->orderId;
    case IsOrderDeviceRole:
        return m_ComputerManager->isOrderDevice(computer);
    case UuidRole:
        return computer->uuid;
    default:
        return QVariant();
    }
}

int ComputerModel::rowCount(const QModelIndex& parent) const
{
    // We should not return a count for valid index values,
    // only the parent (which will not have a "valid" index).
    if (parent.isValid()) {
        return 0;
    }

    return m_Computers.count();
}

QHash<int, QByteArray> ComputerModel::roleNames() const
{
    QHash<int, QByteArray> names;

    names[NameRole] = "name";
    names[OnlineRole] = "online";
    names[PairedRole] = "paired";
    names[BusyRole] = "busy";
    names[WakeableRole] = "wakeable";
    names[StatusUnknownRole] = "statusUnknown";
    names[ServerSupportedRole] = "serverSupported";
    names[DetailsRole] = "details";
    names[StartedAtRole] = "startedAt";
    names[EndedAtRole] = "endedAt";
    names[BitrateRole] = "bitrate";
    names[OrderStatusRole] = "status";
    names[BillingTypeRole] = "billingType";
    names[OrderIdRole] = "orderId";
    names[IsOrderDeviceRole] = "isOrderDevice";
    names[UuidRole] = "uuid";

    return names;
}

Session* ComputerModel::createSessionForCurrentGame(int computerIndex)
{
    Q_ASSERT(computerIndex < m_Computers.count());

    NvComputer* computer = m_Computers[computerIndex];

    // We must currently be streaming a game to use this function
    Q_ASSERT(computer->currentGameId != 0);

    for (NvApp& app : computer->appList) {
        if (app.id == computer->currentGameId) {
            return new Session(computer, app);
        }
    }

    // We have a current running app but it's not in our app list
    Q_ASSERT(false);
    return nullptr;
}

static int findDesktopAppIndex(NvComputer* computer)
{
    for (int i = 0; i < computer->appList.size(); i++) {
        const NvApp& app = computer->appList[i];
        QString nameLower = app.name.toLower();
        if (nameLower.contains(QStringLiteral("desktop")) ||
            nameLower.contains(QStringLiteral("\u684c\u9762"))) {
            return i;
        }
    }
    for (int i = 0; i < computer->appList.size(); i++) {
        if (computer->appList[i].directLaunch) {
            return i;
        }
    }
    return computer->appList.isEmpty() ? -1 : 0;
}

Session* ComputerModel::createDesktopSession(int computerIndex)
{
    Q_ASSERT(computerIndex < m_Computers.count());

    NvComputer* computer = m_Computers[computerIndex];
    int idx = findDesktopAppIndex(computer);
    if (idx < 0) {
        bool changed = false;
        if (m_ComputerManager->fetchAppListSync(computer, &changed)) {
            if (changed) {
                emit dataChanged(createIndex(computerIndex, 0), createIndex(computerIndex, 0));
            }
            idx = findDesktopAppIndex(computer);
        }
    }

    if (idx >= 0) {
        NvApp app = computer->appList[idx];
        return new Session(computer, app);
    }

    return nullptr;
}

QString ComputerModel::getDesktopAppName(int computerIndex)
{
    Q_ASSERT(computerIndex < m_Computers.count());

    NvComputer* computer = m_Computers[computerIndex];

    int idx = findDesktopAppIndex(computer);
    if (idx < 0) {
        if (m_ComputerManager->fetchAppListSync(computer)) {
            idx = findDesktopAppIndex(computer);
        }
    }

    if (idx >= 0) {
        return computer->appList[idx].name;
    }

    return QString();
}

void ComputerModel::deleteComputer(int computerIndex)
{
    if (computerIndex < 0 || computerIndex >= m_Computers.count()) {
        LOG_WARN(QStringLiteral("[ComputerModel] 删除主机失败，索引=%1 无效").arg(computerIndex));
        return;
    }

    beginRemoveRows(QModelIndex(), computerIndex, computerIndex);

    // m_Computer[computerIndex] will be deleted by this call
    m_ComputerManager->deleteHost(m_Computers[computerIndex]);

    // Remove the now invalid item
    m_Computers.removeAt(computerIndex);

    endRemoveRows();
}

class DeferredWakeHostTask : public QRunnable
{
public:
    DeferredWakeHostTask(NvComputer* computer)
        : m_Computer(computer) {}

    void run()
    {
        m_Computer->wake();
    }

private:
    NvComputer* m_Computer;
};

void ComputerModel::wakeComputer(int computerIndex)
{
    Q_ASSERT(computerIndex < m_Computers.count());

    DeferredWakeHostTask* wakeTask = new DeferredWakeHostTask(m_Computers[computerIndex]);
    QThreadPool::globalInstance()->start(wakeTask);
}

void ComputerModel::renameComputer(int computerIndex, QString name)
{
    Q_ASSERT(computerIndex < m_Computers.count());

    m_ComputerManager->renameHost(m_Computers[computerIndex], name);
}

QVariantMap ComputerModel::handlePcClicked(int computerIndex)
{
    QVariantMap result;
    if (computerIndex >= m_Computers.count())
        return result;

    NvComputer* computer = m_Computers[computerIndex];
    {
        QReadLocker lock(&computer->lock);
        if (computer->orderStatus != 0) {
            switch (computer->orderStatus) {
            case 1:
                result["error"] = tr("订单已结束");
                break;
            case 3:
                result["error"] = tr("金币不足");
                break;
            case 4:
                result["error"] = tr("包机到期");
                break;
            default:
                result["error"] = tr("订单状态异常");
                break;
            }
            return result;
        }
        if (computer->state != NvComputer::CS_ONLINE || computer->activeAddress.isNull()) {
            result["error"] = tr("PC is offline");
            return result;
        }

        if (!computer->isSupportedServerVersion) {
            result["error"] = tr("当前 GeForce Experience 版本不受支持。请更新 Moonlight。");
            return result;
        }

        if (computer->pairState == NvComputer::PS_PAIRED) {
            result["open"] = true;
            return result;
        }
    }

    QString pin = m_ComputerManager->generatePinString();
    m_ComputerManager->pairHost(computer, pin);

    // Show PIN only when it must be entered manually on the host
    bool isOrderDevice = m_ComputerManager->isOrderDevice(computer);
    bool autoPin = isOrderDevice ||
                   (computer->manualAddress.isNull() && !computer->remoteAddress.isNull());

    if (!autoPin) {
        // LAN or manually added device - display PIN to the user
        result["pin"] = pin;
    }

    return result;
}

void ComputerModel::checkoutComputer(int computerIndex)
{
    if (computerIndex < 0 || computerIndex >= m_Computers.count()) {
        LOG_WARN(QStringLiteral("[ComputerModel] 结账失败，索引=%1 无效").arg(computerIndex));
        return;
    }

    m_ComputerManager->closeOrder(m_Computers[computerIndex]);
}

int ComputerModel::findComputerIndex(const QString& uuid) const
{
    for (int i = 0; i < m_Computers.count(); ++i) {
        if (m_Computers[i]->uuid == uuid) {
            return i;
        }
    }
    return -1;
}

QString ComputerModel::generatePinString()
{
    return m_ComputerManager->generatePinString();
}

class DeferredTestConnectionTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    void run()
    {
        unsigned int portTestResult = LiTestClientConnectivity("qt.conntest.moonlight-stream.org", 443, ML_PORT_FLAG_ALL);
        if (portTestResult == ML_TEST_RESULT_INCONCLUSIVE) {
            emit connectionTestCompleted(-1, QString());
        }
        else {
            char blockedPorts[512];
            LiStringifyPortFlags(portTestResult, "\n", blockedPorts, sizeof(blockedPorts));
            emit connectionTestCompleted(portTestResult, QString(blockedPorts));
        }
    }

signals:
    void connectionTestCompleted(int result, QString blockedPorts);
};

void ComputerModel::testConnectionForComputer(int)
{
    DeferredTestConnectionTask* testConnectionTask = new DeferredTestConnectionTask();
    QObject::connect(testConnectionTask, &DeferredTestConnectionTask::connectionTestCompleted,
                     this, &ComputerModel::connectionTestCompleted);
    QThreadPool::globalInstance()->start(testConnectionTask);
}

void ComputerModel::pairComputer(int computerIndex, QString pin)
{
    Q_ASSERT(computerIndex < m_Computers.count());

    m_ComputerManager->pairHost(m_Computers[computerIndex], pin);
}

void ComputerModel::handlePairingCompleted(NvComputer*, QString error)
{
    emit pairingCompleted(error.isEmpty() ? QVariant() : error);
}

void ComputerModel::handleComputerStateChanged(NvComputer* computer)
{
    QVector<NvComputer*> newComputerList = m_ComputerManager->getComputers();

    // Reset the model if the structural layout of the list has changed
    if (m_Computers != newComputerList) {
        beginResetModel();
        m_Computers = newComputerList;
        endResetModel();
    }
    else {
        // Let the view know that this specific computer changed
        int index = m_Computers.indexOf(computer);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0));
    }
}

void ComputerModel::handleComputerRemoved(NvComputer*)
{
    beginResetModel();
    m_Computers = m_ComputerManager->getComputers();
    endResetModel();
}

#include "computermodel.moc"
