#ifndef WTLED_H
#define WTLED_H
#include <QJsonArray>

class wtled
{
public:
    virtual ~wtled(){}
    virtual void Work() = 0;
    virtual void UpdateLed(char *CarNo, char *ret) = 0;
    virtual void ResetLed() = 0;
    virtual void UpdateLedEx(QJsonArray CarMsg) = 0;
    virtual void Close() = 0;
};

#endif // WTLED_H
