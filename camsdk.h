#pragma once

typedef void(*CAMCALCALL)(int Lane, char *filename, int pictype, char *carno, char *pcolor, char *cartype);

class camsdk
{
public:
    virtual ~camsdk(){}
    virtual int StartWork() = 0;
    virtual void Car_SetCamCallBackFuc(CAMCALCALL pcb) = 0;
    virtual int DownFile(char *cartime, int lane, int dataid, char *carno, char *pUsr, int et1, int et2) = 0;
};

