#include <QInputDialog>
#include <QSettings>
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
	connect(m_signaling.get(), &MessengerSignaling::userInfoReceived, this, &UserListWidget::onUserInfoReceived);
	QTimer::singleShot(0, this, &UserListWidget::logon);
	m_states = new QMenu(this);
	m_states->addAction(ResourceHolder::get().getGreenIcon(), "Online", this, &UserListWidget::setOnline);
	m_states->addAction(ResourceHolder::get().getRedIcon(), "Offline", this, &UserListWidget::setOffline);
	connect(m_ui->stateToolButton, &QAbstractButton::clicked, this, &UserListWidget::showStateMenu);
    connect(m_ui->listWidget, &QListWidget::itemDoubleClicked, this, &UserListWidget::showDialog);
	connect(&m_kickTimer, &QTimer::timeout, this, &UserListWidget::autoKickUser);
	m_kickTimer.start(1000);
	connect(&m_blinkTimer, &QTimer::timeout, this, &UserListWidget::blinkMessages);
	m_blinkTimer.start(500);

	m_messageForm = new MessageForm(m_signaling, this);

	auto rect = QSettings().value("UserListWidgetGeometry").toRect();
	if (rect != QRect())
		setGeometry(rect);
}

UserListWidget::~UserListWidget()
{
	if (!m_ui->nameLabel->text().isEmpty())
		QSettings().setValue("Name", m_ui->nameLabel->text());
	QSettings().setValue("UserListWidgetGeometry", geometry());
	delete m_ui;
}

// private slots:
void UserListWidget::removeUser(QString a_name)
{
	if (m_messageForm != nullptr)
		m_messageForm->InvalidateDialog(a_name);
	auto index = getUserIndex(a_name);
	if (index == -1)
		return;
	m_users.removeAt(index);
	delete m_ui->listWidget->takeItem(index);
}

void UserListWidget::logon()
{
	QString name = QInputDialog::getText(this, "User name", "Type your name", QLineEdit::Normal, QSettings().value("Name").toString());
	if (name.isEmpty())
		close();
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
	m_messageForm->AddDialog(a_item->text());
	m_messageForm->show();
}

void UserListWidget::autoKickUser()
{
	// обратный порядок из-за удаления из m_users в RemoveUser
	for (int i = m_users.count() - 1; i >= 0; i--)
		if (m_users[i].m_refresh_time.secsTo(QDateTime::currentDateTime()) >= 2)
			removeUser(m_users[i].m_name);
}

void UserListWidget::blinkMessages()
{
	if (m_messageForm == nullptr)
		return;
	setWindowIcon(m_messageForm->HasUnreadMessages() && m_blinkState ? ResourceHolder::get().getMessageIcon() : ResourceHolder::get().getApplicationIcon());
	for (int i = 0; i < m_users.count(); i++)
	{
		bool hasUnreadMessages = m_messageForm->HasUnreadMessages(m_users[i].m_name);
		m_ui->listWidget->item(i)->setIcon(hasUnreadMessages && m_blinkState ? ResourceHolder::get().getMessageIcon() : ResourceHolder::get().getApplicationIcon());
	}
	m_blinkState = !m_blinkState;
}

void UserListWidget::onUserInfoReceived(QString a_name, bool a_online)
{
	if (!a_online)
	{
		removeUser(a_name);
		return;
	}
	auto index = getUserIndex(a_name);
	if (index != -1)
	{
		m_users[index].m_refresh_time = QDateTime::currentDateTime();
		return;
	}
	UserInfo user;
	user.m_name = a_name;
	user.m_refresh_time = QDateTime::currentDateTime();
	m_users.append(user);
	m_ui->listWidget->addItem(new QListWidgetItem(ResourceHolder::get().getApplicationIcon(), a_name, m_ui->listWidget));
	if (m_messageForm != NULL)
		m_messageForm->ValidateDialog(a_name);
}

int UserListWidget::getUserIndex(const QString &a_name)
{
	//std::find_if()
	for (int i = 0; i < m_users.count(); i++)
		if (m_users[i].m_name == a_name)
			return i;
	return -1;
}

bool UserListWidget::hasUnreadMessages()
{
	Q_ASSERT(m_messageForm != nullptr);
	return m_messageForm->HasUnreadMessages();
}

bool UserListWidget::hasUnreadMessages(const QString& a_name)
{
	Q_ASSERT(m_messageForm != nullptr);
	return m_messageForm->HasUnreadMessages(a_name);
}
