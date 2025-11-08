#include <QNetworkDatagram>
#include <QNetworkInterface>
#include "detection_server.h"

DetectionServer::DetectionServer(QObject *a_parent)
    : QObject{a_parent}
{
}

bool DetectionServer::start(quint16 a_signalPort, quint16 a_detectionPort)
{
    m_signalPort = a_signalPort;
    m_listenSocket = std::make_unique<QUdpSocket>();
    if (!m_listenSocket->bind(QHostAddress::AnyIPv4, a_detectionPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
    {
        m_listenSocket = nullptr;
        return false;
    }
    connect(m_listenSocket.get(), &QUdpSocket::readyRead, this, &DetectionServer::readPendingDatagrams);
    return true;
}

void DetectionServer::readPendingDatagrams()
{
    while (m_listenSocket->hasPendingDatagrams())
    {
        auto size = m_listenSocket->pendingDatagramSize();
        if (size != 2)
            continue;
        QByteArray data(size, Qt::Uninitialized);
        QHostAddress address;
        if (m_listenSocket->readDatagram(data.data(), size, &address) != size)
            continue;
        auto localAddresses = QNetworkInterface::allAddresses();
        bool addressIsLocal = std::find(localAddresses.begin(), localAddresses.end(), address) != localAddresses.end();
        auto port = *(quint16*)data.data();
        if (addressIsLocal && port == m_signalPort)
            continue;
        auto it = std::find(m_peers.begin(), m_peers.end(), std::pair<QHostAddress, quint16>{address, port});
        if (it != m_peers.end())
            continue;
        m_peers.push_back({address, port});
        emit peerFound(address, port);
    }
}
