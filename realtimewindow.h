#ifndef REALTIMEWINDOW_H
#define REALTIMEWINDOW_H

#include <common_utils.h>

class RealTimeWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RealTimeWindow(QWidget *parent = 0);
    void updateRealMsg(STR_MSG& msg);

signals:
    void UpdateUISig(STR_MSG& msg);
public slots:
    void slot_updateReal(STR_MSG &msg);
private:
    QGroupBox* m_realtopwid;
    PicLabel* m_cusPre;
    PicLabel* m_cusAfr;
    PicLabel* m_cusCe;
    PicLabel* m_cusAll;
    QHBoxLayout* m_toplaypic;
    QWidget *m_realmsgwid;
    QLabel *m_wtmsglab;
    QHBoxLayout *m_toplaymsg;
    QVBoxLayout *m_realtoplay;
    QGroupBox *m_realbottomwid;
    QHBoxLayout *m_realbottomlay;
    QTableView *m_view;
    QStandardItemModel *m_mode;
    QVBoxLayout *m_mainreallay;
};

#endif // REALTIMEWINDOW_H
