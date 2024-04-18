#include "realtimewindow.h"

RealTimeWindow::RealTimeWindow(QWidget *parent) : QWidget(parent)
{
    //背景
    QPalette palette2;
    palette2.setColor(QPalette::Background, Qt::white);
    this->setAutoFillBackground(true);
    this->setPalette(palette2);
    //top
    m_realtopwid = new QGroupBox;
    m_realtopwid->setStyleSheet("QGroupBox{background-image:url(:/png/image/realtop.png);border: none;}QGroupBox*{background-image:url();}");
    //1
    m_cusPre = new PicLabel;
    m_cusAfr = new PicLabel;
    m_cusCe = new PicLabel;
    m_cusAll = new PicLabel;
    m_cusAll->setFixedHeight(300);
    m_cusCe->setFixedHeight(300);
    m_cusAfr->setFixedHeight(300);
    m_cusPre->setFixedHeight(300);
    m_toplaypic = new QHBoxLayout;
    m_toplaypic->addWidget(m_cusPre);
    m_toplaypic->addWidget(m_cusAfr);
    m_toplaypic->addWidget(m_cusAll);
    m_toplaypic->addWidget(m_cusCe);
    m_toplaypic->setSpacing(10);
    m_toplaypic->setMargin(10);
    //2
    //背景图
    QPalette palette;
    QPixmap pixmap(":/png/image/realmsp.png");
    palette.setBrush(QPalette::Window, QBrush(pixmap));

    m_wtmsglab = new QLabel;
    m_wtmsglab->setStyleSheet(QString("color:#ffffff; border: none; border-radius: 0px;font-family:\"Microsoft YaHei\";font-size:%1px;").arg(20));
    m_toplaymsg = new QHBoxLayout;
    m_toplaymsg->addStretch();
    m_toplaymsg->addWidget(m_wtmsglab);
    m_toplaymsg->addStretch();
    m_realmsgwid = new QWidget;
    m_realmsgwid->setAutoFillBackground(true);
//    m_realmsgwid->setFixedSize(1566,81);
    m_realmsgwid->setFixedSize(QApplication::desktop()->width()*0.8, 81);
    m_realmsgwid->setPalette(palette);
    m_realmsgwid->setLayout(m_toplaymsg);
    m_realtoplay = new QVBoxLayout;
    m_realtoplay->addLayout(m_toplaypic);
    m_realtoplay->addWidget(m_realmsgwid,0,Qt::AlignCenter);
    m_realtoplay->setSpacing(10);
    m_realtoplay->setMargin(10);
    m_realtopwid->setLayout(m_realtoplay);

    //bottom
    m_realbottomwid = new QGroupBox;
    m_realbottomwid->setStyleSheet("QGroupBox{background-image:url(:/png/image/realbottom.png);border: none;}QGroupBox*{background-image:url();}");
//    m_realbottomwid->setFixedSize(1894,436);

    QFont font("Microsoft YaHei", 13); // 微软雅黑
    QFont font2("Microsoft YaHei", 11); // 微软雅黑
    m_view = new QTableView;
    m_view->verticalHeader()->hide();							// 隐藏垂直表头
    m_view->horizontalHeader()->setFont(font);
//    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);	//根据内容自动调整大小
    m_view->horizontalHeader()->setStretchLastSection(true);		// 伸缩最后一列
    m_view->horizontalHeader()->setHighlightSections(false);		// 点击表时不对表头行光亮
    m_view->setShowGrid(false);										// 隐藏网格线
    m_view->setAlternatingRowColors(true);							// 开启隔行异色
//    m_view->setFocusPolicy(Qt::NoFocus);							// 去除当前Cell周边虚线框
//    m_view->horizontalHeader()->setMinimumSectionSize(120);
    m_view->setFont(font2);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);			//为整行选中
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);			//不可多选
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);				//只读属性，即不能编辑
//    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);		//滑动像素
//    m_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);		//滑动像素
//    m_view->setFrameShape(QFrame::NoFrame);

    m_mode = new QStandardItemModel(this);
    m_view->setModel(m_mode);
    m_mode->setColumnCount(19);
    m_mode->setRowCount(12);
    m_mode->setHeaderData(0, Qt::Horizontal, QStringLiteral("序号"));
    m_mode->setHeaderData(1, Qt::Horizontal, QStringLiteral("检测时间"));
    m_mode->setHeaderData(2, Qt::Horizontal, QStringLiteral("车牌号码"));
    m_mode->setHeaderData(3, Qt::Horizontal, QStringLiteral("车牌颜色"));
    m_mode->setHeaderData(4, Qt::Horizontal, QStringLiteral("车道"));
    m_mode->setHeaderData(5, Qt::Horizontal, QStringLiteral("车速"));
    m_mode->setHeaderData(6, Qt::Horizontal, QStringLiteral("车长"));
    m_mode->setHeaderData(7, Qt::Horizontal, QStringLiteral("车宽"));
    m_mode->setHeaderData(8, Qt::Horizontal, QStringLiteral("车高"));
    m_mode->setHeaderData(9, Qt::Horizontal, QStringLiteral("轴数"));
    m_mode->setHeaderData(10, Qt::Horizontal, QStringLiteral("总重量"));
    m_mode->setHeaderData(11, Qt::Horizontal, QStringLiteral("轴1重量"));
    m_mode->setHeaderData(12, Qt::Horizontal, QStringLiteral("轴2重量"));
    m_mode->setHeaderData(13, Qt::Horizontal, QStringLiteral("轴3重量"));
    m_mode->setHeaderData(14, Qt::Horizontal, QStringLiteral("轴4重量"));
    m_mode->setHeaderData(15, Qt::Horizontal, QStringLiteral("轴5重量"));
    m_mode->setHeaderData(16, Qt::Horizontal, QStringLiteral("轴6重量"));
    m_mode->setHeaderData(17, Qt::Horizontal, QStringLiteral("轴7重量"));
    m_mode->setHeaderData(18, Qt::Horizontal, QStringLiteral("轴8重量"));
    m_view->setColumnWidth(1,200);

    m_realbottomlay = new QHBoxLayout;
    m_realbottomlay->addWidget(m_view);
    m_realbottomlay->setMargin(15);
    m_realbottomwid->setLayout(m_realbottomlay);

    m_mainreallay = new QVBoxLayout;
    m_mainreallay->addWidget(m_realtopwid);
    m_mainreallay->addWidget(m_realbottomwid);
    m_mainreallay->setSpacing(5);
    m_mainreallay->setMargin(5);
    this->setLayout(m_mainreallay);
//    qRegisterMetaType<RealTimeWindow>("RealTimeWindow");
    connect(this,&RealTimeWindow::UpdateUISig,this,&RealTimeWindow::slot_updateReal, Qt::QueuedConnection);
}

void RealTimeWindow::updateRealMsg(STR_MSG& msg)
{
//    emit UpdateUISig(msg);
    slot_updateReal(msg);
}
void RealTimeWindow::slot_updateReal(STR_MSG& msg)
{
    int iTC = ::GetTickCount();
    static int rownum = 0;
    if(rownum > 11)  //0~11 共12行
        rownum = 0;
    //插入新记录
    int co=0;
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.dataId),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString(msg.strtime),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::fromLocal8Bit(msg.strcarno),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::fromLocal8Bit(msg.strplatecolor),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.laneno),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.speed),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.l),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.w),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.h),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.axiscount),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.weight),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[0]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[1]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[2]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[3]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[4]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[5]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[6]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg.Axis_weight[7]),Qt::DisplayRole);
    for(int i=0;i<co;i++)
        m_mode->item(rownum,i)->setTextAlignment(Qt::AlignCenter);

    QModelIndex mdidx = m_mode->index(rownum, 1); //获得需要编辑的单元格的位置
    m_view->setCurrentIndex(mdidx);	//设定需要编辑的单元格
    m_view->update();
    rownum++;

    char  szVal[1024] = {};
    sprintf(szVal,"检测时间:%s\t车牌号码:%10s\t轴数:%d\t车道:%d\t车速%4dkm/s\t总重量:%6dkg",msg.strtime,msg.strcarno,msg.axiscount,msg.laneno,msg.speed,msg.weight);
    m_wtmsglab->setText(QString::fromLocal8Bit(szVal));
    m_cusPre->showpic(QString::fromLocal8Bit(msg.carpics[FRONT]));
    m_cusAfr->showpic(QString::fromLocal8Bit(msg.carpics[AFTER]));
    m_cusCe->showpic(QString::fromLocal8Bit(msg.carpics[SMALL]));
    m_cusAll->showpic(QString::fromLocal8Bit(msg.carpics[SIDE]));
    int iTC2 = ::GetTickCount();
    OutputLog("update real ui use time = %d",iTC2-iTC);
}
