#pragma once

#include <any>
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "signaling.h"

struct UserInfo
{
    QString m_name;
    QDateTime m_refresh_time;
};

// todo: хранить историю сообщений
struct Message
{
    Message(bool a_sentToSender, const QDateTime &a_date, const QString &a_text) :
        m_sentToSender(a_sentToSender), m_date(a_date), m_text(a_text)
    {
    }

    bool m_sentToSender = false;
    QDateTime m_date;
    QString m_text;
};

class MessengerSignaling : public QObject
{
    Q_OBJECT

public:
    MessengerSignaling(std::shared_ptr<Signaling> a_signaling);

    QString getId() const;
    void setId(const QString &a_id);
    QString getName() const;
    void setName(const QString& a_name);
    void setOnline(bool a_online);
    bool userIsOnline(const QString &a_id);
    QString getUserName(const QString &a_id);
    QList<Message> getMessages(const QString &a_id);
    void sendMessage(const QString &a_receiver, const QString &a_text);
    bool isTyping(const QString &a_receiver);
    void sendTyping(const QString &a_receiver, bool a_typing);

signals:
    void userAdded(QString a_id, QString a_name);
    void userRemoved(QString a_id);
    void messageReceived(QString a_sender, QDateTime a_date, QString a_text);
    void typing(QString a_sender, bool a_typing);

private slots:
    void sendUserInfo();
    void onSignalReceived(QString a_signal, QVariant a_value);
    void autoKickUser();

private:
    QString getSignalName(const QString &a_prefix, const QString &a_id);
    template<typename T> bool tryHandleSignal(const QString &a_signal, const QVariant &a_value);
    template<typename T> void handleSignal(const T &a_signal);

    std::shared_ptr<Signaling> m_signaling;
    QString m_id;
    QString m_name;
    bool m_online = true;
    QMap<QString, QList<Message>> m_history;
    QTimer m_sendTimer; // таймер для sendUserInfo
    QMap<QString, bool> m_typing;
    std::map<QString, UserInfo> m_users;
    QTimer m_kickTimer; // таймер для очистки m_users
};
