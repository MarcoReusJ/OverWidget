/*
	用于显示定制的时间控件
*/
#ifndef CUSTOM_TIME_CONTROL_H
#define CUSTOM_TIME_CONTROL_H
#include <QWidget>
#include <QPalette>
#include <QColor>

#include "ui_customtimecontrol.h"

#ifdef WIN32
#include <windows.h>
#else
//typedef struct _SYSTEMTIME {
//    long wYear;
//    long wMonth;
//    long wDayOfWeek;
//    long wDay;
//    long wHour;
//    long wMinute;
//    long wSecond;
//    long wMilliseconds;
//} SYSTEMTIME;
#endif


class CustomTimeControl : public QWidget
{
	Q_OBJECT

public:
	CustomTimeControl(QWidget *parent = 0);
	~CustomTimeControl();

public:
	void setCurrentTime();

public slots: 
	void on_surePushButton_clicked();
	void on_cancelPushButton_clicked();

Q_SIGNALS:
	void sendTime(const QString& qstrTime);
//	void starttime_sended(SYSTEMTIME);

private:
	void Init();

private:
	Ui::CustomTimeControl ui;
};

#endif // CUSTOM_TIME_CONTROL_H
