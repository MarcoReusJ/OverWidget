#ifndef WTNETCONTROL_H
#define WTNETCONTROL_H
#include <QLibrary>
#include "wtnet.h"
#include <Windows.h>
#include <QMessageBox>

class CWTNetControl
{
public:
    int InitData(char * strIP, long Port)			//初始化OBD服务信息 设置服务端的IP地址 端口号等信息
    {
        if (m_pCreate)
            return m_net->InitData(strIP, Port);
    }
    int Start()							//启动服务端Socket通讯
    {
        if (m_pCreate)
            return m_net->Start();
    }

    void Load(int type)
    {
        QString strPath;
        if(type == 2)
            strPath = "WTNetHK";
        else if(type == 3)
            strPath = "WTNetST";
        else if(type == 1)
            strPath = "WTNetSB";
        else if(type == 4)
            strPath = "WTNetDW";
        else if(type == 5)
            strPath = "WTNetCQ";
        else if(type == 6)
            strPath = "WTNetKS";
        else if(type == 7)
            strPath = "WTNetAD";

        m_hModuleDll.setFileName(strPath);
        if (m_hModuleDll.load())
        {
            m_pCreate = (pfunCreateNet)m_hModuleDll.resolve("CreateWT");
            if (m_pCreate)
            {
                m_net = m_pCreate();
            }
        }
        else
        {
            QMessageBox::warning(nullptr, strPath, QStringLiteral("未安装WTNet模块，请联系管理员！"));
            m_pCreate = NULL;
            m_net = NULL;
        }
    }

    CWTNetControl()
    {

    }
    virtual ~CWTNetControl()
    {
        if (m_hModuleDll.isLoaded())
        {
            m_hModuleDll.unload();
        }
    }

protected:
    WTNet *m_net;
    QLibrary m_hModuleDll;
    typedef WTNet* (*pfunCreateNet)();
    pfunCreateNet m_pCreate;
};

#endif // WTNETCONTROL_H
