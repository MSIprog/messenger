#include <QInputDialog>
#include <QSettings>
#include <QUuid>
#include "user_list_widget.h"
#include "ui_user_list_widget.h"
#include "signaling.h"
#include "resource_holder.h"

UserListWidget::UserListWidget(std::shared_ptr<MessengerSignaling> a_signaling, QWidget *a_parent) :
    QWidget(a_parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint),
    m_ui(new Ui::UserListWidget)
{
    m_ui->setupUi(this);
    m_signaling = a_signaling;
    connect(m_signaling.get(), &MessengerSignaling::userAdded, this, &UserListWidget::addUser);
    connect(m_signaling.get(), &MessengerSignaling::userRemoved, this, &UserListWidget::removeUser);
    QTimer::singleShot(0, this, &UserListWidget::logon);
    m_states = new QMenu(this);
    m_states->addAction(ResourceHolder::get().getGreenIcon(), "Online", this, &UserListWidget::setOnline);
    m_states->addAction(ResourceHolder::get().getRedIcon(), "Offline", this, &UserListWidget::setOffline);
    connect(m_ui->stateToolButton, &QAbstractButton::clicked, this, &UserListWidget::showStateMenu);
    connect(m_ui->listWidget, &QListWidget::itemDoubleClicked, this, &UserListWidget::showDialog);
    connect(&m_blinkTimer, &QTimer::timeout, this, &UserListWidget::changeIcons);
    m_blinkTimer.start(500);

    m_messageForm = new MessageForm(m_signaling, this);

    auto rect = QSettings().value("UserListWidgetGeometry").toRect();
    if (rect != QRect())
        setGeometry(rect);
}

UserListWidget::~UserListWidget()
{
    QSettings().setValue("UserUuid", m_signaling->getId());
    if (!m_signaling->getName().isEmpty())
        QSettings().setValue("Name", m_signaling->getName());
    QSettings().setValue("UserListWidgetGeometry", geometry());
    delete m_ui;
}

// private slots:
void UserListWidget::logon()
{
    QString name = QInputDialog::getText(this, "User name", "Type your name", QLineEdit::Normal, QSettings().value("Name").toString());
    if (name.isEmpty())
        close();
    auto uid = QSettings().value("UserUuid").toUuid();
    if (uid.isNull())
        uid = QUuid::createUuid();
    m_signaling->setId(uid.toString(QUuid::WithoutBraces));
    // todo: проверить уникальность имени
    m_signaling->setName(name);
    m_ui->nameLabel->setText(name);
    setOnline();
}

void UserListWidget::setOnline()
{
    m_signaling->setOnline(true);
    m_ui->stateToolButton->setIcon(ResourceHolder::get().getGreenIcon());
}

void UserListWidget::setOffline()
{
    m_signaling->setOnline(false);
    m_ui->stateToolButton->setIcon(ResourceHolder::get().getRedIcon());
}

void UserListWidget::showStateMenu()
{
    m_states->exec(mapToGlobal(m_ui->stateToolButton->geometry().bottomLeft()));
}

void UserListWidget::showDialog(QListWidgetItem *a_item)
{
    if (m_messageForm == nullptr)
        m_messageForm = new MessageForm(m_signaling, this);
    m_messageForm->addDialog(a_item->data(Qt::UserRole).toString(), a_item->text());
    m_messageForm->show();
}

void UserListWidget::changeIcons()
{
    if (m_messageForm == nullptr)
        return;
    setWindowIcon(m_messageForm->hasUnreadMessages() && m_blinkState ? ResourceHolder::get().getMessageIcon() : ResourceHolder::get().getApplicationIcon());
    for (int i = 0; i < m_ui->listWidget->count(); i++)
    {
        auto id = m_ui->listWidget->item(i)->data(Qt::UserRole).toString();
        QIcon icon;
        if (m_messageForm->hasUnreadMessages(id) && m_blinkState)
            icon = ResourceHolder::get().getMessageIcon();
        else if (m_signaling->isTyping(id))
            icon = ResourceHolder::get().getTypingIcon();
        else
            icon = ResourceHolder::get().getApplicationIcon();
        m_ui->listWidget->item(i)->setIcon(icon);
    }
    m_blinkState = !m_blinkState;
}

void UserListWidget::addUser(QString a_id, QString a_name)
{
    auto item = new QListWidgetItem(ResourceHolder::get().getApplicationIcon(), a_name);
    item->setData(Qt::UserRole, a_id);
    m_ui->listWidget->addItem(item);
}

void UserListWidget::removeUser(QString a_id)
{
    auto index = getUserIndex(a_id);
    if (index == -1)
        return;
    delete m_ui->listWidget->takeItem(index);
}

int UserListWidget::getUserIndex(const QString &a_id)
{
    for (int i = 0; i < m_ui->listWidget->count(); i++)
        if (m_ui->listWidget->item(i)->data(Qt::UserRole).toString() == a_id)
            return i;
    return -1;
}

bool UserListWidget::hasUnreadMessages()
{
    Q_ASSERT(m_messageForm != nullptr);
    return m_messageForm->hasUnreadMessages();
}

bool UserListWidget::hasUnreadMessages(const QString& a_id)
{
    Q_ASSERT(m_messageForm != nullptr);
    return m_messageForm->hasUnreadMessages(a_id);
}
