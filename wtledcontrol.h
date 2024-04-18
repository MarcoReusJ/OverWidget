#ifndef WTLEDCONTROL_H
#define WTLEDCONTROL_H
#include <QLibrary>
#include "wtled.h"
#include <Windows.h>
#include <QMessageBox>
#include <QJsonObject>
#include <QTimer>
#include <QObject>

class CWTLEDControl : public QObject
{
    //Q_OBJECT

public:
    void Work()
    {
        if (m_pCreate)
            m_led->Work();
    }

    void UpdateLed(char *CarNo, char *ret)
    {
        if (m_pCreate)
            m_led->UpdateLed(CarNo, ret);
    }

    void ResetLed()
    {
        if (m_pCreate)
            m_led->ResetLed();
    }

    void Close()
    {
        if (m_pCreate)
            m_led->Close();
    }
    void UpdateText(QJsonObject jsonObject){m_text= jsonObject;}

    void UpdateLedEx()
    {
        static int countPage=0;//是否需要翻页,翻页计时
        QString page = QString("page%1").arg(countPage+1);
        QJsonArray fields = m_text[page].toArray();
        m_led->UpdateLedEx(fields);
        if(countPage++ > m_page)
            countPage=0;
    }

    void Load(int type)
    {
        QString strPath;
        if(type == 3)
            strPath = "WTLed3";
        else if(type == 1)
            strPath = "WTLed1";
        else if(type == 2)
            strPath = "WTLedEQ";
        else if(type == 4)
            strPath = "WTLedYB";
        else if(type == 5)
            strPath = "WTLed4";

        m_hModuleDll.setFileName(strPath);
        if (m_hModuleDll.load())
        {
            m_pCreate = (pfunCreateLed)m_hModuleDll.resolve("CreateLed");
            if (m_pCreate)
                m_led = m_pCreate();
        }
        else
        {
            QMessageBox::warning(nullptr, strPath, QStringLiteral("未安装WTLed模块，请联系管理员！"));
            m_pCreate = NULL;
            m_led = NULL;
        }
    }

    QJsonObject getTextJson(){return m_text;}

    CWTLEDControl(int type,int page,int time,QString jsonString):
        m_type(type),m_page(page),m_time(time)
    {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8());
        if (jsonDoc.isNull()) { return;}
        m_text = jsonDoc.object();

        m_ledPageTimer = new QTimer;
        Load(m_type);
        m_ledPageTimer->start(m_time);

        //todo
        //connect(m_ledPageTimer, &QTimer::timeout, this, &CWTLEDControl::slot_updateLedEX);
    }
    virtual ~CWTLEDControl()
    {
        if (m_hModuleDll.isLoaded())
        {
            m_hModuleDll.unload();
        }
    }

protected:
    wtled *m_led;
    int m_type;
    int m_page;
    int m_time;
    QJsonObject m_text;
    QTimer* m_ledPageTimer;
    QLibrary m_hModuleDll;
    typedef wtled* (*pfunCreateLed)();
    pfunCreateLed m_pCreate;

public slots:
    void slot_updateLedEX()
    {
        UpdateLedEx();
    }

};

#endif // WTDEVCONTROL_H
