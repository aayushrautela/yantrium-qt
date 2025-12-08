#include "file_export_service.h"
#include "error_service.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QStringConverter>

FileExportService::FileExportService(QObject* parent)
    : QObject(parent)
{
}

bool FileExportService::writeTextFile(const QString& filePath, const QString& content)
{
    QFile file(filePath);
    
    // Create directory if it doesn't exist
    QFileInfo fileInfo(file);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "[FileExportService] Failed to create directory:" << dir.path();
            QString errorMsg = QString("Failed to create directory: %1").arg(dir.path());
            ErrorService::report(errorMsg, "FILE_ERROR", "FileExportService");
            emit error(errorMsg);
            return false;
        }
    }
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[FileExportService] Failed to open file for writing:" << filePath;
        qWarning() << "[FileExportService] Error:" << file.errorString();
        emit error(QString("Failed to open file: %1").arg(file.errorString()));
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);
    out << content;
    file.close();
    
    qDebug() << "[FileExportService] Successfully wrote file:" << filePath;
    qDebug() << "[FileExportService] File size:" << QFileInfo(filePath).size() << "bytes";
    
    emit fileWritten(filePath);
    return true;
}

QString FileExportService::getDocumentsPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    qDebug() << "[FileExportService] Documents path:" << path;
    return path;
}

QString FileExportService::getDownloadsPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (path.isEmpty()) {
        // Fallback to Documents if Downloads doesn't exist
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    qDebug() << "[FileExportService] Downloads path:" << path;
    return path;
}

