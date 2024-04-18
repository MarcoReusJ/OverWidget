#ifndef WTCAMCONTROL_H
#define WTCAMCONTROL_H
#include <QLibrary>
#include "camsdk.h"
#include <Windows.h>
#include <QMessageBox>

class CWTCamControl
{
public:
    int Work()
    {
        if (m_pCreate)
            return m_cam->StartWork();
        return 1;
    }

    void SetCallback(CAMCALCALL pcb)
    {
        if (m_pCreate)
            m_cam->Car_SetCamCallBackFuc(pcb);
    }

    int DownFile(char *cartime, int lane, int dataid, char *carno, char *pUsr, int et1, int et2)
    {
        if (m_pCreate)
            return m_cam->DownFile(cartime,lane,dataid,carno,pUsr,et1,et2);
        return -1;
    }

    void Load(int type)
    {
        QString strPath;
        if(type == 1)
            strPath = "CamSDKHK";
        else if(type == 2)
            strPath = "CamSDKDH";

        m_hModuleDll.setFileName(strPath);
        if (m_hModuleDll.load())
        {
            m_pCreate = (pfunCreateNet)m_hModuleDll.resolve("CreateSDK");
            if (m_pCreate)
                m_cam = m_pCreate();
        }
        else
        {
            QMessageBox::warning(nullptr, strPath, QStringLiteral("未安装WTSDK模块，请联系管理员！"));
            m_pCreate = NULL;
            m_cam = NULL;
        }
    }

    CWTCamControl()
    {

    }
    virtual ~CWTCamControl()
    {
        if (m_hModuleDll.isLoaded())
        {
            m_hModuleDll.unload();
        }
    }

protected:
    camsdk *m_cam;
    QLibrary m_hModuleDll;
    typedef camsdk* (*pfunCreateNet)();
    pfunCreateNet m_pCreate;
};

#endif // WTDEVCONTROL_H
