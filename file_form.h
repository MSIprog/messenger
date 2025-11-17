#pragma once

#include <QWidget>
#include <QFile>
#include "messenger_signaling.h"

namespace Ui
{
    class FileForm;
}

class QListWidgetItem;

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
    static FileId fromString(const QString& a_id);
    QString toString() const;
    bool operator==(const FileId &a_id) const;
    bool operator<(const FileId &a_id) const;

    FileActionType m_action;
    QString m_userId;
    QString m_name;
};

class FileForm : public QWidget
{
    Q_OBJECT

public:
    FileForm(std::shared_ptr<MessengerSignaling> a_signaling, QWidget *a_parent = nullptr);

    ~FileForm();

    bool sendFile(const QString &a_receiver, const QString &a_fileName);
    bool hasPendingFiles();
    bool hasPendingFiles(const QString &a_sender);
    void addUser(const QString &a_id);

private slots:
    void onFileInfoReceived(QString a_sender, QString a_name, QDateTime a_modificationDate, size_t a_size);
    void onFileContentsReceived(QString a_sender, QString a_name, size_t a_offset, size_t a_size, QByteArray a_contents);
    void onFileListWidgetCurrentItemChanged(QListWidgetItem *a_current, QListWidgetItem *a_previous);
    void onSetFolderToolButtonClicked();
    void onStartToolButtonClicked();
    void onPauseToolButtonClicked();
    void onCancelToolButtonClicked();
    void onRestartToolButtonClicked();
    void onOpenFolderToolButtonClicked();
    void onRemoveToolButtonClicked();
    void showFiles(int a_tabIndex);
    void changeIcons();

private:
    static QString createReceivingFileName(const QString &a_user, const QString &a_name);
    std::pair<QString, QString> getShortFileSize(size_t a_size);

    void changeEvent(QEvent *a_event) override;
    QString getFileName(const FileId &a_fileId) const;
    const FileId &getFileId(const QString &a_fileName) const;
    const FileInfo &getReceivingFileInfo(const QString &a_fileName) const;
    FileInfo &getReceivingFileInfo(const QString &a_fileName);
    FileId getCurrentFileId();
    QString getCurrentFileName();
    FileInfo &getCurrentReceivingFileInfo();
    QListWidgetItem *getItem(const FileId &a_fileId);
    QString getItemText(const FileId &a_fileId, bool a_printUserName);
    void updateButtons();
    int getTabIndex(const QString &a_id);
    QString getUserId(int a_tabIndex);
    bool canAddItem(const QString &a_userId);
    void eraseCurrentPendingSender();

    Ui::FileForm *m_ui = nullptr;
    std::shared_ptr<MessengerSignaling> m_signaling;
    // отправляемые и принимаемые файлы: ид - абсолютное имя
    std::map<FileId, QString> m_fileNames;
    // позиции для отправки/запроса очередных фрагментов
    std::map<FileId, size_t> m_offsets;
    // абсолютное имя - информация о получении
    std::map<QString, FileInfo> m_receivingFiles;
    size_t m_maxFragmentSize = 1024 * 1024;
    std::set<QString> m_pendingSenders; // отправители, уведомления от которых не прочитаны
    QTimer m_blinkTimer;
    bool m_blinkState = false;
};
