#include "type_field.h"

TypeField::TypeField(QWidget *a_parent) : QTextEdit(a_parent)
{
}

void TypeField::keyPressEvent(QKeyEvent *a_event)
{
	if ((a_event->key() == Qt::Key_Return || a_event->key() == Qt::Key_Enter) && !(a_event->modifiers() & Qt::ControlModifier))
	{
		emit textEntered(toPlainText());
		textCursor().setPosition(0);
		setPlainText(QString());
	}
	else
		QTextEdit::keyPressEvent(a_event);
}
