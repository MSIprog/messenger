#include <QTcpSocket>
#include <QBuffer>
#include <QNetworkInterface>
#include "signaling.h"

struct DataSignal
{
    explicit DataSignal(const QString &a_name, const QVariant &a_value)
    {
        m_name = a_name;
        m_value = a_value;
    }

    explicit DataSignal(QDataStream &a_stream)
    {
        a_stream >> m_name >> m_value;
    }

    void toQDataStream(QDataStream &a_stream) const
    {
        a_stream << m_name << m_value;
    }

    static constexpr char g_signalCode = 0;
    QString m_name;
    QVariant m_value;
};

//-------------------------------------------------------------------------------------------------
struct SubscribeSignal
{
    explicit SubscribeSignal(const QString &a_name)
    {
        m_name = a_name;
    }

    explicit SubscribeSignal(QDataStream &a_stream)
    {
        a_stream >> m_name;
    }

    void toQDataStream(QDataStream &a_stream) const
    {
        a_stream << m_name;
    }

    static constexpr char g_signalCode = 1;
    QString m_name;
};

//-------------------------------------------------------------------------------------------------
struct UnsubscribeSignal
{
    explicit UnsubscribeSignal(const QString &a_name)
    {
        m_name = a_name;
    }

    explicit UnsubscribeSignal(QDataStream &a_stream)
    {
        a_stream >> m_name;
    }

    void toQDataStream(QDataStream &a_stream) const
    {
        a_stream << m_name;
    }

    static constexpr char g_signalCode = 2;
    QString m_name;
};

//-------------------------------------------------------------------------------------------------
bool Signaling::start()
{
    m_server = std::make_unique<QTcpServer>();
    if (!m_server->listen(QHostAddress::AnyIPv4))
        return false;
    connect(m_server.get(), &QTcpServer::newConnection, this, &Signaling::onClientConencted);
    return true;
}

quint16 Signaling::getPort()
{
    return m_server->serverPort();
}

void Signaling::sendSignal(const QString &a_name, const QVariant &a_value)
{
    std::vector<QTcpSocket *> subscribedPeers;
    for (auto& peer : m_peerSubscriptions)
        if (peer.second.contains(a_name))
            subscribedPeers.push_back(peer.first);
    if (subscribedPeers.empty())
        return;
    auto data = signalToByteArray(DataSignal(a_name, a_value));
    for (auto peer : subscribedPeers)
        peer->write(data);
}

void Signaling::subscribe(const QString &a_name)
{
    m_subscriptions.insert(a_name);
    auto data = signalToByteArray(SubscribeSignal(a_name));
    for (auto peer : m_peers)
        peer->write(data);
}

void Signaling::unsubscribe(const QString &a_name)
{
    m_subscriptions.erase(a_name);
    auto data = signalToByteArray(UnsubscribeSignal(a_name));
    for (auto peer : m_peers)
        peer->write(data);
}

void Signaling::addPeer(QHostAddress a_address, quint16 a_port)
{
    // исключаем дублирующее соединение двух узлов:
    // только узел с меньшим адресом (а при равенстве адресов - с меньшим портом) будет подключаться как клиент
    auto thisAddress = getThisSubnetAddress(a_address);
    auto thisAddressString = thisAddress.toString();
    auto anotherAddressString = a_address.toString();
    if (thisAddress != QHostAddress() && thisAddressString > anotherAddressString)
        return;
    if (thisAddressString == anotherAddressString && m_server->serverPort() > a_port)
        return;

    auto socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Signaling::onConnectedToHost);
    socket->connectToHost(a_address, a_port);
}

void Signaling::onClientConencted()
{
    while (m_server->hasPendingConnections())
        addSocket(m_server->nextPendingConnection());
}

void Signaling::onConnectedToHost()
{
    auto peer = qobject_cast<QTcpSocket *>(sender());
    if (peer == nullptr)
        return;
    addSocket(peer);
}

void Signaling::onPeerDisconnected()
{
    auto peer = qobject_cast<QTcpSocket *>(sender());
    if (peer == nullptr)
        return;
    m_peers.erase(peer);
    m_peerSubscriptions.erase(peer);
    peer->deleteLater();
}

void Signaling::onDataReceived()
{
    auto peer = qobject_cast<QTcpSocket *>(sender());
    if (peer == nullptr)
        return;
    auto &data = m_socketData[peer];
    data.appendRawData(peer->readAll());
    while (data.messageIsReady())
    {
        QByteArray message = data.takeMessage();
        QDataStream stream(message);
        char code;
        stream >> code;
        tryHandleSignal<DataSignal>(peer, code, stream) ||
            tryHandleSignal<SubscribeSignal>(peer, code, stream) ||
            tryHandleSignal<UnsubscribeSignal>(peer, code, stream);
    }
}

QHostAddress Signaling::getThisSubnetAddress(const QHostAddress &a_anotherAddress)
{
    for (auto &interface : QNetworkInterface::allInterfaces())
        for (auto &address : interface.addressEntries())
            if (address.ip().isInSubnet(a_anotherAddress, address.prefixLength()))
                return address.ip();
    return QHostAddress();
}

void Signaling::addSocket(QTcpSocket *a_peer)
{
    m_peers.insert(a_peer);
    connect(a_peer, &QTcpSocket::disconnected, this, &Signaling::onPeerDisconnected);
    connect(a_peer, &QTcpSocket::readyRead, this, &Signaling::onDataReceived);

    for (auto &signal : m_subscriptions)
        a_peer->write(signalToByteArray(SubscribeSignal(signal)));
}

template<typename T> QByteArray Signaling::signalToByteArray(const T &a_signal)
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream << T::g_signalCode;
    a_signal.toQDataStream(stream);
    return MessageQueue::createMessage(buffer.buffer());
}

template<typename T> bool Signaling::tryHandleSignal(QTcpSocket *a_peer, char a_code, QDataStream &a_stream)
{
    if (a_code != T::g_signalCode)
        return false;
    handleSignal<T>(a_peer, T(a_stream));
    return true;
}

template<typename T> void Signaling::handleSignal(QTcpSocket *, const T &)
{
    throw std::exception("Signaling.handleSignal: undefined signal handler");
}

template<> void Signaling::handleSignal(QTcpSocket *, const DataSignal &a_data)
{
    emit signalReceived(a_data.m_name, a_data.m_value);
}

template<> void Signaling::handleSignal(QTcpSocket *a_peer, const SubscribeSignal &a_data)
{
    m_peerSubscriptions[a_peer].insert(a_data.m_name);
}

template<> void Signaling::handleSignal(QTcpSocket *a_peer, const UnsubscribeSignal &a_data)
{
    m_peerSubscriptions[a_peer].remove(a_data.m_name);
}
