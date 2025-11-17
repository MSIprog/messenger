#include "resource_holder.h"

const ResourceHolder& ResourceHolder::get()
{
    static ResourceHolder instance;
    return instance;
}

ResourceHolder::ResourceHolder()
{
    m_applicationIcon = QIcon(":/icons/16x16/licq.png");
    m_greenIcon = QIcon(":/icons/16x16/ledgreen.png");
    m_redIcon = QIcon(":/icons/16x16/ledred.png");
    m_messageIcon = QIcon(":/icons/16x16/message.png");
    m_typingIcon = QIcon(":/icons/16x16/typing.png");
    m_fileIcon = QIcon(":/icons/16x16/file.png");
}
