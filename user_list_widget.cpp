#include <QInputDialog>
#include <QUuid>
#include <QFileDialog>
#include "user_list_widget.h"
#include "ui_user_list_widget.h"
#include "signaling.h"
#include "resource_holder.h"
#include "settings.h"

UserListWidget::UserListWidget(std::shared_ptr<MessengerSignaling> a_signaling, std::shared_ptr<FileSignaling> a_fileSignaling, QWidget *a_parent) :
    QWidget(a_parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint),
    m_ui(new Ui::UserListWidget)
{
    m_ui->setupUi(this);
    m_signaling = a_signaling;
    m_fileSignaling = a_fileSignaling;
    connect(m_signaling.get(), &MessengerSignaling::idChanged, this, &UserListWidget::changeId);
    connect(m_signaling.get(), &MessengerSignaling::userAdded, this, &UserListWidget::addUser);
    connect(m_signaling.get(), &MessengerSignaling::userRenamed, this, &UserListWidget::renameUser);
    connect(m_signaling.get(), &MessengerSignaling::userRemoved, this, &UserListWidget::removeUser);
    QTimer::singleShot(0, this, &UserListWidget::logon);
    m_states = new QMenu(this);
    m_states->addAction(ResourceHolder::get().getGreenIcon(), "Online", this, &UserListWidget::setOnline);
    m_states->addAction(ResourceHolder::get().getRedIcon(), "Offline", this, &UserListWidget::setOffline);
    connect(m_ui->stateToolButton, &QAbstractButton::clicked, this, &UserListWidget::showStateMenu);
    connect(m_ui->listWidget, &QListWidget::itemDoubleClicked, this, &UserListWidget::showDialog);
    connect(m_ui->listWidget, &QWidget::customContextMenuRequested, this, &UserListWidget::showActionsMenu);
    m_actions = new QMenu(this);
    m_actions->addAction(ResourceHolder::get().getMessageIcon(), "Send message...", this, &UserListWidget::sendMessage);
    m_actions->addAction(ResourceHolder::get().getFileIcon(), "Send file...", this, &UserListWidget::sendFile);
    connect(&m_blinkTimer, &QTimer::timeout, this, &UserListWidget::changeIcons);
    m_blinkTimer.start(500);

    m_messageForm = new MessageForm(m_signaling, this);
    m_fileForm = new FileForm(m_signaling, m_fileSignaling, this);

    //m_fileForm->show();

    auto rect = Settings::get().value("UserListWidgetGeometry").toRect();
    if (rect != QRect())
        setGeometry(rect);
}

UserListWidget::~UserListWidget()
{
    Settings::get().setValue("UserUuid", m_signaling->getId());
    if (!m_signaling->getName().isEmpty())
        Settings::get().setValue("Name", m_signaling->getName());
    Settings::get().setValue("UserListWidgetGeometry", geometry());
    delete m_ui;
}

// private slots:
void UserListWidget::logon()
{
    QString name = QInputDialog::getText(this, "User name", "Type your name", QLineEdit::Normal, Settings::get().value("Name").toString());
    if (name.isEmpty())
        close();
    auto uid = Settings::get().value("UserUuid").toUuid();
    if (uid.isNull())
        uid = QUuid::createUuid();
    auto id = uid.toString(QUuid::WithoutBraces);
    m_signaling->setId(id);
    m_fileSignaling->setId(id);
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
    if (m_fileForm == nullptr)
        m_fileForm = new FileForm(m_signaling, m_fileSignaling, this);

    auto id = a_item->data(Qt::UserRole).toString();
    if (m_messageForm->hasUnreadMessages(id) || !m_fileForm->hasPendingFiles(id))
    {
        m_messageForm->addDialog(id);
        m_messageForm->show();
    }
    else
    {
        m_fileForm->addUser(id);
        m_fileForm->show();
    }
}

void UserListWidget::showActionsMenu(const QPoint &a_pos)
{
    auto item = m_ui->listWidget->currentItem();
    if (item == nullptr)
        return;
    m_actions->exec(m_ui->listWidget->mapToGlobal(a_pos));
}

void UserListWidget::sendMessage()
{
    if (m_messageForm == nullptr)
        m_messageForm = new MessageForm(m_signaling, this);
    m_messageForm->show();
    auto item = m_ui->listWidget->currentItem();
    if (item == nullptr)
        return;
    auto id = item->data(Qt::UserRole).toString();
    m_messageForm->addDialog(id);
}

void UserListWidget::sendFile()
{
    if (m_fileForm == nullptr)
        m_fileForm = new FileForm(m_signaling, m_fileSignaling, this);
    m_fileForm->show();
    auto item = m_ui->listWidget->currentItem();
    if (item == nullptr)
        return;
    auto fileName = QFileDialog::getOpenFileName(this);
    if (fileName.isNull())
        return;
    auto id = item->data(Qt::UserRole).toString();
    m_fileForm->addUser(id);
    m_fileForm->sendFile(id, fileName);
}

void UserListWidget::changeIcons()
{
    if (m_blinkState && m_messageForm != nullptr && m_messageForm->hasUnreadMessages())
        setWindowIcon(ResourceHolder::get().getMessageIcon());
    else if (m_blinkState && m_fileForm != nullptr && m_fileForm->hasPendingFiles())
        setWindowIcon(ResourceHolder::get().getFileIcon());
    else
        setWindowIcon(ResourceHolder::get().getApplicationIcon());
    for (int i = 0; i < m_ui->listWidget->count(); i++)
    {
        auto id = m_ui->listWidget->item(i)->data(Qt::UserRole).toString();
        QIcon icon;
        if (m_blinkState && m_messageForm != nullptr && m_messageForm->hasUnreadMessages(id))
            icon = ResourceHolder::get().getMessageIcon();
        else if (m_blinkState && m_fileForm != nullptr && m_fileForm->hasPendingFiles(id))
            icon = ResourceHolder::get().getFileIcon();
        else if (m_signaling->isTyping(id))
            icon = ResourceHolder::get().getTypingIcon();
        else
            icon = ResourceHolder::get().getApplicationIcon();
        m_ui->listWidget->item(i)->setIcon(icon);
    }
    m_blinkState = !m_blinkState;
}

void UserListWidget::changeId(QString a_id)
{
    // MessengerSignaling автоматически изменил идентификатор
    m_fileSignaling->setId(a_id);
}

void UserListWidget::addUser(QString a_id, QString a_name)
{
    auto item = new QListWidgetItem(ResourceHolder::get().getApplicationIcon(), a_name);
    item->setData(Qt::UserRole, a_id);
    m_ui->listWidget->addItem(item);
}

void UserListWidget::renameUser(QString a_id, QString a_name)
{
    auto index = getUserIndex(a_id);
    if (index == -1)
        return;
    m_ui->listWidget->item(index)->setText(a_name);
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
