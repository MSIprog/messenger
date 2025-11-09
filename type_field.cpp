#include "type_field.h"

TypeField::TypeField(QWidget *a_parent) : QTextEdit(a_parent)
{
    connect(&m_typingTimer, &QTimer::timeout, this, &TypeField::onTypingTimerTimeout);
    m_typingTimer.setSingleShot(true);
}

void TypeField::onTypingTimerTimeout()
{
    m_typing = false;
    emit typing(false);
}

void TypeField::keyPressEvent(QKeyEvent *a_event)
{
    QTextEdit::keyPressEvent(a_event);
    m_typingTimer.start(500);
    if (!m_typing)
    {
        m_typing = true;
        emit typing(true);
    }
    if ((a_event->key() == Qt::Key_Return || a_event->key() == Qt::Key_Enter) && !(a_event->modifiers() & Qt::ControlModifier))
    {
        emit textEntered(toPlainText());
        textCursor().setPosition(0);
        setPlainText(QString());
    }
}
