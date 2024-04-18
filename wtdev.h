#ifndef WTDEV_H
#define WTDEV_H

typedef void(*CARRETCALL)(const char* msg, int iUser, void *pUser);

class WTDev
{
public:
    virtual ~WTDev(){}
    virtual int Work() = 0;
    virtual int Datalog(bool save) = 0;
    virtual int CarArrive(int lane, bool bgo) = 0;
    virtual void SetCallback(CARRETCALL pcb) = 0;

};


#endif // WTDEV_H
