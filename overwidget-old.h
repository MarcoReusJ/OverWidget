#ifndef OVERWIDGET_H
#define OVERWIDGET_H

#include "common_utils.h"
#include "carresult.h"
#include "picpoll.h"
#include "include/LogEx.h"
#include "resultMode.h"
#include "custom_time_control.h"
#include "custom_button.h"
#include "picshow.h"
#include "include/PlayM4.h"
#include "include/pthread/pthread.h"
#include "CriticalSection.h"
#include "wtnetcontrol.h"
#include "wtdevcontrol.h"
#include "wtprocontrol.h"
#include "wtcamcontrol.h"
#include "wtledcontrol.h"
#include "wtcarcontrol.h"
#include "wtdbcontrol.h"

namespace Ui {
class OverWidget;
}

//enum PicType{ SMALL, FRONT, AFTER, SIDE,SIDE2,AFRSIDE, ALL,FRONT1};

class OverWidget;
class WorkThread : public QThread
{
    Q_OBJECT

public:
    WorkThread(OverWidget *gui);
    ~WorkThread();
    void run();
    void setname(QString str);

signals:
    void onShowUI();

public:
    OverWidget *m_gui;  //主线程对象指针
    QString m_strname;
};

class CustomModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit CustomModel(QObject *parent = nullptr,QSqlDatabase db = QSqlDatabase());

protected:
    virtual QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

};

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
//    void ResetLed();
//    void UpdateLed(STR_MSG *msgdata);
    int AnalysisCarResult(const char * ResponeValue);
    int AnalysisProfileData(int l, int w, int h, int lane, long etc, int etc2);
    void InitOver();
    void UpdateUI(STR_MSG *msg);
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
    void optimize(STR_MSG* msg);//优化数据
    int analyzeData(STR_MSG* msg);//分析数据



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
    void UpdateUISig(STR_MSG *msg);
    void LedUpdate(int itype, STR_MSG* msg);

public slots:
    void slot_timeout_timer_pointer();
    void slot_timeout_pre_pointer();
    void slot_timeout_pointer();
    void slot_timeout_updateLedSlot(int iType, STR_MSG* msg);
    void UpdateReal(STR_MSG *msg);
    void slot_buttonClied_showSTimeLabel();
    void slot_buttonClied_showETimeLabel();
    void slot_recvTime(const QString& text);
    void slot_buttonClied_query();
    void slot_buttonClied_exportto();
    void slot_showRealTitle();
    void slot_showHisTitle();
    void slot_doubleClicked_toDetails(const QModelIndex& index);
    void slot_labelClicked_onPlayRecord();
    void slot_thread_UpdateUi();
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

    PicLabel *m_recordcusPre;
    PicLabel *m_recordcusPre1;
    PicLabel *m_recordcusHCe;
    PicLabel *m_recordcusAfr;
    PicLabel *m_recordcusSmall;
    PicLabel *m_recordcusCe;
    PicLabel *m_recordcusVideo;
    QGridLayout* m_recordtoplayout;

    QLabel *m_recordcurtimelab;
    QLabel *m_recordlanelab;
    QLabel *m_recordcphmlab;
    QLabel *m_recordcolorlab;
    QLabel *m_recordaxislab;
    QLabel *m_recordxzwtlab;
    QLabel *m_recorduploadlab;
    QLabel *m_recordspeedlab;
    QLabel *m_recordcartypelab;
    QLabel *m_recordweightlab;
    QLabel *m_recorddirectionlab;
    QLabel *m_recordcrossroadlab;
    QGridLayout *m_recordgridlay;

    QLabel *m_recordweight1lab;
    QLabel *m_recordweight2lab;
    QLabel *m_recordweight3lab;
    QLabel *m_recordweight4lab;
    QLabel *m_recordweight5lab;
    QLabel *m_recordweight6lab;
    QLabel *m_recordweight7lab;
    QLabel *m_recordweight8lab;
    QLabel *m_recordspace1lab;
    QLabel *m_recordspace2lab;
    QLabel *m_recordspace3lab;
    QLabel *m_recordspace4lab;
    QLabel *m_recordspace5lab;
    QLabel *m_recordspace6lab;
    QLabel *m_recordspace7lab;
    QLabel *m_recordwlab;
    QLabel *m_recordhlab;
    QLabel *m_recordllab;
    QVBoxLayout *m_recordmainlay;
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
    QWidget *m_realwid;
    QGroupBox *m_realtopwid;
    QHBoxLayout *m_toplaypic;
    PicLabel *m_cusPre;
    PicLabel *m_cusAfr;
    PicLabel *m_cusCe;
    PicLabel *m_cusAll;
    QWidget *m_realmsgwid;
    QLabel *m_wtmsglab;
    QHBoxLayout *m_toplaymsg;
    QVBoxLayout *m_realtoplay;
    QGroupBox *m_realbottomwid;
    QHBoxLayout *m_realbottomlay;
    QTableView *m_view;
    QStandardItemModel *m_mode;
    QVBoxLayout *m_mainreallay;

    //历史
    QWidget *m_hiswid;
    QGroupBox *m_hisleftwid;
    QGroupBox *m_hisrightwid;

    QVBoxLayout *m_hisleftlay;
    QLabel *m_timelab;
    CustomTimeControl *m_starttime;
    QLineEdit *m_startedit;
    CustomTimeControl *m_endttime;
    QLineEdit *m_endedit;
    CustomPushButton *m_stime;
    CustomPushButton *m_etime;
    CustomPushButton *m_querybut;
    CustomPushButton *m_exportbut;
    QHBoxLayout* m_dhlay1;

    QCheckBox *m_carno;
    QCheckBox *m_checkaxis;
    QCheckBox *m_checklane;
    QLineEdit *m_caredit;
    QComboBox *m_checkaxisbox;
    QComboBox *m_checklanebox;
    QCheckBox *m_checkret;
    QComboBox *m_checkretbox;
    QHBoxLayout* m_dhlay2;
    QVBoxLayout *m_toplayhis;

    QTableView *m_viewhis;
    CustomModel *m_modehis;
    QHBoxLayout *m_mainlayhis;

    //总
    QHBoxLayout *m_bottomlay;
    QVBoxLayout *m_mainlayout;

public:
    void initTitle();
    void initReal();
    void initHis();
    void updateRecord(ResultRecord &recod);
    void loadModel();
};

#endif // OVERWIDGET_H
