#ifndef FILE_EXPORT_SERVICE_H
#define FILE_EXPORT_SERVICE_H

#include <QObject>
#include <QString>

class FileExportService : public QObject
{
    Q_OBJECT

public:
    explicit FileExportService(QObject* parent = nullptr);
    
    Q_INVOKABLE bool writeTextFile(const QString& filePath, const QString& content);
    Q_INVOKABLE QString getDocumentsPath();
    Q_INVOKABLE QString getDownloadsPath();

signals:
    void fileWritten(const QString& filePath);
    void error(const QString& message);
};

#endif // FILE_EXPORT_SERVICE_H


