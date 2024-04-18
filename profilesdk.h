#ifndef PROFILESDK_H
#define PROFILESDK_H

typedef bool (*PROFILECALL)(int L,int W,int H,int lane,long etc, int etc2);

typedef void(*PROFILESTATUSCALL)(int Status);

class ProfileSdk
{

public:
    virtual ~ProfileSdk(){}

    virtual int Connect(char *IP, int iPort) = 0;

    virtual void SetCallBackFuc(PROFILECALL pcb) = 0;

    virtual void SetStateCallBackFuc(PROFILESTATUSCALL pcb) = 0;

    virtual void Clean() = 0;
};

#endif // PROFILESDK_H
