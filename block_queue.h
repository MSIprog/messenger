#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <QByteArray>
#include <QList>

class BlockQueue
{
public:
	void AppendBlock(const QByteArray &a_block);
	int GetSize() const
	{
		return m_size;
	}
	QByteArray TakeBlock(int size);

private:
	QList<QByteArray> m_blocks;
	int m_size = 0;
};

class MessageQueue : private BlockQueue
{
public:
	static QByteArray GetMessageForWriting(QByteArray a_data);

	void AppendBlock(const QByteArray &a_block);
	bool IsMessageReady();
	QByteArray TakeMessage();

private:
	//ожидаемая длина следующего сообщения
	int m_next_message_size = -1;
};

#endif // BLOCK_QUEUE_H
