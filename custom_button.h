#ifndef CUSTOM_BUTTON_H
#define CUSTOM_BUTTON_H

#include <QString>
#include <QLabel>
#include <QPainter>
#include <QEvent>
#include <QPaintEvent>
#include <QMouseEvent>

class CustomPushButton : public QLabel
{
	Q_OBJECT

public:
	enum BouttonStatus{ NORMAL, PRESSED, HOVER }; // 按钮的状态 正常 按下 徘徊

public:
	CustomPushButton(const CustomPushButton&) = delete;
	CustomPushButton& operator=(const CustomPushButton&) = delete;

public:
	explicit CustomPushButton(QWidget* parent = nullptr);
	~CustomPushButton(){}

	void setButtonBackground(const QString& filename);

	void setButtonBackground2(const QString& filename,int w,int h);

signals:
	void clicked();

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	//void enterEvent(QEvent* event);
	//void leaveEvent(QEvent* event);

public:
	BouttonStatus status_ = NORMAL; // 默认为正常状态
	bool is_pressed_ = false; // 默认为没有按下
	QString bg_picture_; 
};

#endif // CUSTOM_BUTTON_H