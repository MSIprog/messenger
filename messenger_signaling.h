#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "signaling.h"

struct UserInfo
{
    QString m_name;
    QDateTime m_refresh_time;
};

struct Message
{
    static QString encode(const QString &a_string);
    static QString decode(const QString &a_string);
    static Message fromString(const QString &a_string, bool *a_ok = nullptr);

    QString toString() const;

    inline static const QString m_dateTimeFormat{ "dd.MM.yyyy hh:mm:ss" };

    bool m_sentToSender = false;
    QDateTime m_date;
    QString m_text;
};

#define ATTRIBUTE(type,name) \
   type get_##name() const { return m_container[#name].value<type>(); }\
   void set_##name(const type &a_value) { m_container[#name] = a_value; }

struct AttributeContainer
{
    AttributeContainer() {}

    explicit AttributeContainer(const QVariant &a_value)
    {
        m_container = a_value.toMap();
    }

    QVariant toQVariant() const
    {
        return m_container;
    }

    QMap<QString, QVariant> m_container;
};

class MessengerSignaling : public QObject
{
    Q_OBJECT

public:
    MessengerSignaling(std::shared_ptr<Signaling> a_signaling);

    QString getId() const;
    void setId(const QString &a_id);
    QString getName() const;
    void setName(const QString &a_name);
    void setOnline(bool a_online);
    bool userIsOnline(const QString &a_id);
    QString getUserName(const QString &a_id);
    QList<Message> getMessages(const QString &a_id);
    void sendMessage(const QString &a_receiver, const QString &a_text);
    bool isTyping(const QString &a_receiver);
    void sendTyping(const QString &a_receiver, bool a_typing);

signals:
    void idChanged(QString a_id);
    void userAdded(QString a_id, QString a_name);
    void userRenamed(QString a_id, QString a_name);
    void userRemoved(QString a_id);
    void messageReceived(QString a_sender, QDateTime a_date, QString a_text);
    void typing(QString a_sender, bool a_typing);

private slots:
    void sendUserInfo();
    void onSignalReceived(QString a_signal, QVariant a_value);
    void autoKickUser();

private:
    static QString getSignalName(const QString &a_prefix, const QString &a_id);

    template<typename T> bool tryHandleSignal(const QString &a_signal, const QVariant &a_value);
    template<typename T> void handleSignal(const T &a_signal);
    void addMessageToHistory(const QString &a_id, const Message &a_message);

    std::shared_ptr<Signaling> m_signaling;
    QString m_id;
    QString m_name;
    bool m_online = true;
    std::map<QString, QList<Message>> m_history;
    QTimer m_sendTimer; // таймер для sendUserInfo
    QMap<QString, bool> m_typing;
    std::map<QString, UserInfo> m_users;
    QTimer m_kickTimer; // таймер для очистки m_users
};
