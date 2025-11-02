#ifndef SIGNALING_H
#define SIGNALING_H

#include <set>
#include <QObject>
#include <QVariant>
#include <QHostAddress>
#include <QTcpServer>
#include "block_queue.h"

class QTcpSocket;

class Signaling : public QObject
{
    Q_OBJECT

public:
    bool start();
    quint16 getPort();
    void sendSignal(QString a_name, QVariant a_value);
    void subscribe(QString a_name);
    void unsubscribe(QString a_name);

public slots:
    void addPeer(QHostAddress a_address, quint16 a_port);

signals:
    void signalReceived(QString a_name, QVariant a_value);

private slots:
    void onClientConencted();
    void onConnectedToHost();
    void onPeerDisconnected();
    void onDataReceived();

private:
    static QHostAddress getThisSubnetAddress(const QHostAddress &a_anotherAddress);

    void addSocket(QTcpSocket *a_socket);

    std::unique_ptr<QTcpServer> m_server;
    std::set<QString> m_subscriptions;
    std::set<QTcpSocket *> m_peers;
    std::map<QTcpSocket *, MessageQueue> m_socketData;
    std::map<QTcpSocket *, QSet<QString>> m_peerSubscriptions;
};

#endif // SIGNALING_H
