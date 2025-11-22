#pragma once

#include <QWidget>
#include <QFile>
#include "messenger_signaling.h"
#include "file_signaling.h"

namespace Ui
{
    class FileForm;
}

class QListWidgetItem;

class FileForm : public QWidget
{
    Q_OBJECT

public:
    FileForm(std::shared_ptr<MessengerSignaling> a_messengerSignaling, std::shared_ptr<FileSignaling> a_fileSignaling, QWidget *a_parent = nullptr);

    ~FileForm();

    bool sendFile(const QString &a_receiver, const QString &a_fileName);
    bool hasPendingFiles();
    bool hasPendingFiles(const QString &a_sender);
    void addUser(const QString &a_id);

private slots:
    void renameUser(QString a_id, QString a_name);
    void onFileAboutToReceive(QString a_sender, QString a_name);
    void onFileFragmentSent(QString a_sender, QString a_name, size_t a_offset, size_t a_size);
    void onFileFragmentReceived(QString a_sender, QString a_name, size_t a_offset, size_t a_size);
    void onFileListWidgetCurrentItemChanged(QListWidgetItem *a_current, QListWidgetItem *a_previous);
    void onSetFileNameToolButtonClicked();
    void onStartToolButtonClicked();
    void onPauseToolButtonClicked();
    void onCancelToolButtonClicked();
    void onRestartToolButtonClicked();
    void onOpenFolderToolButtonClicked();
    void onRemoveToolButtonClicked();
    void showFiles(int a_tabIndex);
    void changeIcons();

private:
    static std::pair<QString, QString> getShortFileSize(size_t a_size);
    static FileId fileIdFromString(const QString &a_id);
    static QString fileIdToString(const FileId &a_id);

    void changeEvent(QEvent *a_event) override;
    FileId getCurrentFileId();
    QString getCurrentFileName();
    const FileInfo &getCurrentReceivingFileInfo();
    QListWidgetItem *getItem(const FileId &a_fileId);
    QString getItemText(const FileId &a_fileId, bool a_printUserName);
    void updateButtons();
    int getTabIndex(const QString &a_id);
    QString getUserId(int a_tabIndex);
    bool canAddItem(const QString &a_userId);
    void eraseCurrentPendingSender();

    Ui::FileForm *m_ui = nullptr;
    std::shared_ptr<MessengerSignaling> m_signaling;
    std::shared_ptr<FileSignaling> m_fileSignaling;
    std::set<QString> m_pendingSenders; // отправители, уведомления от которых не прочитаны
    // позиции отправляемых/получаемых файлов для индикаторов выполнения
    std::map<FileId, size_t> m_offsets;
    QTimer m_blinkTimer;
    bool m_blinkState = false;
};
