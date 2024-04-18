#include "hiswindow.h"

HisWindow::HisWindow(QWidget *parent, QSqlDatabase db) : QWidget(parent)
{
    m_db = db;
    m_Thread = new WorkThread(this);
    //背景
    QPalette palette2;
    palette2.setColor(QPalette::Background, Qt::white);
    this->setAutoFillBackground(true);
    this->setPalette(palette2);
    m_hisleftwid = new QGroupBox;
    m_hisleftwid->setStyleSheet("QGroupBox{background-image:url(:/png/image/hisleft.png);border: none;}QGroupBox*{background-image:url();}");
//    m_hisleftwid->setFixedSize(1264,867);

    //设置图标底色和背景色一样
    QPalette myPalette;
    QColor myColor(0, 0, 0);
    myColor.setAlphaF(0);
    myPalette.setBrush(backgroundRole(), myColor);
    //1
    QFont font2("Microsoft YaHei", 13); // 微软雅黑
    QFont font("Microsoft YaHei", 11); // 微软雅黑
    m_timelab = new QLabel;
    m_timelab->setText(QStringLiteral("起止时间"));
    m_startedit = new QLineEdit;
    m_startedit->setCursorPosition(0);
    m_endedit = new QLineEdit;
    m_endedit->setCursorPosition(0);
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString strTemp = current_date_time.toString("yyyy-MM-dd hh:mm:ss");
    m_startedit->setText(strTemp);
    m_endedit->setText(strTemp);
    m_stime = new CustomPushButton;
    m_stime->setButtonBackground(":/pnd/png/cj_06.png");
    m_etime = new CustomPushButton;
    m_etime->setButtonBackground(":/pnd/png/cj_06.png");
    m_stime->setPalette(myPalette);
    m_etime->setPalette(myPalette);
    m_starttime = new CustomTimeControl;
    m_starttime->hide();
    m_endttime = new CustomTimeControl;
    m_endttime->hide();
    m_querybut = new CustomPushButton;
    m_querybut->setButtonBackground(":/png/image/query.png");
    m_exportbut = new CustomPushButton;
    m_exportbut->setButtonBackground(":/png/image/export.png");
    m_querybut->setPalette(myPalette);
    m_exportbut->setPalette(myPalette);
    m_dhlay1 = new QHBoxLayout;
    m_dhlay1->addWidget(m_timelab);
    m_dhlay1->addWidget(m_startedit);
    m_dhlay1->addWidget(m_stime);
    m_dhlay1->addSpacing(30);
    m_dhlay1->addWidget(m_endedit);
    m_dhlay1->addWidget(m_etime);
    m_dhlay1->addStretch();
    m_dhlay1->addWidget(m_querybut);
    m_dhlay1->addWidget(m_exportbut);
    m_dhlay1->setSpacing(10);
    m_dhlay1->setMargin(5);

    m_carno = new QCheckBox;
    m_carno->setText(QStringLiteral("车牌号码"));
    m_caredit = new QLineEdit;
    m_checkaxis = new QCheckBox;
    m_checkaxis->setText(QStringLiteral("轴数"));
    m_checkaxisbox = new QComboBox;
    m_checkaxisbox->setFont(font);
    m_checkaxisbox->addItem(QStringLiteral("2"));
    m_checkaxisbox->addItem(QStringLiteral("3"));
    m_checkaxisbox->addItem(QStringLiteral("4"));
    m_checkaxisbox->addItem(QStringLiteral("5"));
    m_checkaxisbox->addItem(QStringLiteral("6"));
    m_checkaxisbox->addItem(QStringLiteral("7"));
    m_checkaxisbox->addItem(QStringLiteral("8"));
    m_checklane = new QCheckBox(QStringLiteral("车道"));
    m_checklanebox = new QComboBox;
    m_checklanebox->setFont(font);
    m_checklanebox->addItem(QStringLiteral("1"));
    m_checklanebox->addItem(QStringLiteral("2"));
    m_checklanebox->addItem(QStringLiteral("3"));
    m_checklanebox->addItem(QStringLiteral("4"));
    m_checklanebox->addItem(QStringLiteral("5"));
    m_checklanebox->addItem(QStringLiteral("6"));
    m_checklanebox->addItem(QStringLiteral("7"));
    m_checklanebox->addItem(QStringLiteral("8"));
    m_checkaxisbox->setFixedWidth(100);
    m_checklanebox->setFixedWidth(100);
    m_checkret = new QCheckBox;
    m_checkret->setText(QStringLiteral("判定"));
    m_checkretbox = new QComboBox;
    m_checkretbox->setFont(font);
    m_checkretbox->addItem(QStringLiteral("合格"));
    m_checkretbox->addItem(QStringLiteral("不合格"));

    m_dhlay2 = new QHBoxLayout;
    m_dhlay2->addWidget(m_carno);
    m_dhlay2->addWidget(m_caredit);
    m_dhlay2->addStretch(1);
    m_dhlay2->addWidget(m_checkaxis);
    m_dhlay2->addWidget(m_checkaxisbox);
    m_dhlay2->addStretch(1);
    m_dhlay2->addWidget(m_checklane);
    m_dhlay2->addWidget(m_checklanebox);
    m_dhlay2->addStretch(1);
    m_dhlay2->addWidget(m_checkret);
    m_dhlay2->addWidget(m_checkretbox);
    m_dhlay2->addStretch(3);
    m_dhlay2->setSpacing(10);
    m_dhlay2->setMargin(5);

    m_toplayhis = new QVBoxLayout;
    m_toplayhis->addLayout(m_dhlay1);
    m_toplayhis->addLayout(m_dhlay2);
    m_toplayhis->setSpacing(12);
    m_toplayhis->setMargin(0);
    //2
    m_viewhis = new QTableView;
    m_modehis = new CustomModel(this, m_db);
    //设置数据表
    m_modehis->setTable("Task");
    m_modehis->setHeaderData(m_modehis->fieldIndex("id"),Qt::Horizontal,QStringLiteral("序号"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("DetectionTime"),Qt::Horizontal,QStringLiteral("测量时间"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("CarNO"),Qt::Horizontal,QStringLiteral("车牌号码"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("PlateColor"),Qt::Horizontal,QStringLiteral("车牌颜色"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("CarType"),Qt::Horizontal,QStringLiteral("车型"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("Speed"),Qt::Horizontal,QStringLiteral("车速"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("laneno"),Qt::Horizontal,QStringLiteral("车道"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("Axles"),Qt::Horizontal,QStringLiteral("轴数"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("crossroad"),Qt::Horizontal,QStringLiteral("是否跨道"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("Weight"),Qt::Horizontal,QStringLiteral("总重量"));
    m_modehis->setHeaderData(m_modehis->fieldIndex("DetectionResult"),Qt::Horizontal,QStringLiteral("结果"));
    //
    m_viewhis->setModel(m_modehis);
    m_viewhis->verticalHeader()->hide();							// 隐藏垂直表头
    m_viewhis->horizontalHeader()->setFont(font2);
//    m_viewhis->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);	//根据内容自动调整大小
//    m_viewhis->horizontalHeader()->setMinimumSectionSize(100);
    m_viewhis->horizontalHeader()->setStretchLastSection(true);		// 伸缩最后一列
    m_viewhis->horizontalHeader()->setHighlightSections(false);		// 点击表时不对表头行光亮
    m_viewhis->setShowGrid(false);										// 隐藏网格线
    m_viewhis->setAlternatingRowColors(true);							// 开启隔行异色
//    m_viewhis->setFocusPolicy(Qt::NoFocus);							// 去除当前Cell周边虚线框
    m_viewhis->setFont(font);
    m_viewhis->setSelectionBehavior(QAbstractItemView::SelectRows);		//为整行选中
    m_viewhis->setSelectionMode(QAbstractItemView::SingleSelection);		//不可多选
    m_viewhis->setEditTriggers(QAbstractItemView::NoEditTriggers);			//只读属性，即不能编辑
//    m_viewhis->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);	//滑动像素
//    m_viewhis->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);	//滑动像
//    m_viewhis->setFrameShape(QFrame::NoFrame);
    //
    m_viewhis->setColumnWidth(1,200);
    for(int i=10;i<42;i++)
        m_viewhis->setColumnHidden(i,true);
    for(int i=42;i<51;i++)
        m_viewhis->setColumnHidden(i,true);
    //
    m_hisleftlay = new QVBoxLayout;
    m_hisleftlay->addLayout(m_toplayhis);
    m_hisleftlay->addWidget(m_viewhis);
    m_hisleftlay->setSpacing(10);
    m_hisleftlay->setMargin(15);
    m_hisleftwid->setLayout(m_hisleftlay);

    //
    m_hisrightwid = new QGroupBox;
    m_hisrightwid->setStyleSheet("QGroupBox{background-image:url(:/png/image/hisright.png);border: none;}QGroupBox*{background-image:url();}");
    m_hisrightwid->setFixedWidth(QApplication::desktop()->width()/3);
//    m_hisrightwid->setFixedSize(640,867);

    m_recordcusPre = new PicLabel;
    m_recordcusPre1 = new PicLabel;
    m_recordcusHCe = new PicLabel;
    m_recordcusAfr = new PicLabel;
    m_recordcusSmall = new PicLabel;
    m_recordcusCe = new PicLabel;
    m_recordcusVideo = new PicLabel;
    m_recordcusPre->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusPre1->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusHCe->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusAfr->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusCe->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusVideo->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordtoplayout = new QGridLayout;
    m_recordtoplayout->addWidget(m_recordcusPre,0,0);
    m_recordtoplayout->addWidget(m_recordcusAfr,0,1);
    m_recordtoplayout->addWidget(m_recordcusCe, 1,0);
    m_recordtoplayout->addWidget(m_recordcusVideo,1,1);
    m_recordtoplayout->addWidget(m_recordcusPre1, 2,0);
    m_recordtoplayout->addWidget(m_recordcusHCe,2,1);
    m_recordtoplayout->setSpacing(10);
    m_recordtoplayout->setMargin(10);

    m_recordlanelab = new QLabel(QStringLiteral("车道:"));
    m_recordcphmlab = new QLabel(QStringLiteral("车牌号码:"));
    m_recordaxislab = new QLabel(QStringLiteral("轴数:"));
    m_recordspeedlab = new QLabel(QStringLiteral("车辆速度:"));
    m_recordcolorlab = new QLabel(QStringLiteral("车牌颜色:"));
    m_recordcurtimelab = new QLabel(QStringLiteral("检测时间:"));
    m_recordcartypelab = new QLabel(QStringLiteral("车辆类型:"));
    m_recordweightlab = new QLabel(QStringLiteral("总重量:"));
    m_recorduploadlab = new QLabel(QStringLiteral("是否上传:"));
    m_recorddirectionlab = new QLabel(QStringLiteral("行车方向:"));
    m_recordcrossroadlab = new QLabel(QStringLiteral("是否跨道:"));
    m_recordxzwtlab = new QLabel(QStringLiteral("限重:"));
    m_recordwlab = new QLabel(QStringLiteral("宽:"));
    m_recordhlab = new QLabel(QStringLiteral("高:"));
    m_recordllab = new QLabel(QStringLiteral("长:"));
    m_recordweight1lab = new QLabel(QStringLiteral("轴1重:"));
    m_recordweight2lab = new QLabel(QStringLiteral("轴2重:"));
    m_recordweight3lab = new QLabel(QStringLiteral("轴3重:"));
    m_recordweight4lab = new QLabel(QStringLiteral("轴4重:"));
    m_recordweight5lab = new QLabel(QStringLiteral("轴5重:"));
    m_recordweight6lab = new QLabel(QStringLiteral("轴6重:"));
    m_recordweight7lab = new QLabel(QStringLiteral("轴7重:"));
    m_recordweight8lab = new QLabel(QStringLiteral("轴8重:"));
    m_recordspace1lab = new QLabel(QStringLiteral("轴1间距:"));
    m_recordspace2lab = new QLabel(QStringLiteral("轴2间距:"));
    m_recordspace3lab = new QLabel(QStringLiteral("轴3间距:"));
    m_recordspace4lab = new QLabel(QStringLiteral("轴4间距:"));
    m_recordspace5lab = new QLabel(QStringLiteral("轴5间距:"));
    m_recordspace6lab = new QLabel(QStringLiteral("轴6间距:"));
    m_recordspace7lab = new QLabel(QStringLiteral("轴7间距:"));
    m_recordgridlay = new QGridLayout;
    m_recordgridlay->addWidget(m_recordcurtimelab,0,0);
    m_recordgridlay->addWidget(m_recordweight1lab,0,1);
    m_recordgridlay->addWidget(m_recordcphmlab,1,0);
    m_recordgridlay->addWidget(m_recordweight2lab,1,1);
    m_recordgridlay->addWidget(m_recordcusSmall,2,0,2,1);
    m_recordgridlay->addWidget(m_recordweight3lab,2,1);
//    m_recordgridlay->addWidget(m_recordaxislab,3,0);
    m_recordgridlay->addWidget(m_recordweight4lab,3,1);
    m_recordgridlay->addWidget(m_recordxzwtlab,4,0);
    m_recordgridlay->addWidget(m_recordweight5lab,4,1);
    m_recordgridlay->addWidget(m_recordweightlab,5,0);
    m_recordgridlay->addWidget(m_recordweight6lab,5,1);
    m_recordgridlay->addWidget(m_recorddirectionlab,6,0);
    m_recordgridlay->addWidget(m_recordspace1lab,6,1);
    m_recordgridlay->addWidget(m_recordcrossroadlab,7,0);
    m_recordgridlay->addWidget(m_recordspace2lab,7,1);
    m_recordgridlay->addWidget(m_recorduploadlab,8,0);
    m_recordgridlay->addWidget(m_recordspace3lab,8,1);
    m_recordgridlay->addWidget(m_recordwlab,9,0);
    m_recordgridlay->addWidget(m_recordspace4lab,9,1);
    m_recordgridlay->addWidget(m_recordhlab,10,0);
    m_recordgridlay->addWidget(m_recordspace5lab,10,1);
    m_recordgridlay->addWidget(m_recordllab,11,0);
    m_recordgridlay->addWidget(m_recordspace6lab,11,1);
    m_recordgridlay->setContentsMargins(15,10,15,20);

    m_recordmainlay = new QVBoxLayout;
    m_recordmainlay->addLayout(m_recordtoplayout);
    m_recordmainlay->addLayout(m_recordgridlay);
    m_recordmainlay->setSpacing(10);
    m_recordmainlay->setMargin(15);
    m_hisrightwid->setLayout(m_recordmainlay);

    //all
    m_mainlayhis = new QHBoxLayout;
    m_mainlayhis->addWidget(m_hisleftwid);
    m_mainlayhis->addWidget(m_hisrightwid);
    m_mainlayhis->setSpacing(5);
    m_mainlayhis->setMargin(5);
    this->setLayout(m_mainlayhis);

    m_showpiclab = new PicLabel;
    m_showpiclab->setToolTip(QStringLiteral("按Esc返回"));

    m_Timer = new QTimer;
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(slot_timeout_pointer()));
    m_preTimer = new QTimer;
    connect(m_preTimer, SIGNAL(timeout()), this, SLOT(slot_timeout_prepointer()));

    QObject::connect(m_querybut, SIGNAL(clicked()), this, SLOT(slot_query()));
    QObject::connect(m_exportbut, SIGNAL(clicked()), this, SLOT(slot_exportto()));
    QObject::connect(m_stime, SIGNAL(clicked()), this, SLOT(slot_showSTimeLabel()));
    QObject::connect(m_etime, SIGNAL(clicked()), this, SLOT(slot_showETimeLabel()));
    QObject::connect(m_starttime, SIGNAL(sendTime(const QString&)), this, SLOT(slot_recvTime(const QString&)));
    QObject::connect(m_endttime, SIGNAL(sendTime(const QString&)), this, SLOT(slot_recvTime(const QString&)));
    QObject::connect(m_viewhis, SIGNAL(clicked(QModelIndex)), this, SLOT(slot_toDetails(QModelIndex)));
    QObject::connect(m_recordcusVideo, SIGNAL(clicked()), this, SLOT(slot_onPlayRecord()));
    QObject::connect(m_recordcusCe, SIGNAL(dbclicked()), this, SLOT(slot_onPlayRecord()));
    QObject::connect(m_recordcusVideo, SIGNAL(dbclicked()), this, SLOT(slot_onPlayRecord()));
    QObject::connect(m_recordcusPre, SIGNAL(dbclicked()), this, SLOT(slot_onPlayRecord()));
    QObject::connect(m_recordcusPre1, SIGNAL(dbclicked()), this, SLOT(slot_onPlayRecord()));
    QObject::connect(m_recordcusHCe, SIGNAL(dbclicked()), this, SLOT(slot_onPlayRecord()));
    QObject::connect(m_recordcusAfr, SIGNAL(dbclicked()), this, SLOT(slot_onPlayRecord()));
    QObject::connect(m_recordcusSmall, SIGNAL(dbclicked()), this, SLOT(slot_onPlayRecord()));

}

void HisWindow::updateRecord(ResultRecord &recod)
{
    m_strPre = recod.qstrpicvideo;
    recod.qstrpicpre1.replace(QString("/"),QString("\\"));
    m_recordcusPre->showpic(recod.qstrpicpre);
    m_recordcusPre1->showpic(recod.qstrpicpre1);
    m_recordcusHCe->showpic(recod.qstrpichce);
    m_recordcusAfr->showpic(recod.qstrpicafr);
    m_recordcusSmall->showpic(recod.qstrpicsmall,1);

    m_recordcusCe->showpic(recod.qstrpicce);
    if(recod.qstrVideoState.toInt() == 0)
        m_recordcusVideo->showpic(":/pnd/png/play.png",1);
    else if(recod.qstrVideoState.toInt() == 5 || recod.qstrVideoState.toInt() == 7)
        m_recordcusVideo->showpic(":/pnd/png/Min.png",1);
    else if(recod.qstrVideoState.toInt() == 3)
        m_recordcusVideo->showpic(":/pnd/png/X.png",1);
    else   // 1  2  6
        m_recordcusVideo->showpic(recod.qstrpicpre,2);

    if(recod.qstrUploadState.toInt() == 5 || recod.qstrUploadState.toInt() == 7)
        m_recorduploadlab->setText(m_recorduploadlab->text()+" "+QStringLiteral("不上传"));
    else if(recod.qstrUploadState.toInt() == 1)
        m_recorduploadlab->setText(m_recorduploadlab->text()+" "+QStringLiteral("已上传"));
    else if(recod.qstrUploadState.toInt() == 0 || recod.qstrUploadState.toInt() == 2)
        m_recorduploadlab->setText(m_recorduploadlab->text()+" "+QStringLiteral("待上传"));
    else if(recod.qstrUploadState.toInt() == 6)
        m_recorduploadlab->setText(m_recorduploadlab->text()+" "+QStringLiteral("上传失败"));

    m_recordxzwtlab->setText(m_recordxzwtlab->text()+" "+recod.qstrXz);
    m_recordlanelab->setText(m_recordlanelab->text()+" "+recod.qstrlane);
    m_recordcphmlab->setText(m_recordcphmlab->text()+" "+recod.qstrcarno);
    m_recordcurtimelab->setText(m_recordcurtimelab->text()+" "+recod.qstrtime);
    m_recordaxislab->setText(m_recordaxislab->text()+" "+recod.qstraxis);
    m_recordcartypelab->setText(m_recordcartypelab->text()+" "+recod.qstrcartype);
    m_recordspeedlab->setText(m_recordspeedlab->text()+" "+recod.qstrspeed);
    m_recordweightlab->setText(m_recordweightlab->text()+" "+recod.qstrweight);
    m_recordcolorlab->setText(m_recordcolorlab->text()+" "+recod.qstrcolor);
    m_recorddirectionlab->setText(m_recorddirectionlab->text()+" "+recod.qstrdirection);
    m_recordcrossroadlab->setText(m_recordcrossroadlab->text()+" "+recod.qstrcrossroad);
    m_recordwlab->setText(m_recordwlab->text()+" "+recod.qstrw);
    m_recordhlab->setText(m_recordhlab->text()+" "+recod.qstrh);
    m_recordllab->setText(m_recordllab->text()+" "+recod.qstrl);
    m_recordweight1lab->setText(m_recordweight1lab->text()+" "+recod.qstrweight1);
    m_recordweight2lab->setText(m_recordweight2lab->text()+" "+recod.qstrweight2);
    m_recordweight3lab->setText(m_recordweight3lab->text()+" "+recod.qstrweight3);
    m_recordweight4lab->setText(m_recordweight4lab->text()+" "+recod.qstrweight4);
    m_recordweight5lab->setText(m_recordweight5lab->text()+" "+recod.qstrweight5);
    m_recordweight6lab->setText(m_recordweight6lab->text()+" "+recod.qstrweight6);
    m_recordweight7lab->setText(m_recordweight7lab->text()+" "+recod.qstrweight7);
    m_recordweight8lab->setText(m_recordweight8lab->text()+" "+recod.qstrweight8);
    m_recordspace1lab->setText(m_recordspace1lab->text()+" "+recod.qstrs1);
    m_recordspace2lab->setText(m_recordspace2lab->text()+" "+recod.qstrs2);
    m_recordspace3lab->setText(m_recordspace3lab->text()+" "+recod.qstrs3);
    m_recordspace4lab->setText(m_recordspace4lab->text()+" "+recod.qstrs4);
    m_recordspace5lab->setText(m_recordspace5lab->text()+" "+recod.qstrs5);
    m_recordspace6lab->setText(m_recordspace6lab->text()+" "+recod.qstrs6);
    m_recordspace7lab->setText(m_recordspace7lab->text()+" "+recod.qstrs7);
}

void HisWindow::slot_query()
{
    QString strWhere;
    QString S = m_startedit->text().left(13);
    QString E = m_endedit->text().left(13);

    char strSql[256] = {};
    sprintf(strSql, "(DetectionTime >= '%s:00:00') and (DetectionTime <= '%s:59:59' and LENGTH(CarNO)>3) ", S.toStdString().c_str(), E.toStdString().c_str());
    strWhere = strSql;

    if (m_carno->isChecked())
    {
        sprintf(strSql, " and CarNO like '%%%s%%'", m_caredit->text().toStdString().c_str());
        strWhere += strSql;
    }
    if (m_checkaxis->isChecked())
    {
        sprintf(strSql, " and Axles = %d", m_checkaxisbox->currentText().toInt());
        strWhere += strSql;
    }
    if (m_checklane->isChecked())
    {
        sprintf(strSql, " and laneno = %d", m_checklanebox->currentText().toInt());
        strWhere += strSql;
    }
    if (m_checkret->isChecked())
    {
        sprintf(strSql, " and DetectionResult = '%s'", m_checkretbox->currentText().toStdString().c_str());
        strWhere += strSql;
    }

    if (!m_showError)
    {
        sprintf(strSql, " and bUploadpic < 7");
        strWhere += strSql;
    }

    m_modehis->setFilter(strWhere);
    m_modehis->setSort(0,Qt::DescendingOrder);
    //查询数据
    if(!m_modehis->select())
    {
        OutputLog("车辆记录查询失败");
//        return;
    }
    OutputLog("query获取行数");
    while (m_modehis->canFetchMore())
    {
        m_modehis->fetchMore();
    }
    int iAll = m_modehis->rowCount();
    if(iAll > 0)
        QMessageBox::information(NULL,QStringLiteral("总数"),QString::number(iAll));
    else
        QMessageBox::information(NULL,QStringLiteral("总数"),QStringLiteral("未查询到相关记录!"));

    OutputLog("query历史记录end---:%s,%s",strSql,strWhere.toStdString().data());
}

void HisWindow::slot_exportto()
{
    QString S = m_startedit->text().left(10) + ",";
    QString E = m_endedit->text().left(10)+ ",";

    //另存为对话框
    QString fileName = QStringLiteral("./过车检测记录表");
    fileName += QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    fileName += ".csv";
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("导出检测记录信息"), fileName, "*.csv");
    if (filePath.isEmpty())
    {
        return;
    }
    else
    {
        if (filePath.mid(filePath.size() - 4, 4) != QString(".csv"))
        {
            filePath += ".csv";
        }
    }

    m_Thread->setname(filePath);
    m_Thread->quit();
    m_Thread->wait();
    m_Thread->start();

    QMessageBox *box=new QMessageBox(QMessageBox::Warning,QStringLiteral("警告"),QStringLiteral("文件导出中,等待提示完成"),QMessageBox::Yes|QMessageBox::No);
    //2000ms后用户未作选择，则自动取消
    QTimer::singleShot(2000,box,SLOT(accept()));
    if(box->exec()==QMessageBox::Yes)
    {
        //用户选择了YES后的操作
    }
    //new完后delete，防止内存泄漏
    delete box;
    box=NULL;
}

void HisWindow::slot_showSTimeLabel()
{
    QPoint origin = mapToGlobal(m_stime->pos());//原点
    QPoint terminal = QPoint(origin.x(), origin.y() + m_stime->height());//终点
    m_starttime->show();
    m_starttime->move(terminal);
    m_starttime->setCurrentTime();
    m_startedit->setText("");
}
void HisWindow::slot_showETimeLabel()
{
    QPoint origin = mapToGlobal(m_etime->pos());//原点
    QPoint terminal = QPoint(origin.x(), origin.y() + m_etime->height());//终点
    m_endttime->show();
    m_endttime->move(terminal);
    m_endttime->setCurrentTime();
    m_endedit->setText("");
}
void HisWindow::slot_recvTime(const QString& text)
{
    if (dynamic_cast<CustomTimeControl*>(QObject::sender()) == m_starttime)
    {
        m_startedit->setText(text);
    }
    else if (dynamic_cast<CustomTimeControl*>(QObject::sender()) == m_endttime)
    {
        m_endedit->setText(text);
    }
}
void HisWindow::slot_toDetails(const QModelIndex &index)
{
    int nRow = index.row();
    m_recordlanelab->setText(QStringLiteral("车道:"));
    m_recordcphmlab->setText(QStringLiteral("车牌号码:"));
    m_recordaxislab->setText(QStringLiteral("轴数:"));
    m_recordspeedlab->setText(QStringLiteral("车辆速度:"));
    m_recordcolorlab->setText(QStringLiteral("车牌颜色:"));
    m_recordcurtimelab->setText(QStringLiteral("检测时间:"));
    m_recordcartypelab->setText(QStringLiteral("车辆类型:"));
    m_recordweightlab->setText(QStringLiteral("总重量:"));
    m_recorddirectionlab->setText(QStringLiteral("行车方向:"));
    m_recordcrossroadlab->setText(QStringLiteral("是否跨道:"));
    m_recordwlab->setText(QStringLiteral("宽:"));
    m_recordhlab->setText(QStringLiteral("高:"));
    m_recordllab->setText(QStringLiteral("长:"));
    m_recordweight1lab->setText(QStringLiteral("轴1重:"));
    m_recordweight2lab->setText(QStringLiteral("轴2重:"));
    m_recordweight3lab->setText(QStringLiteral("轴3重:"));
    m_recordweight4lab->setText(QStringLiteral("轴4重:"));
    m_recordweight5lab->setText(QStringLiteral("轴5重:"));
    m_recordweight6lab->setText(QStringLiteral("轴6重:"));
    m_recordweight7lab->setText(QStringLiteral("轴7重:"));
    m_recordweight8lab->setText(QStringLiteral("轴8重:"));
    m_recordspace1lab->setText(QStringLiteral("轴1间距:"));
    m_recordspace2lab->setText(QStringLiteral("轴2间距:"));
    m_recordspace3lab->setText(QStringLiteral("轴3间距:"));
    m_recordspace4lab->setText(QStringLiteral("轴4间距:"));
    m_recordspace5lab->setText(QStringLiteral("轴5间距:"));
    m_recordspace6lab->setText(QStringLiteral("轴6间距:"));
    m_recordspace7lab->setText(QStringLiteral("轴7间距:"));
    m_recordxzwtlab->setText(QStringLiteral("限重:"));
    m_recorduploadlab->setText(QStringLiteral("是否上传:"));
    ResultRecord record;
    record.qstrcarno = m_modehis->record(nRow).value("CarNO").toString();
    record.qstrw = m_modehis->record(nRow).value("w").toString();
    record.qstrh = m_modehis->record(nRow).value("h").toString();
    record.qstrl = m_modehis->record(nRow).value("l").toString();
    record.qstrweight1 = m_modehis->record(nRow).value("AxlesWeight1").toString();
    record.qstrweight2 = m_modehis->record(nRow).value("AxlesWeight2").toString();
    record.qstrweight3 = m_modehis->record(nRow).value("AxlesWeight3").toString();
    record.qstrweight4 = m_modehis->record(nRow).value("AxlesWeight4").toString();
    record.qstrweight5 = m_modehis->record(nRow).value("AxlesWeight5").toString();
    record.qstrweight6 = m_modehis->record(nRow).value("AxlesWeight6").toString();
    record.qstrweight7 = m_modehis->record(nRow).value("AxlesWeight7").toString();
    record.qstrweight8 = m_modehis->record(nRow).value("AxlesWeight8").toString();
    record.qstrs1 = m_modehis->record(nRow).value("AxlesSpace1").toString();
    record.qstrs2 = m_modehis->record(nRow).value("AxlesSpace2").toString();
    record.qstrs3 = m_modehis->record(nRow).value("AxlesSpace3").toString();
    record.qstrs4 = m_modehis->record(nRow).value("AxlesSpace4").toString();
    record.qstrs5 = m_modehis->record(nRow).value("AxlesSpace5").toString();
    record.qstrs6 = m_modehis->record(nRow).value("AxlesSpace6").toString();
    record.qstrs7 = m_modehis->record(nRow).value("AxlesSpace7").toString();
    record.qstrcrossroad = m_modehis->record(nRow).value("crossroad").toString();
    record.qstrdirection = m_modehis->record(nRow).value("direction").toString();
    record.qstrcolor = m_modehis->record(nRow).value("PlateColor").toString();
    record.qstrweight = m_modehis->record(nRow).value("Weight").toString();
    record.qstrspeed = m_modehis->record(nRow).value("Speed").toString();
    record.qstrcartype = m_modehis->record(nRow).value("CarType").toString();
    record.qstraxis = m_modehis->record(nRow).value("Axles").toString();
    record.qstrtime = m_modehis->record(nRow).value("DetectionTime").toString();
    record.qstrlane = m_modehis->record(nRow).value("laneno").toString();
    record.qstrpicce = m_modehis->record(nRow).value("ImageBaseSide").toString();
    record.qstrpicsmall = m_modehis->record(nRow).value("ImageBaseCarNO").toString();
    record.qstrpicafr = m_modehis->record(nRow).value("ImageBaseBack").toString();
    record.qstrpicpre = m_modehis->record(nRow).value("ImageBaseFront").toString();
    record.qstrpicpre1 = m_modehis->record(nRow).value("ImageBasePanorama").toString();
    record.qstrpichce = m_modehis->record(nRow).value("vedioBaseSide").toString();
    record.qstrpicvideo = m_modehis->record(nRow).value("vedioBaseFront").toString();
    record.qstrVideoState = m_modehis->record(nRow).value("bUploadvedio").toString();
//    record.qstrUploadState = m_modehis->record(nRow).value("bUploadpic").toString();
    record.qstrUploadState = m_modehis->record(nRow).value("bUploadvedio").toString();
    record.qstrXz = m_modehis->record(nRow).value("XZ").toString();
    updateRecord(record);
}
void HisWindow::slot_onPlayRecord()
{
    char szRatio[32] = "";
    if(m_preTimer->isActive())
        onStop();

    if(m_PrePort!=-1)
        onStop();

    // 获取播放库端口号
    bool bFlag = PlayM4_GetPort(&m_PrePort);
    //打开待播放的文件
    bFlag = PlayM4_OpenFile(m_PrePort,(char*)m_strPre.toStdString().c_str());
    if (bFlag == false)
    {
        int nErr = PlayM4_GetLastError(m_PrePort);
        OutputLog("========Open file failed %d!",nErr);
        QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("前录像打开失败！"), QMessageBox::Ok);
        return;
    }
    //开始播放文件
    PlayM4_Play(m_PrePort, (HWND)m_recordcusVideo->winId());
    m_preTimer->start(1000);
}
void HisWindow::slot_thread_UpdateUi()
{
    QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("文件导出完成！"), QMessageBox::Ok);
}
void HisWindow::slot_timeout_prepointer()
{
    float nPos = 0;
    nPos = PlayM4_GetPlayPos(m_PrePort);
    OutputLog("pre Be playing... %.2f", nPos);
    if(nPos > 0.98)
        onStop();
}
void HisWindow::slot_timeout_pointer()
{
    float nPos = 0;
    nPos = PlayM4_GetPlayPos(m_AfrPort);
    OutputLog("afr Be playing... %.2f", nPos);
    if(nPos > 0.98)
        onStop(true);
}
void HisWindow::onStop(bool bFull)
{
    if(bFull)
    {
        m_Timer->stop();
        //停止播放
        PlayM4_Stop(m_AfrPort);
        //关闭文件
        PlayM4_CloseFile(m_AfrPort);
        PlayM4_FreePort(m_AfrPort);
        m_AfrPort  =  -1;
        m_showpiclab->update();
    }
    else
    {
        m_preTimer->stop();
        //停止播放
        PlayM4_Stop(m_PrePort);
        //关闭文件
        PlayM4_CloseFile(m_PrePort);
        PlayM4_FreePort(m_PrePort);
        m_PrePort  =  -1;
        m_recordcusVideo->update();
    }
}
/******************类WorkThread******************/
WorkThread::WorkThread(HisWindow *gui)
{
    m_gui = gui;
    connect(this, SIGNAL(onShowUI()), m_gui, SLOT(slot_thread_UpdateUi()));
}
WorkThread::~WorkThread()
{

}
void WorkThread::run()
{
    QFile file(m_strname) ;
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        return;
    }

    QString S = m_gui->m_startedit->text().left(10) + ",";
    QString E = m_gui->m_endedit->text().left(10)+ ",";

    QTextStream out(&file);
    out<<QStringLiteral("检测时间:,")<<S.toStdString().c_str()<<QStringLiteral("至," )<< E.toStdString().c_str()<<",\n" ;
    QString Stmp;
    int colu = m_gui->m_modehis->columnCount();
    for (int i=0;i<colu;i++)
    {
        if(!m_gui->m_viewhis->isColumnHidden(i))
        {
            Stmp= m_gui->m_modehis->headerData(i,Qt::Horizontal,Qt::DisplayRole).toString()+",";
            out<<Stmp;
        }
    }
    out<<",\n";
    //获取行数
    int iNum = m_gui->m_modehis->rowCount();
    //循环显示数据
    QString strVal;
    for (int i = 0; i < iNum; i++)
    {
        for(int k = 0;k<colu;k++)
        {
            if(!m_gui->m_viewhis->isColumnHidden(k))
            {
                strVal = m_gui->m_modehis->record(i).value(k).toString()+",";
                out<<strVal;
            }
        }
        out<<",\n";
    }
    file.close();

    emit onShowUI();
}
void WorkThread::setname(QString str)
{
    m_strname = str;
}
/******************类CustomModel******************/
CustomModel::CustomModel(QObject *parent, QSqlDatabase db):QSqlTableModel(parent,db)
{

}
QVariant CustomModel::data(const QModelIndex &idx, int role) const
{
    // 设置 Model 内容居中
    QVariant value = QSqlQueryModel::data(idx,role);
    if(Qt::TextAlignmentRole == role)
        value = Qt::AlignCenter;
    return value;
}
