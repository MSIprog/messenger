#pragma once

#include <any>
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "signaling.h"

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

    QString getName() const;
    void setName(const QString& a_name);
    void setOnline(bool a_online);
    QList<Message> getMessages(const QString &a_sender);
    void sendMessage(const QString& a_text, const QString& a_receiver);
    bool isTyping(const QString &a_sender);
    void sendTyping(const QString &a_receiver, bool a_typing);

signals:
    void userInfoReceived(QString a_name, bool a_online);
    void messageReceived(QString a_sender, QDateTime a_date, QString a_text);
    void typing(QString a_sender, bool a_typing);

private slots:
    void sendUserInfo();
    void onSignalReceived(QString a_signal, QVariant a_value);

private:
    template<typename T> void sendSignal(const T &a_signal);
    template<typename T> bool tryHandleSignal(const QString &a_signal, const QVariant &a_value);
    template<typename T> void handleSignal(const T &a_signal);

    std::shared_ptr<Signaling> m_signaling;
    QString m_name;
    bool m_online = true;
    QMap<QString, QList<Message>> m_history;
    QTimer m_timer; // таймер для sendUserInfo
    QMap<QString, bool> m_typing;
};
