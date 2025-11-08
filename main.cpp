#include <QApplication>
#include "user_list_widget.h"
#include "signaling_facade.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("MSIprog");
    QCoreApplication::setApplicationName("Messenger");

    SignalingFacade signalingFacade;
    if (signalingFacade.getSignaling() == nullptr)
        return 1;
    auto messengerSignaling = std::make_shared<MessengerSignaling>(signalingFacade.getSignaling());

    UserListWidget w(messengerSignaling);
    w.show();
    return a.exec();
}
