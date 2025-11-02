#include <QUdpSocket>
#include "seeker_client.h"

SeekerClient::SeekerClient()
{
}

void SeekerClient::start(quint16 a_signalPort, quint16 a_detectionPort)
{
    m_signalPort = a_signalPort;
    m_detectionPort = a_detectionPort;
    connect(&m_timer, &QTimer::timeout, this, &SeekerClient::seek);
    m_timer.start(1000);
}

void SeekerClient::seek()
{
    QUdpSocket socket;
    QByteArray data((const char*)&m_signalPort, sizeof(quint16));
    socket.writeDatagram(data, QHostAddress::Broadcast, m_detectionPort);
}
