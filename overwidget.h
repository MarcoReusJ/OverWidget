#ifndef OVERWIDGET_H
#define OVERWIDGET_H

#include "common_utils.h"
#include "carresult.h"
#include "picpoll.h"
#include "wtdbcontrol.h"
#include "realtimewindow.h"
#include "hiswindow.h"

namespace Ui {
class OverWidget;
}


class OverWidget;


//class CustomModel : public QSqlTableModel
//{
//    Q_OBJECT
//public:
//    explicit CustomModel(QObject *parent = nullptr,QSqlDatabase db = QSqlDatabase());

//protected:
//    virtual QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

//};

class OverWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverWidget(QWidget *parent = 0);
    ~OverWidget();
    virtual void keyPressEvent(QKeyEvent* event);
    //
    bool    mMoveing;
    QPoint  mMovePosition;
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    //
    void OutputLog2(const char *msg, ...);
    int StartLED();
    void MSesGCallback(int Lane, char *filename, int pictype, char *carno, char *pcolor, char *cartype);
    int AnalysisCarResult(const char * ResponeValue);
    int AnalysisProfileData(int l, int w, int h, int lane, long etc, int etc2);
    void InitOver();
    void UpdateUI(STR_MSG msg);
    int IsBlack(QString strcar);

    WorkThread *m_Thread;

    CWTDevControl m_DevCon;
    CWTNetControl m_NetCon[8];   //联网平台
    CWTProControl m_ProCon;
    CWTCamControl m_CamCon;
    CWTLEDControl* m_LedCon;
    CWTCarControl m_CarCon;
    CWTDBControl* m_DbCon;
    PicPoll* m_picPool;
    CarResult m_CarResult;
    void optimize(STR_MSG msg);//优化数据
    int analyzeData(STR_MSG msg);//分析数据



    std::map<std::string, int> m_DelFialedVec;
    std::queue<STR_MSG> m_MsgVedioQueueHis;
    std::queue<STR_MSG> m_MsgVedioQueue;
    std::map<int, STR_MSG*> m_maplaneAndmsg;
    CCriSec         m_QueueSect;      //消息内存锁
    CCriSec         m_QueueSectVedio;      //消息内存锁
    CCriSec         m_QueueSectVedioHis;      //消息内存锁
    QTimer *timer_pointer;
    bool bStart;

Q_SIGNALS:
    void FlushSig(QString str);
    void UpdateUISig(STR_MSG msg);
    void LedUpdate(int itype, STR_MSG msg);

public slots:
    void slot_timeout_timer_pointer();
    void slot_timeout_updateLedSlot(int iType, STR_MSG msg);

    void slot_showRealTitle();
    void slot_showHisTitle();
    void onClose();
public:
    QProcess *m_proc;
    bool m_bNeedFlush;
    int m_TimeFlush;
    long m_LastTic;
    std::string strConfigfile;
    int m_GBXZ;
    float m_GBXZW;
    float m_GBXZL;
    float m_GBXZH;

    int m_Timeout;
    int m_OkNoVideo;
    int m_OkNoShow;
    int m_UnFullExit;
    int m_SmallNoUp;
    int m_PassengerNoUp;
    int m_weidu;
    int m_WkZeroNoUp;
    int m_TimeHeart;
    int m_LaneNum;
    int m_VideoSave;
    char m_szAddr[256];
    bool m_showError;
    int m_twoce;

#ifdef WIN32
    LONG m_PrePort;
    LONG m_AfrPort;
#else
    int m_PrePort;
    int m_AfrPort;
#endif
    QTimer *m_Timer;
    //record
    QString m_strPre;
    QTimer *m_preTimer;
    void onStop(bool bFull = false);

    QShortcut * shortcut_exitfullscreen_esc;
private:
    Ui::OverWidget *ui;

public:
    //fullscreen
    QWidget *m_showwid;
    QHBoxLayout *m_piclay;
    PicLabel *m_showpiclab;
    //recordshow
    QWidget *m_recordshowwid;

    //title
    QWidget *m_titlewid;
    CustomPushButton *m_toplogo;
    CustomPushButton *m_labtitle;
    CustomPushButton *m_closebut;
    CustomPushButton *m_minbut;
    QHBoxLayout* m_toplayout;

    CustomPushButton *m_backbut;
    QHBoxLayout *m_dhtoplay;
    CustomPushButton *m_cReal;
    CustomPushButton *m_cHis;
    //实时
    RealTimeWindow* m_realwid;

    //历史
    HisWindow* m_hiswid;

    //总
    QHBoxLayout *m_bottomlay;
    QVBoxLayout *m_mainlayout;

public:
    void initTitle();
};

#endif // OVERWIDGET_H
