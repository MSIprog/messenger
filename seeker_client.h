#ifndef SEEKER_CLIENT_H
#define SEEKER_CLIENT_H

#include <QObject>
#include <QTimer>

class SeekerClient : public QObject
{
    Q_OBJECT

public:
    SeekerClient();

    void start(quint16 a_signalPort, quint16 a_detectionPort = 1234);

private slots:
    void seek();

private:
    quint16 m_signalPort = 0;
    quint16 m_detectionPort = 1234;
    QTimer m_timer;
};

#endif // SEEKER_CLIENT_H
