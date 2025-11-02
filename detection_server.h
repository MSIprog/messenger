#ifndef LISTEN_SERVER_H
#define LISTEN_SERVER_H

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>

class DetectionServer : public QObject
{
    Q_OBJECT

public:
    explicit DetectionServer(QObject *a_parent = nullptr);

    bool start(quint16 a_signalPort, quint16 a_detectionPort = 1234);

signals:
    void peerFound(QHostAddress a_address, quint16 a_port);

private:
    void readPendingDatagrams();

    quint16 m_signalPort = 0;
    std::unique_ptr<QUdpSocket> m_listenSocket;
    std::vector<std::pair<QHostAddress, quint16>> m_peers;
};

#endif // LISTEN_SERVER_H
