#include <QFileInfo>
#include <QDir>
#include "file_signaling.h"
#include "messenger_signaling.h"

//-------------------------------------------------------------------------------------------------
struct FileInfoSignal : AttributeContainer
{
    ATTRIBUTE(QString, sender);
    ATTRIBUTE(QString, name);
    ATTRIBUTE(QDateTime, modification_date);
    ATTRIBUTE(size_t, size);

    FileInfoSignal(const QString &a_sender, const QString &a_name, const QDateTime &a_modificationDate, size_t a_size)
    {
        set_sender(a_sender);
        set_name(a_name);
        set_modification_date(a_modificationDate);
        set_size(a_size);
    }

    explicit FileInfoSignal(const QVariant &a_value) : AttributeContainer(a_value) {}

    static constexpr char g_signalName[]{ "FileInfo" };
};

//-------------------------------------------------------------------------------------------------
struct FileContentsSignal : AttributeContainer
{
    ATTRIBUTE(QString, sender);
    ATTRIBUTE(QString, name);
    ATTRIBUTE(size_t, offset);
    ATTRIBUTE(size_t, size);
    ATTRIBUTE(QByteArray, contents);

    // для отправки фрагмента
    FileContentsSignal(const QString &a_sender, const QString &a_name, size_t a_offset, const QByteArray &a_contents)
    {
        set_sender(a_sender);
        set_name(a_name);
        set_offset(a_offset);
        set_size(a_contents.size());
        set_contents(a_contents);
    }

    // для запроса на фрагмент
    FileContentsSignal(const QString &a_sender, const QString &a_name, size_t a_offset, size_t a_size)
    {
        set_sender(a_sender);
        set_name(a_name);
        set_offset(a_offset);
        set_size(a_size);
    }

    explicit FileContentsSignal(const QVariant &a_value) : AttributeContainer(a_value) {}

    static constexpr char g_signalName[]{ "FileContents" };
};

//-------------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------------
FileSignaling::FileSignaling(std::shared_ptr<Signaling> a_signaling)
{
    m_signaling = a_signaling;
    connect(m_signaling.get(), &Signaling::signalReceived, this, &FileSignaling::onSignalReceived);
}

QString FileSignaling::getId() const
{
    return m_id;
}

void FileSignaling::setId(const QString &a_id)
{
    m_signaling->unsubscribe(getSignalName(FileInfoSignal::g_signalName, m_id));
    m_signaling->unsubscribe(getSignalName(FileContentsSignal::g_signalName, m_id));
    m_id = a_id;
    m_signaling->subscribe(getSignalName(FileInfoSignal::g_signalName, m_id));
    m_signaling->subscribe(getSignalName(FileContentsSignal::g_signalName, m_id));
}

bool FileSignaling::sendFile(const QString &a_receiver, const QString &a_fileName)
{
    QFileInfo fileInfo(a_fileName);
    if (!fileInfo.exists())
        return false;
    auto name = fileInfo.fileName();
    FileId fileId{ FileActionType::Send, a_receiver, name };
    if (!getFileName(fileId).isNull())
        return false; // файл с таким именем уже отправлен/отправляется
    m_fileNames[fileId] = a_fileName;

    // идентификатор ресурса - короткое имя файла
    // передача файлов с одинаковыми короткими именами, но разными полными именами невозможна
    m_signaling->sendSignal(getSignalName(FileInfoSignal::g_signalName, a_receiver), FileInfoSignal(m_id, name, fileInfo.lastModified(), (size_t)fileInfo.size()).toQVariant());
    return true;
}

void FileSignaling::receiveFile(const QString &a_sender, const QString &a_name)
{
    FileId fileId{ FileActionType::Receive, a_sender, a_name };
    auto fileName = getFileName(fileId);
    auto &fileInfo = getReceivingFileInfoRef(fileName);
    if (!fileInfo.isValid())
        return;
    // перед началом приема файла удалим существующий файл
    if (fileInfo.m_status == FileInfo::Status::Pending &&
        QFileInfo(fileName).exists() &&
        !QFile::remove(fileName))
        return;
    // также создадим отсутствующие каталоги
    if (fileInfo.m_status == FileInfo::Status::Pending &&
        !QDir().mkpath(QFileInfo(fileName).path()))
        return;
    fileInfo.m_status = FileInfo::Status::Started;
    requestFileContents(fileId.m_userId, fileId.m_name, m_offsets[fileId], fileInfo.m_size > m_maxFragmentSize ? m_maxFragmentSize : fileInfo.m_size);
}

void FileSignaling::renameFileName(const QString &a_oldFileName, const QString &a_newFileName)
{
    auto fileId = getFileId(a_oldFileName);
    if (fileId == FileId() || fileId.m_action == FileActionType::Send)
        return;
    // имя файла FileId.m_name определено отправителем и не может тут измениться
    m_fileNames[fileId] = a_newFileName;
    auto fileInfo = m_receivingFiles[a_oldFileName];
    m_receivingFiles.erase(a_oldFileName);
    m_receivingFiles[a_newFileName] = fileInfo;
    QFile::rename(a_oldFileName, a_newFileName);
}

void FileSignaling::pauseReceivingFile(const QString &a_sender, const QString &a_name)
{
    auto &fileInfo = getReceivingFileInfoRef(getFileName(FileId{ FileActionType::Receive, a_sender, a_name }));
    if (!fileInfo.isValid())
        return;
    fileInfo.m_status = FileInfo::Status::Paused;
}

void FileSignaling::cancelReceivingFile(const QString &a_sender, const QString &a_name)
{
    FileId id{ FileActionType::Receive, a_sender, a_name };
    auto fileName = getFileName(id);
    auto &fileInfo = getReceivingFileInfoRef(fileName);
    if (!fileInfo.isValid())
        return;
    QFile::remove(fileName);
    m_offsets.erase(id);
    fileInfo.m_status = FileInfo::Status::Pending;
}

void FileSignaling::removeFile(const QString &a_userId, const QString &a_name)
{
    FileId id{ FileActionType::Receive, a_userId, a_name };
    auto fileName = getFileName(id);
    if (!fileName.isNull())
    {
        m_fileNames.erase(id);
        m_offsets.erase(id);
        QFile::remove(fileName);
        m_receivingFiles.erase(fileName);
        return;
    }
    id = FileId{ FileActionType::Send, a_userId, a_name };
    fileName = getFileName(id);
    if (fileName.isNull())
        return;
    m_fileNames.erase(id);
}

QString FileSignaling::getFileName(const FileId &a_fileId) const
{
    auto it = m_fileNames.find(a_fileId);
    if (it == m_fileNames.end())
        return QString();
    return it->second;
}

QStringList FileSignaling::getFileNames(const QString &a_userId) const
{
    QStringList result;
    for (auto &fileId : m_fileNames)
        if (a_userId.isNull() || fileId.first.m_userId == a_userId)
            result.append(fileId.second);
    return result;
}

const FileId &FileSignaling::getFileId(const QString &a_fileName) const
{
    auto it = std::find_if(m_fileNames.begin(), m_fileNames.end(), [&a_fileName](auto &a_fileInfo)
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

const FileInfo &FileSignaling::getReceivingFileInfo(const QString &a_fileName) const
{
    return const_cast<FileSignaling &>(*this).getReceivingFileInfoRef(a_fileName);
}

// private slots:
void FileSignaling::onSignalReceived(QString a_signal, QVariant a_value)
{
    tryHandleSignal<FileInfoSignal>(a_signal, a_value) ||
        tryHandleSignal<FileContentsSignal>(a_signal, a_value);
}

// private:
QString FileSignaling::getSignalName(const QString &a_prefix, const QString &a_id)
{
    return QString("%1_%2").arg(a_prefix).arg(a_id);
}

QString FileSignaling::createReceivingFileName(const QString &a_user, const QString &a_name)
{
    auto result = QString("%1/files/%2/%3").arg(QDir::currentPath()).arg(a_user).arg(a_name);
    if (!QFile::exists(result))
        return result;
    auto baseName = QFileInfo(a_name).baseName();
    auto suffix = QFileInfo(a_name).suffix();
    for (int i = 1; QFile::exists(result); i++)
        result = QString("%1/files/%2/%3(%4).%5").arg(QDir::currentPath()).arg(a_user).arg(baseName).arg(i).arg(suffix);
    return result;
}

FileInfo &FileSignaling::getReceivingFileInfoRef(const QString &a_fileName)
{
    auto it = m_receivingFiles.find(a_fileName);
    if (it == m_receivingFiles.end())
    {
        static FileInfo nullInfo;
        return nullInfo;
    }
    return it->second;
}

template<typename T> bool FileSignaling::tryHandleSignal(const QString &a_signal, const QVariant &a_value)
{
    if (a_signal.left(QString(T::g_signalName).length()) != T::g_signalName)
        return false;
    handleSignal<T>(T(a_value));
    return true;
}

template<typename T> void FileSignaling::handleSignal(const T &)
{
    throw std::exception("FileSignaling.handleSignal: undefined signal handler");
}

template<> void FileSignaling::handleSignal(const FileInfoSignal &a_data)
{
    auto sender = a_data.get_sender();
    auto name = a_data.get_name();
    auto fileName = createReceivingFileName(sender, name);
    if (getReceivingFileInfo(fileName).isValid())
        return; // файл с таким именем уже принимается/принят

    m_receivingFiles[fileName] = FileInfo{ FileInfo::Status::Pending, a_data.get_modification_date(), a_data.get_size() };

    FileId fileId{ FileActionType::Receive, sender, name };
    m_fileNames[fileId] = fileName;

    emit fileAboutToReceive(sender, name);
}

template<> void FileSignaling::handleSignal(const FileContentsSignal &a_data)
{
    auto sender = a_data.get_sender();
    auto name = a_data.get_name();
    auto offset = a_data.get_offset();
    auto size = a_data.get_size();
    FileId fileId{ FileActionType::Send, sender, name };
    auto fileName = getFileName(fileId);
    if (!fileName.isNull())
    {
        // отправка
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            // файл удален
            sendFileContents(sender, name, offset, QByteArray());
            return;
        }
        if (!file.seek(offset))
        {
            // неправильное смещение
            sendFileContents(sender, name, offset, QByteArray());
            return;
        }
        auto contents = file.read(size);
        if (contents.size() != size)
        {
            // неправильный размер
            sendFileContents(sender, name, offset, QByteArray());
            return;
        }
        sendFileContents(sender, name, offset, contents);
        //m_offsets[fileId] = offset + size; // обновляем смещение для индикатора выполнения
        emit fileFragmentSent(sender, name, offset, size);
    }
    else
    {
        // прием
        fileId = FileId{ FileActionType::Receive, sender, name };
        fileName = getFileName(fileId);
        if (fileName.isNull())
            return; // файл не запрашивался
        auto &fileInfo = getReceivingFileInfoRef(fileName);
        if (!fileInfo.isValid())
            return;
        if (fileInfo.m_status != FileInfo::Status::Started)
            return; // пользователь отказался от приема файла
        if (size == 0)
        {
            // отправитель вернул ошибку
            fileInfo.m_status = FileInfo::Status::Error;
            return;
        }
        auto &offset = m_offsets[fileId];
        if (offset != offset)
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
        file.write(a_data.get_contents());
        size = a_data.get_contents().size();
        offset += a_data.get_contents().size();

        // запрашиваем следующий фрагмент
        if (fileInfo.m_status == FileInfo::Status::Started)
        {
            auto nextFragmentSize = std::min(fileInfo.m_size - offset, m_maxFragmentSize);
            if (nextFragmentSize != 0)
                requestFileContents(sender, name, offset, nextFragmentSize);
            else
            {
                fileInfo.m_status = FileInfo::Status::Finished; // прием завершен
                file.setFileTime(fileInfo.m_modificationDate, QFileDevice::FileModificationTime);
            }
        }

        emit fileFragmentReceived(sender, name, offset - size, size);
    }
}

void FileSignaling::sendFileContents(const QString &a_receiver, QString a_name, size_t a_offset, const QByteArray &a_contents)
{
    m_signaling->sendSignal(getSignalName(FileContentsSignal::g_signalName, a_receiver), FileContentsSignal(m_id, a_name, a_offset, a_contents).toQVariant());
}

void FileSignaling::requestFileContents(const QString &a_receiver, QString a_name, size_t a_offset, size_t a_size)
{
    m_signaling->sendSignal(getSignalName(FileContentsSignal::g_signalName, a_receiver), FileContentsSignal(m_id, a_name, a_offset, a_size).toQVariant());
}
