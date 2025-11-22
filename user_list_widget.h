#ifndef USER_LIST_WIDGET_H
#define USER_LIST_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QMenu>
#include <QListWidgetItem>
#include "message_form.h"
#include "messenger_signaling.h"
#include "file_form.h"
#include "file_signaling.h"

namespace Ui
{
    class UserListWidget;
}

class UserListWidget : public QWidget
{
    Q_OBJECT

public:
    UserListWidget(std::shared_ptr<MessengerSignaling> a_signaling, std::shared_ptr<FileSignaling> a_fileSignaling, QWidget *a_parent = nullptr);
    ~UserListWidget();

private slots:
    void logon();
    void setOnline();
    void setOffline();
    void showStateMenu();
    void showDialog(QListWidgetItem *a_item);
    void showActionsMenu(const QPoint &a_pos);
    void sendMessage();
    void sendFile();
    void changeIcons();
    void changeId(QString a_id);
    void addUser(QString a_id, QString a_name);
    void renameUser(QString a_id, QString a_name);
    void removeUser(QString a_id);

private:
    int getUserIndex(const QString &a_name);

    Ui::UserListWidget *m_ui = nullptr;
    std::shared_ptr<MessengerSignaling> m_signaling;
    std::shared_ptr<FileSignaling> m_fileSignaling;
    QMenu *m_states = nullptr;
    QMenu *m_actions = nullptr;
    MessageForm *m_messageForm = nullptr;
    QTimer m_blinkTimer;
    bool m_blinkState = false;
    FileForm *m_fileForm = nullptr;
};

#endif // USER_LIST_WIDGET_H
