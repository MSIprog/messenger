#include "messenger_signaling.h"

MessengerSignaling::MessengerSignaling(std::shared_ptr<Signaling> a_signaling)
{
    m_signaling = a_signaling;
    connect(m_signaling.get(), &Signaling::signalReceived, this, &MessengerSignaling::onSignalReceived);
    m_signaling->subscribe("UserInfo");
    m_signaling->subscribe("Message");
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

/*bool MessengerSignaling::hasUnreadMessages()
{
    for (auto &sender : m_history.keys())
        if (hasUnreadMessages(sender))
            return true;
    return false;
}

bool MessengerSignaling::hasUnreadMessages(const QString &a_sender)
{
    auto &messages = m_history[a_sender];
    auto index = m_historyIndex[a_sender];
    return messages.count() == index;
}

QList<std::pair<QDateTime, QString>> MessengerSignaling::getLastMessages(const QString &a_sender)
{
    auto &messages = m_history[a_sender];
    size_t &index = m_historyIndex[a_sender];
    QList<std::pair<QDateTime, QString>> result(messages.begin() + index, messages.end());
    index = (size_t)std::distance(messages.end(), messages.end());
    return result;
}*/

QList<Message> MessengerSignaling::getMessages(const QString &a_sender)
{
    return m_history[a_sender];
}

void MessengerSignaling::sendMessage(const QString &a_text, const QString &a_receiver)
{
    QMap<QString, QVariant> params;
    params["sender"] = m_name;
    params["receiver"] = a_receiver;
    params["text"] = a_text;
    m_signaling->sendSignal("Message", params);
    m_history[m_name].append(Message(false, QDateTime::currentDateTime(), a_text));
}

void MessengerSignaling::sendUserInfo()
{
    QMap<QString, QVariant> params;
    params["name"] = m_name;
    params["online"] = m_online;
    m_signaling->sendSignal("UserInfo", params);
}

void MessengerSignaling::onSignalReceived(QString a_signal, QVariant a_value)
{
    if (a_signal == "UserInfo")
    {
        auto params = a_value.toMap();
        emit userInfoReceived(params["name"].toString(), params["online"].toBool());
    }
    else if (a_signal == "Message")
    {
        auto params = a_value.toMap();
        if (params.contains("receiver") && params["receiver"].toString() == m_name)
        {
            auto sender = params["sender"].toString();
            auto date = QDateTime::currentDateTime();
            auto text = params["text"].toString();
            auto& messages = m_history[sender];
            messages.append(Message(true, date, text));
            emit messageReceived(params["sender"].toString(), date, text);
        }
    }
}
