#ifndef USER_LIST_WIDGET_H
#define USER_LIST_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QMenu>
#include <QListWidgetItem>
#include "message_form.h"
#include "messenger_signaling.h"

namespace Ui
{
    class UserListWidget;
}

class UserListWidget : public QWidget
{
    Q_OBJECT

public:
    UserListWidget(std::shared_ptr<MessengerSignaling> a_signaling, QWidget *a_parent = nullptr);
    ~UserListWidget();

private slots:
    void logon();
    void setOnline();
    void setOffline();
    void showStateMenu();
    void showDialog(QListWidgetItem *a_item);
    void changeIcons();
    void addUser(QString a_id, QString a_name);
    void removeUser(QString a_id);

private:
    int getUserIndex(const QString& a_name);
    bool hasUnreadMessages();
    bool hasUnreadMessages(const QString& a_name);

    Ui::UserListWidget *m_ui = nullptr;
    std::shared_ptr<MessengerSignaling> m_signaling;
    QMenu *m_states = nullptr;
    MessageForm *m_messageForm = nullptr;
    QTimer m_blinkTimer;
    bool m_blinkState = false;
};

#endif // USER_LIST_WIDGET_H
