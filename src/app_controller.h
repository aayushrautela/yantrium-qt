#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QString>

class AppController : public QObject
{
    Q_OBJECT
    
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() = default;
    
    // TODO: Add application-wide properties and methods
    
signals:
    // TODO: Add signals for QML communication
    
public slots:
    // TODO: Add slots for QML interaction
    
private:
    // TODO: Add private members
};

#endif // APP_CONTROLLER_H

