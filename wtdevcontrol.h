#ifndef WTDEVCONTROL_H
#define WTDEVCONTROL_H
#include <QLibrary>
#include "wtdev.h"
#include <Windows.h>
#include <QMessageBox>

class CWTDevControl
{
public:
    int Work()
    {
        if (m_pCreate)
            return m_net->Work();
        return 1;
    }

    int CarArrive(int lane, bool bgo)
    {
        if (m_pCreate)
            return m_net->CarArrive(lane, bgo);
        return -1;
    }

    int Datalog(bool save)
    {
        if (m_pCreate)
            return m_net->Datalog(save);
        return -1;
    }

    void SetCallback(CARRETCALL pcb)
    {
        if (m_pCreate)
            return m_net->SetCallback(pcb);
    }

    void Load(int type)
    {
        QString strPath;
        if(type == 1)
            strPath = "WTDevSBSY";
        else if(type == 2)
            strPath = "WTDevSBPB";
        else if(type == 3)
            strPath = "WTDevSB";

        m_hModuleDll.setFileName(strPath);
        if (m_hModuleDll.load())
        {
            m_pCreate = (pfunCreateNet)m_hModuleDll.resolve("CreateDev");
            if (m_pCreate)
                m_net = m_pCreate();
        }
        else
        {
            QMessageBox::warning(nullptr, strPath, QStringLiteral("未安装WTDev模块，请联系管理员！"));
            m_pCreate = NULL;
            m_net = NULL;
        }
    }

    CWTDevControl()
    {

    }
    virtual ~CWTDevControl()
    {
        if (m_hModuleDll.isLoaded())
        {
            m_hModuleDll.unload();
        }
    }

protected:
    WTDev *m_net;
    QLibrary m_hModuleDll;
    typedef WTDev* (*pfunCreateNet)();
    pfunCreateNet m_pCreate;
};

#endif // WTDEVCONTROL_H
