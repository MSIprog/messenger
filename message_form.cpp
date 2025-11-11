#include <QScrollBar>
#include <QSettings>
#include <QDateTime>
#include "message_form.h"
#include "ui_message_form.h"
#include "resource_holder.h"

MessageForm::MessageForm(std::shared_ptr<MessengerSignaling> a_signaling, QWidget *a_parent) :
    QWidget(a_parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
    m_ui(new Ui::MessageForm)
{
    m_ui->setupUi(this);
    m_signaling = a_signaling;
    connect(a_signaling.get(), &MessengerSignaling::messageReceived, this, &MessageForm::onMessageReceived);
    connect(m_signaling.get(), &MessengerSignaling::userAdded, this, &MessageForm::onUserAdded);
    connect(m_signaling.get(), &MessengerSignaling::userRemoved, this, &MessageForm::onUserRemoved);
    connect(m_ui->typeField, &TypeField::textEntered, this, &MessageForm::sendText);
    connect(m_ui->typeField, &TypeField::typing, this, &MessageForm::sendTyping);
    m_ui->tabBar->setExpanding(false);
    m_ui->tabBar->setTabsClosable(true);
    connect(m_ui->tabBar, &QTabBar::currentChanged, this, &MessageForm::showHistory);
    connect(m_ui->tabBar, &QTabBar::tabCloseRequested, this, &MessageForm::closeTab);
    connect(&m_blinkTimer, &QTimer::timeout, this, &MessageForm::changeIcons);
    m_blinkTimer.start(500);

    auto rect = QSettings().value("MessageFormGeometry").toRect();
    if (rect != QRect())
        setGeometry(rect);
    auto variantSizes = QSettings().value("MessageFormSplitterSizes").toList();
    if (!variantSizes.isEmpty())
    {
        QList<int> sizes(variantSizes.size());
        std::transform(variantSizes.begin(), variantSizes.end(), sizes.begin(), [](auto &a_variantSize)
            {
                return a_variantSize.toInt();
            });
        m_ui->splitter->setSizes(sizes);
    }
}

MessageForm::~MessageForm()
{
    QSettings().setValue("MessageFormGeometry", geometry());
    auto sizes = m_ui->splitter->sizes();
    QList<QVariant> variantSizes(sizes.size());
    std::transform(sizes.begin(), sizes.end(), variantSizes.begin(), [](auto a_size)
        {
            return a_size;
        });
    QSettings().setValue("MessageFormSplitterSizes", variantSizes);

    delete m_ui;
}

void MessageForm::addDialog(const QString &a_id, const QString &a_name)
{
    int tabIndex = getTabIndex(a_id);
    if (tabIndex != -1)
    {
        m_ui->tabBar->setCurrentIndex(tabIndex);
        onMessagesRead(a_id);
        return;
    }
    m_ui->tabBar->addTab(ResourceHolder::get().getGreenIcon(), a_name);
    m_ui->tabBar->setCurrentIndex(m_ui->tabBar->count() - 1);
    m_ui->tabBar->setTabData(m_ui->tabBar->count() - 1, a_id);
    receiveHistory(a_id);
    showHistory(m_ui->tabBar->count() - 1);
    onMessagesRead(a_id);
}

bool MessageForm::hasUnreadMessages()
{
    return !m_unreadSenders.empty();
}

bool MessageForm::hasUnreadMessages(const QString &a_id)
{
    return m_unreadSenders.find(a_id) != m_unreadSenders.end();
}

void MessageForm::onUserAdded(QString a_id, QString)
{
    int tabIndex = getTabIndex(a_id);
    if (tabIndex == -1)
        return;
    m_ui->tabBar->setTabIcon(getTabIndex(a_id), ResourceHolder::get().getGreenIcon());
}

void MessageForm::onUserRemoved(const QString &a_id)
{
    int tabIndex = getTabIndex(a_id);
    if (tabIndex == -1)
        return;
    m_ui->tabBar->setTabIcon(tabIndex, ResourceHolder::get().getRedIcon());
}

void MessageForm::showHistory(int a_tabIndex)
{
    if (a_tabIndex == -1)
        return;
    auto id = getUserId(a_tabIndex);
    m_ui->dialogField->setHtml(m_history[id]);
    onMessagesRead(id);
}

void MessageForm::closeTab(int a_tabIndex)
{
    m_ui->tabBar->removeTab(a_tabIndex);
    if (m_ui->tabBar->count() == 0)
        close();
}

void MessageForm::sendText(const QString &a_text)
{
    QString receiver = getCurrentUserId();
    m_signaling->sendMessage(receiver, a_text);
    auto message = formatMessage(false, m_signaling->getName(), QDateTime::currentDateTime(), a_text);
    m_history[receiver] += message;
    appendHTML(message);
}

void MessageForm::sendTyping(bool a_typing)
{
    m_signaling->sendTyping(getCurrentUserId(), a_typing);
}

void MessageForm::onMessageReceived(QString a_sender, QDateTime a_date, QString a_text)
{
    auto formattedMessage = formatMessage(true, m_signaling->getUserName(a_sender), a_date, a_text);
    m_history[a_sender] += formattedMessage;
    if (getCurrentUserId() == a_sender)
        appendHTML(formattedMessage);
    if (!isVisible() || getCurrentUserId() != a_sender)
    {
        m_unreadSenders.insert(a_sender);
        return;
    }
    if (windowState() & Qt::WindowActive)
        onMessagesRead(a_sender);
    else
        m_unreadSenders.insert(a_sender);
}

void MessageForm::changeIcons()
{
    for (int i = 0; i < m_ui->tabBar->count(); i++)
    {
        auto id = getUserId(i);
        QIcon icon;
        if (hasUnreadMessages(id) && m_blinkState)
            icon = ResourceHolder::get().getMessageIcon();
        else if (m_signaling->isTyping(id))
            icon = ResourceHolder::get().getTypingIcon();
        else if (m_signaling->userIsOnline(id))
            icon = ResourceHolder::get().getGreenIcon();
        else
            icon = ResourceHolder::get().getRedIcon();
        m_ui->tabBar->setTabIcon(i, icon);
    }
    m_blinkState = !m_blinkState;
}

void MessageForm::changeEvent(QEvent *a_event)
{
    QWidget::changeEvent(a_event);
    switch (a_event->type())
    {
    case QEvent::ActivationChange:
    case QEvent::WindowStateChange:
        if (QApplication::activeWindow() == this)
            onMessagesRead(getCurrentUserId());
        break;
    default:
        break;
    }
}

void MessageForm::onMessagesRead(const QString &a_sender)
{
    m_unreadSenders.erase(a_sender);
    emit messagesRead(a_sender);
}

void MessageForm::receiveHistory(const QString &a_id)
{
    auto &messages = m_history[a_id];
    messages.clear();
    for (auto &message : m_signaling->getMessages(a_id))
        messages.append(formatMessage(message.m_sentToSender, message.m_sentToSender ? m_signaling->getUserName(a_id) : m_signaling->getName(), message.m_date, message.m_text));
}

void MessageForm::appendHTML(const QString& a_text)
{
    QTextCursor cursor = m_ui->dialogField->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(a_text);
    auto bar = m_ui->dialogField->verticalScrollBar();
    bar->setValue(bar->maximum());
}

QString MessageForm::formatMessage(bool a_sentToSender, const QString &a_name, const QDateTime &a_date, const QString &a_text)
{
    return QString("<font color=%1><b>%2</b></font> <font color=gray>(%3)</font><br>%4<br><br>")
        .arg(a_sentToSender ? "red" : "blue")
        .arg(a_name)
        .arg(a_date.toString("dd.MM.yyyy hh:mm:ss"))
        .arg(a_text);
}

int MessageForm::getTabIndex(const QString &a_id)
{
    for (int i = 0; i < m_ui->tabBar->count(); i++)
        if (getUserId(i) == a_id)
            return i;
    return -1;
}

QString MessageForm::getUserId(int a_tabIndex)
{
    return m_ui->tabBar->tabData(a_tabIndex).toString();
}

QString MessageForm::getCurrentUserId()
{
    return getUserId(m_ui->tabBar->currentIndex());
}
