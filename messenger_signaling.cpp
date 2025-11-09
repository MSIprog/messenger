#include "messenger_signaling.h"

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

//-------------------------------------------------------------------------------------------------
struct UserInfoSignal : AttributeContainer
{
    ATTRIBUTE(QString, name);
    ATTRIBUTE(bool, online);

    UserInfoSignal(const QString &a_name, bool a_online)
    {
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
    ATTRIBUTE(QString, receiver);
    ATTRIBUTE(QString, text);

    MessageSignal(const QString &a_sender, const QString &a_receiver, const QString &a_text)
    {
        set_sender(a_sender);
        set_receiver(a_receiver);
        set_text(a_text);
    }

    explicit MessageSignal(const QVariant &a_value) : AttributeContainer(a_value) {}

    static constexpr char g_signalName[]{ "Message" };
};

//-------------------------------------------------------------------------------------------------
struct TypingSignal : AttributeContainer
{
    ATTRIBUTE(QString, sender);
    ATTRIBUTE(QString, receiver);
    ATTRIBUTE(bool, typing);

    TypingSignal(const QString &a_sender, const QString &a_receiver, bool a_typing)
    {
        set_sender(a_sender);
        set_receiver(a_receiver);
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
    m_signaling->subscribe(MessageSignal::g_signalName);
    m_signaling->subscribe(TypingSignal::g_signalName);
    connect(&m_timer, &QTimer::timeout, this, &MessengerSignaling::sendUserInfo);
}

QString MessengerSignaling::getName() const
{
    return m_name;
}

void MessengerSignaling::setName(const QString &a_name)
{
    m_name = a_name;
    m_timer.start(1000);
}

void MessengerSignaling::setOnline(bool a_online)
{
    m_online = a_online;
    if (m_online)
        m_timer.start(1000);
}

QList<Message> MessengerSignaling::getMessages(const QString &a_sender)
{
    return m_history[a_sender];
}

void MessengerSignaling::sendMessage(const QString &a_text, const QString &a_receiver)
{
    sendSignal(MessageSignal(m_name, a_receiver, a_text));
    m_history[m_name].append(Message(false, QDateTime::currentDateTime(), a_text));
}

bool MessengerSignaling::isTyping(const QString &a_sender)
{
    if (!m_typing.contains(a_sender))
        return false;
    return m_typing[a_sender];
}

void MessengerSignaling::sendTyping(const QString &a_receiver, bool a_typing)
{
    sendSignal(TypingSignal(m_name, a_receiver, a_typing));
}

void MessengerSignaling::sendUserInfo()
{
    sendSignal(UserInfoSignal(m_name, m_online));
}

void MessengerSignaling::onSignalReceived(QString a_signal, QVariant a_value)
{
    tryHandleSignal<UserInfoSignal>(a_signal, a_value) ||
        tryHandleSignal<MessageSignal>(a_signal, a_value) ||
        tryHandleSignal<TypingSignal>(a_signal, a_value);
}

template<typename T> void MessengerSignaling::sendSignal(const T &a_signal)
{
    m_signaling->sendSignal(T::g_signalName, a_signal.toQVariant());
}

template<typename T> bool MessengerSignaling::tryHandleSignal(const QString &a_signal, const QVariant &a_value)
{
    if (a_signal != T::g_signalName)
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
    emit userInfoReceived(a_data.get_name(), a_data.get_online());
}

template<> void MessengerSignaling::handleSignal(const MessageSignal &a_data)
{
    if (!a_data.get_receiver().isNull() && a_data.get_receiver() == m_name)
    {
        auto date = QDateTime::currentDateTime();
        m_history[a_data.get_sender()].append(Message(true, date, a_data.get_text()));
        emit messageReceived(a_data.get_sender(), date, a_data.get_text());
    }
}

template<> void MessengerSignaling::handleSignal(const TypingSignal &a_data)
{
    if (!a_data.get_receiver().isNull() && a_data.get_receiver() == m_name)
    {
        m_typing[a_data.get_sender()] = a_data.get_typing();
        emit typing(a_data.get_sender(), a_data.get_typing());
    }
}
