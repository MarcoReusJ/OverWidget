#ifndef HISWINDOW_H
#define HISWINDOW_H
#include "common_utils.h"
class HisWindow;
class CustomModel : public QSqlTableModel
{
    Q_OBJECT
public:
    explicit CustomModel(QObject *parent = nullptr,QSqlDatabase db = QSqlDatabase());

protected:
    virtual QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

};
class WorkThread : public QThread
{
    Q_OBJECT

public:
    WorkThread(HisWindow *gui);
    ~WorkThread();
    void run();
    void setname(QString str);

signals:
    void onShowUI();

public:
    HisWindow *m_gui;  //主线程对象指针
    QString m_strname;
};
class HisWindow : public QWidget
{
    Q_OBJECT
public:
    explicit HisWindow(QWidget *parent = 0,QSqlDatabase db = QSqlDatabase());
    void updateRecord(ResultRecord &recod);
signals:

public slots:

    void slot_query();
    void slot_exportto();
    void slot_showSTimeLabel();
    void slot_showETimeLabel();
    void slot_recvTime(const QString &text);
    void slot_toDetails(const QModelIndex &index);
    void slot_thread_UpdateUi();
    void slot_timeout_prepointer();
    void slot_timeout_pointer();
    void slot_onPlayRecord();
public:
    bool m_showError;
    LONG m_PrePort;
    LONG m_AfrPort;
    PicLabel *m_showpiclab;
    //recordshow
    QWidget *m_recordshowwid;
    WorkThread *m_Thread;
    //record
    QString m_strPre;
    QTimer *m_Timer;
    QTimer *m_preTimer;
    void onStop(bool bFull = false);

    QSqlDatabase m_db;
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
};

#endif // HISWINDOW_H
