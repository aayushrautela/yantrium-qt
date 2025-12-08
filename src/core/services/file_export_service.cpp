#include "file_export_service.h"
#include "logging_service.h"
#include "logging_service.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
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
            LoggingService::logWarning("FileExportService", QString("Failed to create directory: %1").arg(dir.path()));
            QString errorMsg = QString("Failed to create directory: %1").arg(dir.path());
            LoggingService::report(errorMsg, "FILE_ERROR", "FileExportService");
            emit error(errorMsg);
            return false;
        }
    }
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LoggingService::logWarning("FileExportService", QString("Failed to open file for writing: %1").arg(filePath));
        LoggingService::logWarning("FileExportService", QString("Error: %1").arg(file.errorString()));
        emit error(QString("Failed to open file: %1").arg(file.errorString()));
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);
    out << content;
    file.close();
    
    LoggingService::logDebug("FileExportService", QString("Successfully wrote file: %1").arg(filePath));
    LoggingService::logDebug("FileExportService", QString("File size: %1 bytes").arg(QFileInfo(filePath).size()));
    
    emit fileWritten(filePath);
    return true;
}

QString FileExportService::getDocumentsPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    LoggingService::logDebug("FileExportService", QString("Documents path: %1").arg(path));
    return path;
}

QString FileExportService::getDownloadsPath()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (path.isEmpty()) {
        // Fallback to Documents if Downloads doesn't exist
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    LoggingService::logDebug("FileExportService", QString("Downloads path: %1").arg(path));
    return path;
}

