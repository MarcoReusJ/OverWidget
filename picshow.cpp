#include "picshow.h"
#include <QImageReader>

PicLabel::PicLabel()
{
    this->setAutoFillBackground(true);
    this->setStyleSheet("background-color:black;background-image:url(:/pnd/png/39.png);background-position:center;background-repeat:no-repeat;");
}

void PicLabel::showpic(QString bg_picture_, int type)
{
    m_bk = bg_picture_;
    if(type == 1)  //small
    {
        bgImage2.load(bg_picture_);
        QPixmap pixmap(this->size());
        pixmap.fill(Qt::black); // 一定要填充白色，否则会有无效图形
        QPainter painter(&pixmap);
        int x = (this->size().width() - bgImage2.width()) / 2;
        int y = (this->size().height() - bgImage2.height()) / 2;
        painter.drawImage(x,y,bgImage2);
        this->setPixmap(pixmap);
    }
    else if(type == 2)
    {
        bgImage2.load(":/pnd/png/play.png");
        QImageReader Reader;
        Reader.setFileName(bg_picture_);
        Reader.setAutoTransform(true);
        auto targetsize = Reader.size().scaled(this->size(), Qt::IgnoreAspectRatio);
        Reader.setScaledSize(targetsize);
        QPixmap pixmap = QPixmap::fromImageReader(&Reader);
        QPainter painter(&pixmap);
        int x = (this->size().width() - bgImage2.width()) / 2;
        int y = (this->size().height() - bgImage2.height()) / 2;
        painter.drawImage(x,y,bgImage2);
        this->setPixmap(pixmap);
    }
    else
    {
//        bgImage.load(bg_picture_);
//        this->setStyleSheet("background-color:black");
//        this->setPixmap(bgImage.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QImageReader Reader;
        Reader.setFileName(bg_picture_);
        Reader.setAutoTransform(true);
        auto targetsize = Reader.size().scaled(this->size(), Qt::IgnoreAspectRatio);
        Reader.setScaledSize(targetsize);
        this->setPixmap(QPixmap::fromImageReader(&Reader));
    }
}


void PicLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit this->clicked();
    }
}

void PicLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    emit this->dbclicked();
}

