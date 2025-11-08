#include "block_queue.h"

void BlockQueue::appendBlock(const QByteArray &a_block)
{
    m_blocks.append(a_block);
    m_size += a_block.size();
}

QByteArray BlockQueue::takeBlock(qsizetype a_size)
{
    if (a_size > m_size)
        throw std::exception("BlockQueue.TakeBlock: requested more than stored");
    QByteArray result;
    auto size = a_size; // требуемое количество байт
    while (size != 0)
    {
        QByteArray block = m_blocks.takeFirst();
        if (block.size() > size) // если очередной блок оказался длинее требуемого количества
        {
            result.append(block.left(size));
            m_blocks.prepend(block.mid(size)); // оставшуюся часть блока записываем обратно
            break;
        }
        result.append(block);
        size -= block.size();
    }
    m_size -= a_size;
    return result;
}

//------------------------------------------------------------------------------------------------------
QByteArray MessageQueue::createMessage(const QByteArray &a_rawData)
{
    QByteArray result(sizeof(MessageSize), Qt::Uninitialized);
    *reinterpret_cast<MessageSize *>(result.data()) = a_rawData.size();
    result.append(a_rawData);
    return result;
}

void MessageQueue::appendRawData(const QByteArray &a_rawData)
{
    m_data.appendBlock(a_rawData);
    if (!m_nextMessageSize.has_value())
        updateNextMessageSize();
}

bool MessageQueue::messageIsReady() const
{
    return m_nextMessageSize.has_value();
}

QByteArray MessageQueue::takeMessage()
{
    if (!m_nextMessageSize.has_value())
        throw std::exception("MessageQueue.takeMessage: message is not ready");
    auto result = m_data.takeBlock(m_nextMessageSize.value());
    m_nextMessageSize.reset();
    updateNextMessageSize();
    return result;
}

void MessageQueue::updateNextMessageSize()
{
    if (m_data.getSize() < sizeof(MessageSize))
        return;
    auto messageSizeBlock = m_data.takeBlock(sizeof(MessageSize));
    m_nextMessageSize = *reinterpret_cast<MessageSize *>(messageSizeBlock.data());
}
