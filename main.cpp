#include <QApplication>
#include "user_list_widget.h"
#include "detection_server.h"
#include "seeker_client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("MSIprog");
    QCoreApplication::setApplicationName("Messenger");

    auto signaling = std::make_shared<Signaling>();
    if (!signaling->start())
        return 1;

    DetectionServer detectionServer;
    if (!detectionServer.start(signaling->getPort()))
        return 1;
    QObject::connect(&detectionServer, &DetectionServer::peerFound, signaling.get(), &Signaling::addPeer);

    SeekerClient seekerClient;
    seekerClient.start(signaling->getPort());

    auto messengerSignaling = std::make_shared<MessengerSignaling>(signaling);

    UserListWidget w(messengerSignaling);
    w.show();
    return a.exec();
}
