#pragma once

#include <QIcon>

class ResourceHolder
{
public:
    static const ResourceHolder& get();

    ResourceHolder();

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

private:
    QIcon m_applicationIcon;
    QIcon m_greenIcon;
    QIcon m_redIcon;
    QIcon m_messageIcon;
};
