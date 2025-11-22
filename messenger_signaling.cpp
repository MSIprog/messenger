#include <QUuid>
#include "messenger_signaling.h"

//-------------------------------------------------------------------------------------------------
struct UserInfoSignal : AttributeContainer
{
    ATTRIBUTE(QString, id);
    ATTRIBUTE(QString, name);
    ATTRIBUTE(bool, online);

    UserInfoSignal(const QString &a_id, const QString &a_name, bool a_online)
    {
        set_id(a_id);
        set_name(a_name);
        set_online(a_online);
    }

    explicit UserInfoSignal(const QVariant &a_value) : AttributeContainer(a_value) {}

    static constexpr char g_signalName[]{ "UserInfo" };
};

//-------------------------------------------------------------------------------------------------
struct MessageSignal : AttributeContainer
{
    ATTRIBUTE(QString, sender);
    ATTRIBUTE(QString, text);

    MessageSignal(const QString &a_sender, const QString &a_text)
    {
        set_sender(a_sender);
        set_text(a_text);
    }

    explicit MessageSignal(const QVariant &a_value) : AttributeContainer(a_value) {}

    static constexpr char g_signalName[]{ "Message" };
};

//-------------------------------------------------------------------------------------------------
struct TypingSignal : AttributeContainer
{
    ATTRIBUTE(QString, sender);
    ATTRIBUTE(bool, typing);

    TypingSignal(const QString &a_sender, bool a_typing)
    {
        set_sender(a_sender);
        set_typing(a_typing);
    }

    explicit TypingSignal(const QVariant &a_value) : AttributeContainer(a_value) {}

    static constexpr char g_signalName[]{ "Typing" };
};

//-------------------------------------------------------------------------------------------------
MessengerSignaling::MessengerSignaling(std::shared_ptr<Signaling> a_signaling)
{
    m_signaling = a_signaling;
    connect(m_signaling.get(), &Signaling::signalReceived, this, &MessengerSignaling::onSignalReceived);
    m_signaling->subscribe(UserInfoSignal::g_signalName);
    connect(&m_sendTimer, &QTimer::timeout, this, &MessengerSignaling::sendUserInfo);
    connect(&m_kickTimer, &QTimer::timeout, this, &MessengerSignaling::autoKickUser);
    m_kickTimer.start(1000);
}

QString MessengerSignaling::getId() const
{
    return m_id;
}

void MessengerSignaling::setId(const QString &a_id)
{
    m_signaling->unsubscribe(getSignalName(MessageSignal::g_signalName, m_id));
    m_signaling->unsubscribe(getSignalName(TypingSignal::g_signalName, m_id));
    m_id = a_id;
    m_signaling->subscribe(getSignalName(MessageSignal::g_signalName, m_id));
    m_signaling->subscribe(getSignalName(TypingSignal::g_signalName, m_id));
    m_sendTimer.start(1000);
}

QString MessengerSignaling::getName() const
{
    return m_name;
}

void MessengerSignaling::setName(const QString &a_name)
{
    m_name = a_name;
}

void MessengerSignaling::setOnline(bool a_online)
{
    m_online = a_online;
    if (m_online)
        m_sendTimer.start(1000);
}

bool MessengerSignaling::userIsOnline(const QString &a_id)
{
    return m_users.find(a_id) != m_users.end();
}

QString MessengerSignaling::getUserName(const QString &a_id)
{
    if (a_id == m_id)
        return m_name;
    auto it = m_users.find(a_id);
    if (it == m_users.end())
        return QString();
    return it->second.m_name;
}

QList<Message> MessengerSignaling::getMessages(const QString &a_sender)
{
    return m_history[a_sender];
}

void MessengerSignaling::sendMessage(const QString &a_receiver, const QString &a_text)
{
    m_signaling->sendSignal(getSignalName(MessageSignal::g_signalName, a_receiver), MessageSignal(m_id, a_text).toQVariant());
    m_history[a_receiver].append(Message(false, QDateTime::currentDateTime(), a_text));
}

bool MessengerSignaling::isTyping(const QString &a_sender)
{
    if (!m_typing.contains(a_sender))
        return false;
    return m_typing[a_sender];
}

void MessengerSignaling::sendTyping(const QString &a_receiver, bool a_typing)
{
    m_signaling->sendSignal(getSignalName(TypingSignal::g_signalName, a_receiver), TypingSignal(m_id, a_typing).toQVariant());
}

// private slots:
void MessengerSignaling::sendUserInfo()
{
    m_signaling->sendSignal(UserInfoSignal::g_signalName, UserInfoSignal(m_id, m_name, m_online).toQVariant());
}

void MessengerSignaling::onSignalReceived(QString a_signal, QVariant a_value)
{
    tryHandleSignal<UserInfoSignal>(a_signal, a_value) ||
        tryHandleSignal<MessageSignal>(a_signal, a_value) ||
        tryHandleSignal<TypingSignal>(a_signal, a_value);
}

void MessengerSignaling::autoKickUser()
{
    std::vector<QString> removingIds;
    for (auto &user : m_users)
        if (user.second.m_refresh_time.secsTo(QDateTime::currentDateTime()) >= 2)
            removingIds.push_back(user.first);
    for (auto &id : removingIds)
    {
        m_users.erase(id);
        emit userRemoved(id);
    }
}

// private:
QString MessengerSignaling::getSignalName(const QString &a_prefix, const QString &a_id)
{
    return QString("%1_%2").arg(a_prefix).arg(a_id);
}

template<typename T> bool MessengerSignaling::tryHandleSignal(const QString &a_signal, const QVariant &a_value)
{
    if (a_signal.left(QString(T::g_signalName).length()) != T::g_signalName)
        return false;
    handleSignal<T>(T(a_value));
    return true;
}

template<typename T> void MessengerSignaling::handleSignal(const T &)
{
    throw std::exception("MessengerSignaling.handleSignal: undefined signal handler");
}

template<> void MessengerSignaling::handleSignal(const UserInfoSignal &a_data)
{
    auto id = a_data.get_id();
    if (id == m_id)
    {
        auto thisId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        setId(thisId);
        emit idChanged(thisId);
    }

    if (!a_data.get_online())
    {
        m_users.erase(id);
        emit userRemoved(id);
        return;
    }
    auto it = m_users.find(id);
    if (it != m_users.end())
    {
        it->second.m_refresh_time = QDateTime::currentDateTime();
        auto name = a_data.get_name();
        if (it->second.m_name != name)
            emit userRenamed(id, name);
        return;
    }
    UserInfo user;
    user.m_name = a_data.get_name();
    user.m_refresh_time = QDateTime::currentDateTime();
    m_users[id] = user;
    emit userAdded(id, user.m_name);
}

template<> void MessengerSignaling::handleSignal(const MessageSignal &a_data)
{
    auto date = QDateTime::currentDateTime();
    m_history[a_data.get_sender()].append(Message(true, date, a_data.get_text()));
    emit messageReceived(a_data.get_sender(), date, a_data.get_text());
}

template<> void MessengerSignaling::handleSignal(const TypingSignal &a_data)
{
    m_typing[a_data.get_sender()] = a_data.get_typing();
    emit typing(a_data.get_sender(), a_data.get_typing());
}
