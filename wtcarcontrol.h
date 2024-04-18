#ifndef CWTCARCONTROL_H
#define CWTCARCONTROL_H

#include <QLibrary>
#include "wtcar.h"
#include <Windows.h>
#include <QMessageBox>

class CWTCarControl
{
public:
    int Car_Connect()
    {
        if (m_pCreate)
            return m_net->Car_Connect();
        return 1;
    }

    void Car_SetGasCallBackFuc(CASCALCALL pcb)
    {
        if (m_pCreate)
            m_net->Car_SetGasCallBackFuc(pcb);
    }

    void Load(int type)
    {
        QString strPath;
        if(type == 1)
            strPath = "CarPassHK";
        else if(type == 2)
            strPath = "CarPassNF";
        else if(type == 3)
            strPath = "CarPassNF2";

        m_hModuleDll.setFileName(strPath);
        if (m_hModuleDll.load())
        {
            m_pCreate = (pfunCreateNet)m_hModuleDll.resolve("CreateCar");
            if (m_pCreate)
                m_net = m_pCreate();
        }
        else
        {
            QMessageBox::warning(nullptr, strPath, QStringLiteral("未安装WTCar模块，请联系管理员！"));
            m_pCreate = NULL;
            m_net = NULL;
        }
    }

    CWTCarControl()
    {

    }
    virtual ~CWTCarControl()
    {
        if (m_hModuleDll.isLoaded())
        {
            m_hModuleDll.unload();
        }
    }

protected:
    WTCar* m_net;
    QLibrary m_hModuleDll;
    typedef WTCar* (*pfunCreateNet)();
    pfunCreateNet m_pCreate;
};

#endif // CWTCARCONTROL_H
