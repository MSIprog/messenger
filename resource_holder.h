#pragma once

#include <QIcon>

class ResourceHolder
{
public:
    static const ResourceHolder &get();

    const QIcon &getApplicationIcon() const
    {
        return m_applicationIcon;
    }

    const QIcon &getGreenIcon() const
    {
        return m_greenIcon;
    }

    const QIcon &getRedIcon() const
    {
        return m_redIcon;
    }

    const QIcon &getMessageIcon() const
    {
        return m_messageIcon;
    }

    const QIcon &getTypingIcon() const
    {
        return m_typingIcon;
    }

    const QIcon &getFileIcon() const
    {
        return m_fileIcon;
    }

    const QIcon &getSendIcon() const
    {
        return m_sendIcon;
    }

    const QIcon &getReceiveIcon() const
    {
        return m_receiveIcon;
    }

    const QIcon &getPauseIcon() const
    {
        return m_pauseIcon;
    }

    const QIcon &getErrorIcon() const
    {
        return m_errorIcon;
    }

private:
    ResourceHolder();

    QIcon m_applicationIcon;
    QIcon m_greenIcon;
    QIcon m_redIcon;
    QIcon m_messageIcon;
    QIcon m_typingIcon;
    QIcon m_fileIcon;
    QIcon m_sendIcon;
    QIcon m_receiveIcon;
    QIcon m_pauseIcon;
    QIcon m_errorIcon;
};
