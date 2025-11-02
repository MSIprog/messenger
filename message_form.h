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

    void AddDialog(const QString &a_name);
    void ValidateDialog(const QString &a_name);
    void InvalidateDialog(const QString &a_name);
    bool HasUnreadMessages();
    bool HasUnreadMessages(const QString &a_name);

signals:
    void messagesRead(const QString &a_sender);

private slots:
    void showHistory(int a_tabIndex);
    void closeTab(int a_tabIndex);
    void send(const QString &a_text);
    void onMessageReceived(QString a_sender, QDateTime a_date, QString a_text);
    void blinkMessages();

private:
    void changeEvent(QEvent *a_event) override;
    void onMessagesRead(const QString &a_sender);
    void receiveHistory(const QString &a_name);
    void addToHistory(const QString &a_name, const QString &a_text);
    void appendHTML(const QString &a_text);
    static QString formatMessage(bool a_sentToSender, const QString &a_sender, const QDateTime &date, const QString &a_text);
    int getTabIndex(const QString &a_name);

    Ui::MessageForm *m_ui = nullptr;
    std::shared_ptr<MessengerSignaling> m_signaling;
    QMap<QString, QString> m_history; // история сообщений в форматированном виде
    std::set<QString> m_onlineUsers; // todo: move to MessengerSignaling
    std::set<QString> m_unreadSenders; // отправители, сообщения которых не прочитаны пользователем
    QTimer m_blinkTimer;
    bool m_blinkState = false;
};

#endif // MESSAGE_FORM_H
