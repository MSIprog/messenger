#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <QByteArray>
#include <QList>

class BlockQueue
{
public:
    void appendBlock(const QByteArray &a_block);

    qsizetype getSize() const
    {
        return m_size;
    }

    QByteArray takeBlock(qsizetype size);

private:
    QList<QByteArray> m_blocks;

    qsizetype m_size = 0;
};

class MessageQueue
{
public:
    using MessageSize = unsigned int;

    static QByteArray createMessage(const QByteArray &a_rawData);

    void appendRawData(const QByteArray &a_rawData);

    bool messageIsReady() const;

    QByteArray takeMessage();

private:
    void updateNextMessageSize();

    BlockQueue m_data;
    std::optional<MessageSize> m_nextMessageSize;
};

#endif // BLOCK_QUEUE_H
