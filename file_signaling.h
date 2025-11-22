#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "signaling.h"

// Информация о принимаемом файле.
struct FileInfo
{
    enum class Status
    {
        Unknown,
        Pending,
        Started,
        Paused,
        Finished,
        Error
    };

    bool isValid() const
    {
        return m_status != Status::Unknown;
    }

    Status m_status;
    QDateTime m_modificationDate;
    // размер файла
    size_t m_size = 0;
};

enum class FileActionType
{
    Send,
    Receive
};

struct FileId
{
    bool operator==(const FileId &a_id) const;
    bool operator<(const FileId &a_id) const;

    FileActionType m_action;
    QString m_userId;
    QString m_name; // короткое имя файла
};

class FileSignaling : public QObject
{
    Q_OBJECT

public:
    FileSignaling(std::shared_ptr<Signaling> a_signaling);

    QString getId() const;
    void setId(const QString &a_id);
    bool sendFile(const QString &a_receiver, const QString &a_fileName);
    void receiveFile(const QString &a_sender, const QString &a_name);
    void renameFileName(const QString &a_oldFileName, const QString &a_newFileName);
    void pauseReceivingFile(const QString &a_receiver, const QString &a_name);
    void cancelReceivingFile(const QString &a_receiver, const QString &a_name);
    void removeFile(const QString &a_userId, const QString &a_name);
    QString getFileName(const FileId &a_fileId) const;
    QStringList getFileNames(const QString &a_userId) const;
    const FileId &getFileId(const QString &a_fileName) const;
    const FileInfo &getReceivingFileInfo(const QString &a_fileName) const;

signals:
    void fileAboutToReceive(QString a_sender, QString a_name);
    void fileFragmentSent(QString a_receiver, QString a_name, size_t a_offset, size_t a_size);
    void fileFragmentReceived(QString a_sender, QString a_name, size_t a_offset, size_t a_size);

private slots:
    void onSignalReceived(QString a_signal, QVariant a_value);

private:
    static QString getSignalName(const QString &a_prefix, const QString &a_id);
    static QString createReceivingFileName(const QString &a_user, const QString &a_name);

    FileInfo &getReceivingFileInfoRef(const QString &a_fileName);
    template<typename T> bool tryHandleSignal(const QString &a_signal, const QVariant &a_value);
    template<typename T> void handleSignal(const T &a_signal);
    void sendFileContents(const QString &a_receiver, QString a_name, size_t a_offset, const QByteArray &a_contents);
    void requestFileContents(const QString &a_receiver, QString a_name, size_t a_offset, size_t a_size);

    std::shared_ptr<Signaling> m_signaling;
    QString m_id;
    // отправляемые и принимаемые файлы: ид - абсолютное имя
    std::map<FileId, QString> m_fileNames;
    // позиции для запроса очередных фрагментов
    std::map<FileId, size_t> m_offsets;
    // абсолютное имя - информация о получении
    std::map<QString, FileInfo> m_receivingFiles;
    size_t m_maxFragmentSize = 1024 * 1024;
};
