#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QProcess>
#include "file_form.h"
#include "ui_file_form.h"
#include "resource_holder.h"
#include "settings.h"

FileId FileId::fromString(const QString &a_id)
{
    auto index1 = a_id.lastIndexOf('/');
    auto index2 = a_id.lastIndexOf('/', index1 - 1);
    return FileId{ a_id.left(index2) == "S" ? FileActionType::Send : FileActionType::Receive,
        a_id.mid(index2 + 1, index1 - index2 - 1), a_id.mid(index1 + 1)};
}

QString FileId::toString() const
{
    return QString("%1/%2/%3").arg(m_action == FileActionType::Send ? "S" : "R").arg(m_userId).arg(m_name);
}

bool FileId::operator==(const FileId &a_id) const
{
    return m_userId == a_id.m_userId && m_name == a_id.m_name && m_action == a_id.m_action;
}

bool FileId::operator<(const FileId &a_id) const
{
    if (m_action != a_id.m_action)
        return m_action < a_id.m_action;
    if (m_userId != a_id.m_userId)
        return m_userId < a_id.m_userId;
    return m_name < a_id.m_name;
}

FileForm::FileForm(std::shared_ptr<MessengerSignaling> a_signaling, QWidget *a_parent) :
    QWidget(a_parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
    m_ui(new Ui::FileForm)
{
    m_ui->setupUi(this);
    m_signaling = a_signaling;
    connect(a_signaling.get(), &MessengerSignaling::fileInfoReceived, this, &FileForm::onFileInfoReceived);
    connect(a_signaling.get(), &MessengerSignaling::fileContentsReceived, this, &FileForm::onFileContentsReceived);
    connect(m_ui->filesListWidget, &QListWidget::currentItemChanged, this, &FileForm::onFileListWidgetCurrentItemChanged);
    connect(m_ui->setFolderToolButton, &QAbstractButton::clicked, this, &FileForm::onSetFolderToolButtonClicked);
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
    QFileInfo fileInfo(a_fileName);
    if (!fileInfo.exists())
        return false;
    auto name = fileInfo.fileName();
    FileId fileId{ FileActionType::Send, a_receiver, name };
    if (!getFileName(fileId).isNull())
        return false; // файл с таким именем уже отправлен/отправляется
    m_fileNames[fileId] = a_fileName;

    if (canAddItem(a_receiver))
    {
        auto item = new QListWidgetItem(QIcon(":/upload"), getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
        item->setData(Qt::UserRole, fileId.toString());
        m_ui->filesListWidget->addItem(item);
    }

    // идентификатор ресурса - короткое имя файла
    // передача файлов с одинаковыми короткими именами, но разными полными именами невозможна
    m_signaling->sendFileInfo(a_receiver, name, fileInfo.lastModified(), (size_t)fileInfo.size());
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

void FileForm::onFileInfoReceived(QString a_sender, QString a_name, QDateTime a_modificationDate, size_t a_size)
{
    auto fileName = createReceivingFileName(a_sender, a_name);
    if (getReceivingFileInfo(fileName).isValid())
        return; // файл с таким именем уже принимается/принят
    QDir().mkpath(QFileInfo(fileName).path());

    m_receivingFiles[fileName] = FileInfo{ FileInfo::Status::Pending, a_modificationDate, a_size };

    FileId fileId{ FileActionType::Receive, a_sender, a_name };
    m_fileNames[fileId] = fileName;

    if (canAddItem(a_sender))
    {
        auto item = new QListWidgetItem(QIcon(":/download"), getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
        item->setData(Qt::UserRole, fileId.toString());
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

#include <QThread>

void FileForm::onFileContentsReceived(QString a_sender, QString a_name, size_t a_offset, size_t a_size, QByteArray a_contents)
{
    FileId fileId{ FileActionType::Send, a_sender, a_name };
    auto fileName = getFileName(fileId);
    if (!fileName.isNull())
    {
        // отправка
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            // файл удален
            m_signaling->sendFileContents(a_sender, a_name, a_offset, QByteArray());
            return;
        }
        if (!file.seek(a_offset))
        {
            // неправильное смещение
            m_signaling->sendFileContents(a_sender, a_name, a_offset, QByteArray());
            return;
        }
        auto contents = file.read(a_size);
        if (contents.size() != a_size)
        {
            // неправильный размер
            m_signaling->sendFileContents(a_sender, a_name, a_offset, QByteArray());
            return;
        }
        m_signaling->sendFileContents(a_sender, a_name, a_offset, contents);
        m_offsets[fileId] = a_offset + a_size; // обновляем смещение для индикатора выполнения
    }
    else
    {
        // прием
        fileId = FileId{ FileActionType::Receive, a_sender, a_name };
        fileName = getFileName(fileId);
        if (fileName.isNull())
            return; // файл не запрашивался
        auto &fileInfo = getReceivingFileInfo(fileName);
        if (!fileInfo.isValid())
            return;
        if (fileInfo.m_status != FileInfo::Status::Started)
            return; // пользователь отказался от приема файла
        if (a_size == 0)
        {
            // отправитель вернул ошибку
            fileInfo.m_status = FileInfo::Status::Error;
            return;
        }
        auto &offset = m_offsets[fileId];
        if (offset != a_offset)
        {
            // неправильное смещение
            fileInfo.m_status = FileInfo::Status::Error;
            return;
        }
        QFile file(fileName);
        file.open(QIODevice::Append);
        if (file.pos() != offset)
        {
            // неправильный размер локального файла
            fileInfo.m_status = FileInfo::Status::Error;
            return;
        }
        file.write(a_contents);
        offset += a_contents.size();

        QThread::msleep(200);

        // запрашиваем следующий фрагмент
        if (fileInfo.m_status == FileInfo::Status::Started)
        {
            auto nextFragmentSize = std::min(fileInfo.m_size - offset, m_maxFragmentSize);
            if (nextFragmentSize != 0)
                m_signaling->requestFileContents(a_sender, a_name, offset, nextFragmentSize);
            else
            {
                // прием завершен
                fileInfo.m_status = FileInfo::Status::Finished;

                updateButtons();
            }
        }
    }

    auto item = getItem(fileId);
    if (item != nullptr)
        item->setText(getItemText(fileId, m_ui->tabBar->currentIndex() != 0));
}

void FileForm::onFileListWidgetCurrentItemChanged(QListWidgetItem *, QListWidgetItem *)
{
    updateButtons();
}

void FileForm::onSetFolderToolButtonClicked()
{
    auto id = getCurrentFileId();
    if (id == FileId())
        return;
    auto fileName = getFileName(id);
    if (fileName.isNull())
        return;
    auto newFileName = QFileDialog::getSaveFileName(this);
    if (newFileName.isNull())
        return;
    // это работает только с файлами в ожидании (если ни одного фрагмента не было получено)
    m_fileNames.erase(id);
    m_fileNames[id] = newFileName;
    auto fileInfo = m_receivingFiles[fileName];
    m_receivingFiles.erase(fileName);
    m_receivingFiles[newFileName] = fileInfo;
}

void FileForm::onStartToolButtonClicked()
{
    auto id = getCurrentFileId();
    if (id == FileId())
        return;
    auto fileName = getFileName(id);
    auto &fileInfo = getReceivingFileInfo(fileName);
    if (!fileInfo.isValid())
        return;
    // перед началом приема файла удалим существующий файл
    if (fileInfo.m_status == FileInfo::Status::Pending &&
        QFileInfo(fileName).exists() &&
        !QFile::remove(fileName))
        return;
    fileInfo.m_status = FileInfo::Status::Started;

    updateButtons();

    m_signaling->requestFileContents(id.m_userId, id.m_name, m_offsets[id], fileInfo.m_size > m_maxFragmentSize ? m_maxFragmentSize : fileInfo.m_size);
}

void FileForm::onPauseToolButtonClicked()
{
    auto &fileInfo = getCurrentReceivingFileInfo();
    if (!fileInfo.isValid())
        return;
    fileInfo.m_status = FileInfo::Status::Paused;

    updateButtons();
}

void FileForm::onCancelToolButtonClicked()
{
    auto id = getCurrentFileId();
    auto fileName = getFileName(id);
    auto &fileInfo = getReceivingFileInfo(fileName);
    if (!fileInfo.isValid())
        return;
    QFile::remove(fileName);
    m_offsets.erase(id);
    fileInfo.m_status = FileInfo::Status::Pending;

    updateButtons();
    m_ui->filesListWidget->currentItem()->setText(getItemText(id, m_ui->tabBar->currentIndex() != 0));
}

void FileForm::onRestartToolButtonClicked()
{
    auto id = getCurrentFileId();
    auto fileName = getFileName(id);
    auto &fileInfo = getReceivingFileInfo(fileName);
    if (!fileInfo.isValid())
        return;
    QFile::remove(fileName);
    m_offsets[id] = 0;
    fileInfo.m_status = FileInfo::Status::Started;

    updateButtons();
    m_ui->filesListWidget->currentItem()->setText(getItemText(id, m_ui->tabBar->currentIndex() != 0));

    m_signaling->requestFileContents(id.m_userId, id.m_name, m_offsets[id], fileInfo.m_size > m_maxFragmentSize ? m_maxFragmentSize : fileInfo.m_size);
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
    auto id = getCurrentFileId();
    if (id == FileId())
        return;
    m_fileNames.erase(id);
    if (id.m_action == FileActionType::Receive)
    {
        auto fileName = getFileName(id);
        QFile::remove(fileName);
        m_receivingFiles.erase(fileName);
    }

    delete m_ui->filesListWidget->takeItem(m_ui->filesListWidget->currentRow());
    updateButtons();
}

void FileForm::showFiles(int a_tabIndex)
{
    m_ui->filesListWidget->clear();
    if (a_tabIndex == -1)
        return;
    auto userId = getUserId(a_tabIndex);
    for (auto &fileId : m_fileNames)
    {
        if (a_tabIndex != 0 && fileId.first.m_userId != userId)
            continue;
        auto icon = fileId.first.m_action == FileActionType::Receive ? QIcon(":/download") : QIcon(":/upload");
        auto item = new QListWidgetItem(icon, getItemText(fileId.first, a_tabIndex != 0));
        item->setData(Qt::UserRole, fileId.first.toString());
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
        auto id = FileId::fromString(item->data(Qt::UserRole).toString());
        auto &fileInfo = getReceivingFileInfo(getFileName(id));
        auto status = FileInfo::Status::Unknown;
        if (fileInfo.isValid())
            status = fileInfo.m_status;
        QIcon icon;
        if (m_blinkState && status == FileInfo::Status::Pending)
            icon = ResourceHolder::get().getFileIcon();
        else if (m_blinkState && status == FileInfo::Status::Paused)
            icon = QIcon(":pause");
        else if (m_blinkState && status == FileInfo::Status::Error)
            icon = QIcon(":error");
        else
            icon = id.m_action == FileActionType::Send ? QIcon(":upload") : QIcon(":download");
        item->setIcon(icon);
    }
    m_blinkState = !m_blinkState;
}

QString FileForm::createReceivingFileName(const QString &a_user, const QString &a_name)
{
    return QString("%1/files/%2/%3").arg(QDir::currentPath()).arg(a_user).arg(a_name);
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

QString FileForm::getFileName(const FileId &a_fileId) const
{
    auto it = m_fileNames.find(a_fileId);
    if (it == m_fileNames.end())
        return QString();
    return it->second;
}

const FileId &FileForm::getFileId(const QString &a_fileName) const
{
    auto it = std::find_if(m_fileNames.begin(), m_fileNames.end(), [&a_fileName](auto& a_fileInfo)
        {
            return a_fileInfo.second == a_fileName;
        });
    if (it == m_fileNames.end())
    {
        static FileId nullId;
        return nullId;
    }
    return it->first;
}

const FileInfo &FileForm::getReceivingFileInfo(const QString &a_fileName) const
{
    return const_cast<FileForm &>(*this).getReceivingFileInfo(a_fileName);
}

FileInfo &FileForm::getReceivingFileInfo(const QString &a_fileName)
{
    auto it = m_receivingFiles.find(a_fileName);
    if (it == m_receivingFiles.end())
    {
        static FileInfo nullInfo;
        return nullInfo;
    }
    return it->second;
}

FileId FileForm::getCurrentFileId()
{
    auto item = m_ui->filesListWidget->currentItem();
    if (item == nullptr)
        return {};
    return FileId::fromString(item->data(Qt::UserRole).toString());
}

QString FileForm::getCurrentFileName()
{
    return getFileName(getCurrentFileId());
}

FileInfo &FileForm::getCurrentReceivingFileInfo()
{
    return getReceivingFileInfo(getCurrentFileName());
}

QListWidgetItem *FileForm::getItem(const FileId &a_fileId)
{
    for (int i = 0; i < m_ui->filesListWidget->count(); i++)
    {
        auto item = m_ui->filesListWidget->item(i);
        auto id = FileId::fromString(item->data(Qt::UserRole).toString());
        if (id == a_fileId)
            return item;
    }
    return nullptr;
}

QString FileForm::getItemText(const FileId &a_fileId, bool a_printUserName)
{
    auto fileName = getFileName(a_fileId);
    QString userName = m_signaling->getUserName(a_fileId.m_userId);
    size_t size = 0;
    auto &fileInfo = getReceivingFileInfo(fileName);
    if (fileInfo.isValid())
    {
        size = fileInfo.m_size;
        auto sizeAndUnits = getShortFileSize(size);
        if (fileInfo.m_status != FileInfo::Status::Started && fileInfo.m_status != FileInfo::Status::Paused)
        {
            if (a_printUserName)
                return QString("%1: %2 (%3 %4)").arg(userName).arg(a_fileId.m_name).arg(sizeAndUnits.first).arg(sizeAndUnits.second);
            return QString("%1 (%2 %3)").arg(a_fileId.m_name).arg(sizeAndUnits.first).arg(sizeAndUnits.second);
        }
    }
    else
        size = (size_t)QFileInfo(fileName).size();
    auto offset = m_offsets[a_fileId];
    auto percents = (int)(((double)offset / size) * 100);
    auto offsetAndUnits = getShortFileSize(offset);
    auto sizeAndUnits = getShortFileSize(size);
    if (a_printUserName)
        return QString("%1: %2 (%3 %4 of %5 %6, %7 %)").arg(userName).arg(a_fileId.m_name)
            .arg(offsetAndUnits.first).arg(offsetAndUnits.second).arg(sizeAndUnits.first).arg(sizeAndUnits.second).arg(percents);
    return QString("%1 (%2 %3 of %4 %5, %6 %)").arg(a_fileId.m_name)
        .arg(offsetAndUnits.first).arg(offsetAndUnits.second).arg(sizeAndUnits.first).arg(sizeAndUnits.second).arg(percents);
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
