#ifndef MESSAGE_FORM_H
#define MESSAGE_FORM_H

#include <QWidget>
#include <QMap>
#include "messenger_signaling.h"

namespace Ui
{
    class MessageForm;
}

class MessageForm : public QWidget
{
    Q_OBJECT

public:
    MessageForm(std::shared_ptr<MessengerSignaling> a_signaling, QWidget *a_parent = nullptr);
    ~MessageForm();

    void addDialog(const QString &a_id);
    bool hasUnreadMessages();
    bool hasUnreadMessages(const QString &a_id);

signals:
    void messagesRead(const QString &a_sender);

private slots:
    void addUser(QString a_id, QString a_name);
    void renameUser(QString a_id, QString a_name);
    void removeUser(const QString &a_id);
    void showHistory(int a_tabIndex);
    void closeTab(int a_tabIndex);
    void sendText(const QString &a_text);
    void sendTyping(bool a_typing);
    void onMessageReceived(QString a_sender, QDateTime a_date, QString a_text);
    void changeIcons();

private:
    void changeEvent(QEvent *a_event) override;
    void onMessagesRead(const QString &a_sender);
    void receiveHistory(const QString &a_id);
    void appendHTML(const QString &a_text);
    static QString formatMessage(bool a_sentToSender, const QString &a_name, const QDateTime &date, const QString &a_text);
    int getTabIndex(const QString &a_id);
    QString getUserId(int a_tabIndex);
    QString getCurrentUserId();

    Ui::MessageForm *m_ui = nullptr;
    std::shared_ptr<MessengerSignaling> m_signaling;
    QMap<QString, QString> m_history; // история сообщений в форматированном виде
    std::set<QString> m_unreadSenders; // отправители, сообщения которых не прочитаны пользователем
    QTimer m_blinkTimer;
    bool m_blinkState = false;
};

#endif // MESSAGE_FORM_H
