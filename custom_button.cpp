#include "custom_button.h"

CustomPushButton::CustomPushButton(QWidget* parent) : QLabel(parent)
{
	this->setWindowFlags(Qt::FramelessWindowHint);
	this->setAutoFillBackground(true);
}

void CustomPushButton::setButtonBackground(const QString& filename)
{
	bg_picture_ = filename;

	this->setFixedSize(QPixmap(bg_picture_).size());

	update();
}

void CustomPushButton::setButtonBackground2(const QString& filename, int w, int h)
{
	bg_picture_ = filename;

    this->setFixedSize(w,h);

	update();
}

void CustomPushButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
	QPainter painter(this);
	QString pixmapPath;

	switch (status_)
	{
	case NORMAL:
		pixmapPath = bg_picture_;
		break;
	case HOVER:
		pixmapPath = bg_picture_ + "_pressed";
		break;
	case PRESSED:
		pixmapPath = bg_picture_ + "_pressed";
		break;
	default:
		pixmapPath = bg_picture_;
		break;
	}
	painter.drawPixmap(this->rect(), QPixmap(pixmapPath));
}

void CustomPushButton::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		is_pressed_ = true;
		//status_ = PRESSED;
		//update();
	}
}

void CustomPushButton::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && is_pressed_)
	{
		is_pressed_ = false;
		//update();
		emit this->clicked();
	}
}

//void CustomPushButton::enterEvent(QEvent* event)
//{
//	is_pressed_ = false;
//	status_ = HOVER;
//	update();
//}
//
//void CustomPushButton::leaveEvent(QEvent* event)
//{
//	is_pressed_ = false;
//	status_ = NORMAL;
//	update();
//}
