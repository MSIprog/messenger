#include <QTcpSocket>
#include <QBuffer>
#include <QNetworkInterface>
#include "signaling.h"

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

void Signaling::sendSignal(QString a_name, QVariant a_value)
{
    std::vector<QTcpSocket *> subscribedPeers;
    for (auto& peer : m_peerSubscriptions)
        if (peer.second.contains(a_name))
            subscribedPeers.push_back(peer.first);
    if (subscribedPeers.empty())
        return;
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream << (char)0 << a_name << a_value;
    auto data = MessageQueue::createMessage(buffer.buffer());
    for (auto peer : subscribedPeers)
    {
        auto bytesWritten = peer->write(data);
        Q_ASSERT(bytesWritten == data.size());
    }
}

void Signaling::subscribe(QString a_name)
{
    m_subscriptions.insert(a_name);
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream << (char)1 << a_name;
    auto data = MessageQueue::createMessage(buffer.buffer());
    for (auto peer : m_peers)
    {
        auto bytesWritten = peer->write(data);
        Q_ASSERT(bytesWritten == data.size());
    }
}

void Signaling::unsubscribe(QString a_name)
{
    m_subscriptions.erase(a_name);
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream << (char)2 << a_name;
    auto data = MessageQueue::createMessage(buffer.buffer());
    for (auto peer : m_peers)
    {
        auto bytesWritten = peer->write(data);
        Q_ASSERT(bytesWritten == data.size());
    }
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
    auto& data = m_socketData[peer];
    data.appendRawData(peer->readAll());
    while (data.messageIsReady())
    {
        QByteArray message = data.takeMessage();
        QDataStream stream(message);
        char operationCode;
        QString name;
        stream >> operationCode >> name;
        /*if (stream.status() != QDataStream::Ok)
            continue;*/
        if (operationCode == 0)
        {
            QVariant value;
            stream >> value;
            /*if (stream.status() != QDataStream::Ok)
                continue;*/
            emit signalReceived(name, value);
        }
        else if (operationCode == 1)
            m_peerSubscriptions[peer].insert(name);
        else if (operationCode == 2)
            m_peerSubscriptions[peer].remove(name);
    }
}

QHostAddress Signaling::getThisSubnetAddress(const QHostAddress &a_anotherAddress)
{
    for (auto& interface : QNetworkInterface::allInterfaces())
        for (auto& address : interface.addressEntries())
            if (address.ip().isInSubnet(a_anotherAddress, address.prefixLength()))
                return address.ip();
    return QHostAddress();
}

void Signaling::addSocket(QTcpSocket *a_peer)
{
    m_peers.insert(a_peer);
    connect(a_peer, &QTcpSocket::disconnected, this, &Signaling::onPeerDisconnected);
    connect(a_peer, &QTcpSocket::readyRead, this, &Signaling::onDataReceived);

    for (auto& signal : m_subscriptions)
    {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QDataStream stream(&buffer);
        stream << (char)1 << signal;
        auto data = MessageQueue::createMessage(buffer.buffer());
        auto bytesWritten = a_peer->write(data);
        Q_ASSERT(bytesWritten == data.size());
    }
}
