#ifndef TYPE_FIELD_H
#define TYPE_FIELD_H

#include <QTextEdit>
#include <QKeyEvent>
#include <QDateTime>
#include <QTimer>

class TypeField : public QTextEdit
{
    Q_OBJECT

public:
    TypeField(QWidget *a_parent);

signals:
    void textEntered(QString a_text);
    void typing(bool a_typing);

private slots:
    void onTypingTimerTimeout();

private:
    void keyPressEvent(QKeyEvent *a_event) override;

    bool m_typing = false;
    QTimer m_typingTimer;
};

#endif // TYPE_FIELD_H
