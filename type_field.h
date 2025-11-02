#ifndef TYPE_FIELD_H
#define TYPE_FIELD_H

#include <QTextEdit>
#include <QKeyEvent>

class TypeField : public QTextEdit
{
	Q_OBJECT

public:
	TypeField(QWidget *a_parent);

signals:
	void textEntered(QString a_text);

private:
	void keyPressEvent(QKeyEvent *a_event) override;
};

#endif // TYPE_FIELD_H
