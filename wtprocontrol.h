#ifndef WTPROCONTROL_H
#define WTPROCONTROL_H
#include <QLibrary>
#include "profilesdk.h"
#include <Windows.h>
#include <QMessageBox>

class CWTProControl
{
public:
    int Connect(char *IP, int iPort)
    {
        if (m_pCreate)
            return m_pro->Connect(IP,iPort);
        return 1;
    }

    void Clean()
    {
        if (m_pCreate)
            return m_pro->Clean();
    }

    void SetCallBackFuc(PROFILECALL pcb)
    {
        if (m_pCreate)
            return m_pro->SetCallBackFuc(pcb);
    }

    void SetStateCallBackFuc(PROFILESTATUSCALL pcb)
    {
        if (m_pCreate)
            return m_pro->SetStateCallBackFuc(pcb);
    }

    void Load(int type)
    {
        QString strPath;
        if(type == 1)
            strPath = "ProfileSdkDG";
        else
            strPath = "ProfileSdkYTL";

        m_hModuleDll.setFileName(strPath);
        if (m_hModuleDll.load())
        {
            m_pCreate = (pfunCreatePro)m_hModuleDll.resolve("CreatePro");
            if (m_pCreate)
                m_pro = m_pCreate();
        }
        else
        {
            QMessageBox::warning(nullptr, strPath, QStringLiteral("未安装ProfileSdk模块，请联系管理员！"));
            m_pCreate = NULL;
            m_pro = NULL;
        }
    }

    CWTProControl()
    {

    }
    virtual ~CWTProControl()
    {
        if (m_hModuleDll.isLoaded())
        {
            m_hModuleDll.unload();
        }
    }

protected:
    ProfileSdk *m_pro;
    QLibrary m_hModuleDll;
    typedef ProfileSdk* (*pfunCreatePro)();
    pfunCreatePro m_pCreate;
};

#endif // WTPROCONTROL_H
