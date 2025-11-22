#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QProcess>
#include "file_form.h"
#include "ui_file_form.h"
#include "resource_holder.h"
#include "settings.h"

FileForm::FileForm(std::shared_ptr<MessengerSignaling> a_signaling, std::shared_ptr<FileSignaling> a_fileSignaling, QWidget *a_parent) :
    QWidget(a_parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
    m_ui(new Ui::FileForm)
{
    m_ui->setupUi(this);
    m_signaling = a_signaling;
    connect(m_signaling.get(), &MessengerSignaling::userRenamed, this, &FileForm::renameUser);
    m_fileSignaling = a_fileSignaling;
    connect(m_fileSignaling.get(), &FileSignaling::fileAboutToReceive, this, &FileForm::onFileAboutToReceive);
    connect(m_fileSignaling.get(), &FileSignaling::fileFragmentSent, this, &FileForm::onFileFragmentSent);
    connect(m_fileSignaling.get(), &FileSignaling::fileFragmentReceived, this, &FileForm::onFileFragmentReceived);
    connect(m_ui->filesListWidget, &QListWidget::currentItemChanged, this, &FileForm::onFileListWidgetCurrentItemChanged);
    connect(m_ui->setFolderToolButton, &QAbstractButton::clicked, this, &FileForm::onSetFileNameToolButtonClicked);
    connect(m_ui->startToolButton, &QAbstractButton::clicked, this, &FileForm::onStartToolButtonClicked);
    connect(m_ui->pauseToolButton, &QAbstractButton::clicked, this, &FileForm::onPauseToolButtonClicked);
    connect(m_ui->cancelToolButton, &QAbstractButton::clicked, this, &FileForm::onCancelToolButtonClicked);
    connect(m_ui->restartToolButton, &QAbstractButton::clicked, this, &FileForm::onRestartToolButtonClicked);
    connect(m_ui->openFolderToolButton, &QAbstractButton::clicked, this, &FileForm::onOpenFolderToolButtonClicked);
    connect(m_ui->removeToolButton, &QAbstractButton::clicked, this, &FileForm::onRemoveToolButtonClicked);
    m_ui->tabBar->setExpanding(false);
    m_ui->tabBar->addTab("All files");
    connect(m_ui->tabBar, &QTabBar::currentChanged, this, &FileForm::showFiles);
    updateButtons();
    connect(&m_blinkTimer, &QTimer::timeout, this, &FileForm::changeIcons);
    m_blinkTimer.start(500);

    auto rect = Settings::get().value("FileFormGeometry").toRect();
    if (rect != QRect())
        setGeometry(rect);
}

FileForm::~FileForm()
{
    Settings::get().setValue("FileFormGeometry", geometry());

    delete m_ui;
}

bool FileForm::sendFile(const QString &a_receiver, const QString &a_fileName)
{
    if (!m_fileSignaling->sendFile(a_receiver, a_fileName))
        return false;
    if (canAddItem(a_receiver))
    {
        auto &fileId = m_fileSignaling->getFileId(a_fileName);
        auto item = new QListWidgetItem(ResourceHolder::get().getSendIcon(), getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
        item->setData(Qt::UserRole, fileIdToString(fileId));
        m_ui->filesListWidget->addItem(item);
    }
    return true;
}

bool FileForm::hasPendingFiles()
{
    return !m_pendingSenders.empty();
}

bool FileForm::hasPendingFiles(const QString &a_sender)
{
    return m_pendingSenders.find(a_sender) != m_pendingSenders.end();
}

void FileForm::addUser(const QString &a_id)
{
    int tabIndex = getTabIndex(a_id);
    if (tabIndex != -1)
    {
        m_ui->tabBar->setCurrentIndex(tabIndex);
        m_pendingSenders.erase(a_id);
        return;
    }
    m_ui->tabBar->addTab(ResourceHolder::get().getGreenIcon(), m_signaling->getUserName(a_id));
    m_ui->tabBar->setTabData(m_ui->tabBar->count() - 1, a_id);
    m_ui->tabBar->setCurrentIndex(m_ui->tabBar->count() - 1);
    showFiles(m_ui->tabBar->count() - 1);
}

void FileForm::renameUser(QString a_id, QString a_name)
{
    auto index = getTabIndex(a_id);
    if (index == -1)
        return;
    m_ui->tabBar->setTabText(index, a_name);
}

void FileForm::onFileAboutToReceive(QString a_sender, QString a_name)
{
    if (canAddItem(a_sender))
    {
        FileId fileId{ FileActionType::Receive, a_sender, a_name };
        auto item = new QListWidgetItem(ResourceHolder::get().getReceiveIcon(), getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
        item->setData(Qt::UserRole, fileIdToString(fileId));
        m_ui->filesListWidget->addItem(item);
    }
    if (!canAddItem(a_sender) || !isVisible() || !(windowState() & Qt::WindowActive))
        m_pendingSenders.insert(a_sender);

    if (getTabIndex(a_sender) == -1)
    {
        m_ui->tabBar->addTab(ResourceHolder::get().getGreenIcon(), m_signaling->getUserName(a_sender));
        m_ui->tabBar->setTabData(m_ui->tabBar->count() - 1, a_sender);
    }
}

void FileForm::onFileFragmentSent(QString a_sender, QString a_name, size_t a_offset, size_t a_size)
{
    FileId fileId{ FileActionType::Send, a_sender, a_name };
    m_offsets[fileId] = a_offset + a_size;
    auto item = getItem(fileId);
    if (item != nullptr)
        item->setText(getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
}

void FileForm::onFileFragmentReceived(QString a_sender, QString a_name, size_t a_offset, size_t a_size)
{
    FileId fileId{ FileActionType::Receive, a_sender, a_name };
    m_offsets[fileId] = a_offset + a_size;
    auto item = getItem(fileId);
    if (item != nullptr)
        item->setText(getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
    if (m_fileSignaling->getReceivingFileInfo(m_fileSignaling->getFileName(fileId)).m_status == FileInfo::Status::Finished)
        updateButtons();
}

void FileForm::onFileListWidgetCurrentItemChanged(QListWidgetItem *, QListWidgetItem *)
{
    updateButtons();
}

void FileForm::onSetFileNameToolButtonClicked()
{
    auto fileld = getCurrentFileId();
    if (fileld == FileId())
        return;
    auto fileName = m_fileSignaling->getFileName(fileld);
    if (fileName.isNull())
        return;
    auto newFileName = QFileDialog::getSaveFileName(this);
    if (newFileName.isNull())
        return;
    m_fileSignaling->renameFileName(fileName, newFileName);
}

void FileForm::onStartToolButtonClicked()
{
    auto fileId = getCurrentFileId();
    if (fileId == FileId())
        return;
    m_fileSignaling->receiveFile(fileId.m_userId, fileId.m_name);
    updateButtons();
}

void FileForm::onPauseToolButtonClicked()
{
    auto fileId = getCurrentFileId();
    if (fileId == FileId())
        return;
    m_fileSignaling->pauseReceivingFile(fileId.m_userId, fileId.m_name);
    updateButtons();
}

void FileForm::onCancelToolButtonClicked()
{
    auto fileId = getCurrentFileId();
    if (fileId == FileId())
        return;
    m_fileSignaling->cancelReceivingFile(fileId.m_userId, fileId.m_name);
    m_offsets.erase(fileId);
    updateButtons();
    m_ui->filesListWidget->currentItem()->setText(getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
}

void FileForm::onRestartToolButtonClicked()
{
    auto fileId = getCurrentFileId();
    if (fileId == FileId())
        return;
    m_fileSignaling->cancelReceivingFile(fileId.m_userId, fileId.m_name);
    m_offsets.erase(fileId);
    m_fileSignaling->receiveFile(fileId.m_userId, fileId.m_name);
    updateButtons();
    m_ui->filesListWidget->currentItem()->setText(getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
}

void FileForm::onOpenFolderToolButtonClicked()
{
    auto fileName = getCurrentFileName();
    if (fileName.isNull())
        return;
    fileName = QDir::toNativeSeparators(fileName);
    // только для Windows
    QProcess::startDetached("explorer.exe", { "/select,", fileName });
}

void FileForm::onRemoveToolButtonClicked()
{
    auto fileId = getCurrentFileId();
    if (fileId == FileId())
        return;
    m_offsets.erase(fileId);
    m_fileSignaling->removeFile(fileId.m_userId, fileId.m_name);
    delete m_ui->filesListWidget->takeItem(m_ui->filesListWidget->currentRow());
    updateButtons();
}

void FileForm::showFiles(int a_tabIndex)
{
    m_ui->filesListWidget->clear();
    if (a_tabIndex == -1)
        return;
    auto userId = getUserId(a_tabIndex);
    for (auto fileName : m_fileSignaling->getFileNames(userId))
    {
        auto fileId = m_fileSignaling->getFileId(fileName);
        auto &icon = fileId.m_action == FileActionType::Receive ? ResourceHolder::get().getReceiveIcon() : ResourceHolder::get().getSendIcon();
        auto item = new QListWidgetItem(icon, getItemText(fileId, a_tabIndex != 0));
        item->setData(Qt::UserRole, fileIdToString(fileId));
        m_ui->filesListWidget->addItem(item);
   }
    eraseCurrentPendingSender();
}

void FileForm::changeIcons()
{
    for (int i = 1; i < m_ui->tabBar->count(); i++)
    {
        auto userId = getUserId(i);
        QIcon icon;
        if (m_blinkState && m_pendingSenders.find(userId) != m_pendingSenders.end())
            icon = ResourceHolder::get().getFileIcon();
        else if (m_signaling->userIsOnline(userId))
            icon = ResourceHolder::get().getGreenIcon();
        else
            icon = ResourceHolder::get().getRedIcon();
        m_ui->tabBar->setTabIcon(i, icon);
    }
    for (int i = 0; i < m_ui->filesListWidget->count(); i++)
    {
        auto item = m_ui->filesListWidget->item(i);
        auto fileId = fileIdFromString(item->data(Qt::UserRole).toString());
        auto &fileInfo = m_fileSignaling->getReceivingFileInfo(m_fileSignaling->getFileName(fileId));
        auto status = FileInfo::Status::Unknown;
        if (fileInfo.isValid())
            status = fileInfo.m_status;
        QIcon icon;
        if (m_blinkState && status == FileInfo::Status::Pending)
            icon = ResourceHolder::get().getFileIcon();
        else if (m_blinkState && status == FileInfo::Status::Paused)
            icon = ResourceHolder::get().getPauseIcon();
        else if (m_blinkState && status == FileInfo::Status::Error)
            icon = ResourceHolder::get().getErrorIcon();
        else
            icon = fileId.m_action == FileActionType::Receive ? ResourceHolder::get().getReceiveIcon() : ResourceHolder::get().getSendIcon();
        item->setIcon(icon);
    }
    m_blinkState = !m_blinkState;
}

// получить строку, содержащую размер, переведенный в КБ, МБ или ГБ, например: 2.07 MB
std::pair<QString, QString> FileForm::getShortFileSize(size_t a_size)
{
    // строка с сокращением
    static QString byteUnits = tr("B");
    const size_t gb = 1 << 30;
    const size_t mb = 1 << 20;
    const size_t kb = 1 << 10;
    double size = (double)a_size;
    QString units = byteUnits;
    if (a_size >= gb)
    {
        size = size / gb;
        units = tr("G") + units;
    }
    else if (a_size >= mb)
    {
        size = size / mb;
        units = tr("M") + units;
    }
    else if (a_size >= kb)
    {
        size = size / kb;
        units = tr("K") + units;
    }
    QString value;
    if (a_size < kb)
        value = QString::number(a_size); // размер до килобайта выводим без дробной части
    else if (size >= 100)
        value = QString::number(qRound(size));
    else if (size >= 10)
        value = QString::number(size, 'f', 1);
    else
        value = QString::number(size, 'f', 2);
    return std::make_pair(value, units);
}

FileId FileForm::fileIdFromString(const QString &a_id)
{
    auto index1 = a_id.lastIndexOf('/');
    auto index2 = a_id.lastIndexOf('/', index1 - 1);
    return FileId{ a_id.left(index2) == "S" ? FileActionType::Send : FileActionType::Receive,
        a_id.mid(index2 + 1, index1 - index2 - 1), a_id.mid(index1 + 1) };
}

// m_action/m_userId/m_name
QString FileForm::fileIdToString(const FileId &a_id)
{
    return QString("%1/%2/%3").arg(a_id.m_action == FileActionType::Send ? "S" : "R").arg(a_id.m_userId).arg(a_id.m_name);
}

void FileForm::changeEvent(QEvent *a_event)
{
    QWidget::changeEvent(a_event);
    switch (a_event->type())
    {
    case QEvent::ActivationChange:
    case QEvent::WindowStateChange:
        if (QApplication::activeWindow() == this)
            eraseCurrentPendingSender();
        break;
    default:
        break;
    }
}

FileId FileForm::getCurrentFileId()
{
    auto item = m_ui->filesListWidget->currentItem();
    if (item == nullptr)
        return {};
    return fileIdFromString(item->data(Qt::UserRole).toString());
}

QString FileForm::getCurrentFileName()
{
    return m_fileSignaling->getFileName(getCurrentFileId());
}

const FileInfo &FileForm::getCurrentReceivingFileInfo()
{
    return m_fileSignaling->getReceivingFileInfo(getCurrentFileName());
}

QListWidgetItem *FileForm::getItem(const FileId &a_fileId)
{
    for (int i = 0; i < m_ui->filesListWidget->count(); i++)
    {
        auto item = m_ui->filesListWidget->item(i);
        auto fileId = fileIdFromString(item->data(Qt::UserRole).toString());
        if (fileId == a_fileId)
            return item;
    }
    return nullptr;
}

QString FileForm::getItemText(const FileId &a_fileId, bool a_printUserName)
{
    QString result;
    auto userName = m_signaling->getUserName(a_fileId.m_userId);
    if (a_printUserName)
        result = QString("%1: ").arg(userName);
    size_t size = 0;
    auto fileName = m_fileSignaling->getFileName(a_fileId);
    auto &fileInfo = m_fileSignaling->getReceivingFileInfo(fileName);
    if (fileInfo.isValid())
        size = fileInfo.m_size;
    else
        size = (size_t)QFileInfo(fileName).size();
    auto sizeAndUnits = getShortFileSize(size);
    if (m_offsets.find(a_fileId) == m_offsets.end())
    {
        result += QString("%1 (%2 %3)")
            .arg(a_fileId.m_name)
            .arg(sizeAndUnits.first)
            .arg(sizeAndUnits.second);
        return result;
    }
    auto offset = m_offsets[a_fileId];
    auto percents = (int)(((double)offset / size) * 100);
    auto offsetAndUnits = getShortFileSize(offset);
    result += QString("%1 (%2 %3 / %4 %5, %6 %)")
        .arg(a_fileId.m_name)
        .arg(offsetAndUnits.first)
        .arg(offsetAndUnits.second)
        .arg(sizeAndUnits.first)
        .arg(sizeAndUnits.second)
        .arg(percents);
    return result;
}

void FileForm::updateButtons()
{
    m_ui->setFolderToolButton->setEnabled(false);
    m_ui->startToolButton->setEnabled(false);
    m_ui->startToolButton->setEnabled(false);
    m_ui->pauseToolButton->setEnabled(false);
    m_ui->cancelToolButton->setEnabled(false);
    m_ui->restartToolButton->setEnabled(false);
    m_ui->openFolderToolButton->setEnabled(false);
    m_ui->removeToolButton->setEnabled(false);

    if (m_ui->filesListWidget->currentItem() == nullptr)
        return;
    if (getCurrentFileId().m_action == FileActionType::Send)
        m_ui->openFolderToolButton->setEnabled(true);
    m_ui->removeToolButton->setEnabled(true);
    // остальные кнопки работают только для получаемых файлов
    auto &fileInfo = getCurrentReceivingFileInfo();
    if (!fileInfo.isValid())
        return;
    switch (fileInfo.m_status)
    {
    case FileInfo::Status::Pending:
        m_ui->setFolderToolButton->setEnabled(true);
        m_ui->startToolButton->setEnabled(true);
        break;
    case FileInfo::Status::Started:
        m_ui->pauseToolButton->setEnabled(true);
        m_ui->cancelToolButton->setEnabled(true);
        break;
    case FileInfo::Status::Paused:
        m_ui->startToolButton->setEnabled(true);
        m_ui->cancelToolButton->setEnabled(true);
        break;
    case FileInfo::Status::Finished:
        m_ui->restartToolButton->setEnabled(true);
        m_ui->openFolderToolButton->setEnabled(true);
        break;
    case FileInfo::Status::Error:
        m_ui->restartToolButton->setEnabled(true);
    }
}

int FileForm::getTabIndex(const QString &a_userId)
{
    for (int i = 0; i < m_ui->tabBar->count(); i++)
        if (getUserId(i) == a_userId)
            return i;
    return -1;
}

QString FileForm::getUserId(int a_tabIndex)
{
    return m_ui->tabBar->tabData(a_tabIndex).toString();
}

bool FileForm::canAddItem(const QString &a_userId)
{
    auto tabIndex = m_ui->tabBar->currentIndex();
    if (tabIndex == 0)
        return true;
    return a_userId == getUserId(tabIndex);
}

void FileForm::eraseCurrentPendingSender()
{
    auto tabIndex = m_ui->tabBar->currentIndex();
    if (tabIndex == 0)
        m_pendingSenders.clear();
    m_pendingSenders.erase(getUserId(m_ui->tabBar->currentIndex()));
}
