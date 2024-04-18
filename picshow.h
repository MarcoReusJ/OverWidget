#ifndef PICSHOW_H
#define PICSHOW_H
#include <QString>
#include <QLabel>
#include <QPainter>
#include <QEvent>
#include <QPaintEvent>
#include <QMouseEvent>

class PicLabel : public QLabel
{
    Q_OBJECT

public:
    PicLabel();
    void showpic(QString bg_picture_,int type = 0);

signals:
    void clicked();
    void dbclicked();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *e);

public:
    QPixmap bgImage;
    QImage bgImage2;
    QString m_bk;
};

#endif // PICSHOW_H
