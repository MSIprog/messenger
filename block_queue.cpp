#include "block_queue.h"

void BlockQueue::AppendBlock(const QByteArray &a_block)
{
	m_blocks.append(a_block);
	m_size += a_block.size();
}

QByteArray BlockQueue::TakeBlock(int a_size)
{
	if(a_size > m_size)
		return QByteArray();
	QByteArray result;
	int size = a_size; //требуемое количество байт с очередного блока
	while(size > 0)
	{
		QByteArray block = m_blocks.takeFirst();
		if(block.size() > size) //если очередной блок оказался длинее требуемого количества
		{
			result.append(block.left(size));
			m_blocks.prepend(block.mid(size)); //оставшуюся часть блока записываем обратно
			size = 0;
		}
		else
		{
			result.append(block);
			size -= block.size();
		}
	}
	m_size -= a_size;
	return result;
}

//------------------------------------------------------------------------------------------------------
QByteArray MessageQueue::GetMessageForWriting(QByteArray a_data)
{
	QByteArray result = a_data;
	QByteArray prefix(sizeof(int), Qt::Uninitialized);
	*(int*)prefix.data() = (int)a_data.size();
	result.prepend(prefix);
	return result;
}

void MessageQueue::AppendBlock(const QByteArray &a_block)
{
	BlockQueue::AppendBlock(a_block);
	if((m_next_message_size < 0) && (GetSize() >= sizeof(int))) //если длина след. сообщения не определена, и определить ее возможно
		m_next_message_size = *(int *)TakeBlock(sizeof(int)).constData();
}

bool MessageQueue::IsMessageReady()
{
	if(m_next_message_size < 0)
		return false; //длина след. сообщения не определена
	if(GetSize() < m_next_message_size)
		return false;
	return true;
}

QByteArray MessageQueue::TakeMessage()
{
	if(!IsMessageReady())
		return QByteArray();
	QByteArray msg = TakeBlock(m_next_message_size);
	m_next_message_size = -1;
	if(GetSize() != 0)
	{
		//извлекаем остаток и помещаем его обратно в попытке определить длину след. сообщения
		QByteArray block = TakeBlock(GetSize());
		AppendBlock(block);
	}
	return msg;
}
