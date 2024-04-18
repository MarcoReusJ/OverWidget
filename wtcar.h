#ifndef WTCARCONTROL_H
#define WTCARCONTROL_H

typedef void(*CASCALCALL)(int Lane, int Type);

class WTCar
{
public:
    virtual ~WTCar(){}
    virtual int Car_Connect() = 0;
    virtual void Car_SetGasCallBackFuc(CASCALCALL pcb) = 0;

};

#endif // WTCARCONTROL_H
