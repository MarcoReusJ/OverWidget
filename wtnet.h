#pragma once

class WTNet
{
public:
    virtual ~WTNet(){}
    virtual int InitData(char * strIP, long Port) = 0;
    virtual int Start() = 0;
};

