#pragma once

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
	/*bool hasUnreadMessages();
	bool hasUnreadMessages(const QString &a_sender);
	QList<std::pair<QDateTime, QString>> getLastMessages(const QString &a_sender);*/
	QList<Message> getMessages(const QString &a_sender);

public slots:
	void sendMessage(const QString& a_text, const QString& a_receiver);

signals:
	void userInfoReceived(QString a_name, bool a_online);
	void messageReceived(QString a_sender, QDateTime a_date, QString a_text);

private slots:
	void sendUserInfo();
	void onSignalReceived(QString a_signal, QVariant a_value);

private:
	std::shared_ptr<Signaling> m_signaling;
	QString m_name;
	bool m_online = true;
	QMap<QString, QList<Message>> m_history;
	//QMap<QString, size_t> m_historyIndex;
	QTimer m_timer; // таймер для sendUserInfo
};
