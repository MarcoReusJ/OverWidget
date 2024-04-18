#include "custom_time_control.h"

CustomTimeControl::CustomTimeControl(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	this->setWindowFlags(Qt::FramelessWindowHint); // 设置无边框
	this->setAutoFillBackground(true);				// 自动填充背景
	//this->setAttribute(Qt::WA_ShowModal, true);		// 设置为模态

	this->Init();
}

CustomTimeControl::~CustomTimeControl()
{
}

void CustomTimeControl::setCurrentTime()
{
	ui.timeEdit->setTime(QTime::currentTime());
}

void CustomTimeControl::on_surePushButton_clicked()
{
	QString qstrDate = ui.calendarWidget->selectedDate().toString("yyyy-MM-dd");
	QString qstrClock = ui.timeEdit->time().toString("HH:mm:ss");
	QString qstrTime = QString("%1 %2").arg(qstrDate).arg(qstrClock);

	emit sendTime(qstrTime);

	// yyyy-MM-dd hh:mm:ss
//	SYSTEMTIME st;

//	st.wYear = atoi(qstrTime.mid(0, 4).toStdString().c_str());
//	st.wMonth = atoi(qstrTime.mid(5, 2).toStdString().c_str());
//	st.wDay = atoi(qstrTime.mid(8, 2).toStdString().c_str());
//	st.wHour = atoi(qstrTime.mid(11, 2).toStdString().c_str());
//	st.wMinute = atoi(qstrTime.mid(14, 2).toStdString().c_str());
//	st.wSecond = atoi(qstrTime.mid(17, 2).toStdString().c_str());
//	st.wMilliseconds = 0;

//	emit starttime_sended(st);
	
	this->close();
}

void CustomTimeControl::on_cancelPushButton_clicked()
{
	this->close();
}

void CustomTimeControl::Init()
{
	ui.surePushButton->setStyleSheet(
		"QPushButton:hover {border: 1px solid rgb(89, 153, 48);background-color: rgb(240, 240, 240);}");
	ui.cancelPushButton->setStyleSheet(
		"QPushButton:hover {border: 1px solid rgb(89, 153, 48);background-color: rgb(240, 240, 240);}");
}
