#include "overwidget-old.h"
#include "ui_overwidget.h"
#include <QFile>
#include <QPainter>
#include <QDebug>
#include <QRect>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextCodec>
#include <QStorageInfo>

OverWidget *g_Over;

QQueue<QByteArray> afrQueue;
QQueue<QByteArray> ceQueue;
QQueue<QByteArray> hceQueue;
QQueue<QByteArray> fro1Queue;
QQueue<QByteArray> smallQueue;

#ifdef WIN32
void DropFile(const char* szfilename)
{
    if(DeleteFileA(szfilename)==0)
        OutputLog("%s delete error %d",szfilename, GetLastError());
//    else
//        OutputLog("%s delete success!!",szfilename);
}
#else
void DropFile(const char* szfilename)
{
    QFile file(QString::fromLocal8Bit(szfilename));
    file.remove();
}
#endif

/////////////////////////////////////////////////////////////////
//比较两个文件夹名，返回时间早的那一个
//文件夹名格式：YYYY-MM-DD，2012-10-03，长度必须是10，否则抛出异常
std::string MyCompare(std::string strFirst,std::string strSecond)
{
    if (strFirst.size()!=10||strSecond.size()!=10)
    {
        throw std::exception();
    }

    for (int i=0;i<10;++i)
    {
        if (strFirst[i]==strSecond[i])
        {
            continue;
        }
        if (strFirst[i]<strSecond[i])
        {
            return strFirst;
        }
        return strSecond;
    }
    return strFirst;
}

//输入一个保存路径，返回时间最早的文件夹名
//格式：E:\或D:\asdf
std::string FindEarliest(QString path,std::map<std::string, int> &DelFieldVec)
{
    std::string strRet("9999-12-31");	//本段代码到9999年12月31日24时之前有效
    if (!QFile::exists(path))
    {
        return strRet;
    }
    QDir sourceDir(path);
    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach (QFileInfo fileInfo, fileInfoList)
    {
        if (fileInfo.fileName() == "." || fileInfo.fileName() == "..")
        {
            continue;
        }
        if (fileInfo.isDir())
        {
            auto ifind = DelFieldVec.find(fileInfo.fileName().toStdString());
            if (ifind != DelFieldVec.end())
            {
                continue;		//这个文件夹删不掉
            }
            try
            {
                strRet = MyCompare(fileInfo.fileName().toStdString(),strRet);
            }
            catch(std::exception)
            {
                //保存路径中有不是本程序创建的文件夹，忽略掉
                continue;
            }
        }
    }

    return strRet;
}

//删除指定文件夹和文件夹下的内容，删除失败则返回false
//即使某个文件删除失败，也会继续将本文件夹中其他文件删除
bool MyDeleteDir(QString strDirToDel)
{
    QDir sourceDir(strDirToDel);
    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach (QFileInfo fileInfo, fileInfoList)
    {
        if (fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if (fileInfo.isDir())
        {
            QString	strSubDir = strDirToDel + PLATPATH + fileInfo.fileName();
            OutputLog("递归 %s",(const char*)strSubDir.toLocal8Bit());
            MyDeleteDir(strSubDir);
        }
        else
        {
            QString	strName = strDirToDel + PLATPATH + fileInfo.fileName();
            DropFile((const char*)strName.toLocal8Bit());
        }
    }
    QDir qDir(strDirToDel);
    return qDir.removeRecursively();
}
/////////////////////////////////////////////////////////////////
void* FlushThreadProc(LPVOID pParam);
void* VedioMsgThreadProc(LPVOID pParam);
void* HisVedioMsgThreadProc(LPVOID pParam);
void* SendMsgThreadProc(LPVOID pParam);
void* CheckStoreThreadProc(LPVOID pParam);
extern QSqlDatabase g_DB;

/////////////////////////////////////////////////////////////////
static pthread_mutex_t queue_cs;     /*互斥量*/
typedef std::queue<STR_MSG*>  MsgQueue;  //by mazq
MsgQueue g_msgQueue;

//使用线程池  add by mazq begin
void * routine(void * arg);
int pool_add_job(void *(*process)(void * arg),void *arg);
int pool_init(unsigned int thread_num);
int pool_destroy(void);
void *test(void *);

typedef struct job{
    void * (*process)(void *arg); /* process() will employed on job*/
    void *arg;/* argument to process */
    struct job * next;
}Job;

typedef struct thread_pool{
    pthread_mutex_t pool_lock;
    pthread_cond_t  job_ready;
    Job * jobs;
    int destroy;
    pthread_t * threads;
    unsigned int thread_num;
    int size;
}Thread_pool;

static Thread_pool * pool=NULL;

/////////////////////////////////////////////////////////////////////////////////////////mazq mazq
/*Initialize the thread pool*/
int pool_init(unsigned int thread_num)
{
    OutputLog("进入pool_init，Num = %d", thread_num);
    int nRet = 0;
    pool=(Thread_pool *)malloc(sizeof(Thread_pool));
    if(NULL == pool)
    {
        OutputLog("pool_init中 NULL==pool");
        return -1;
    }
    pthread_mutex_init(&(pool->pool_lock),NULL);
    pthread_cond_init(&(pool->job_ready),NULL);
    pool->jobs=NULL;   //具体业务处理
    pool->thread_num=thread_num;  //线程池线程总数目
    pool->size=0;    //当前线程数量
    pool->destroy=0;  //是否销毁， 1为销毁 0为不销毁
    pool->threads=(pthread_t *)malloc(thread_num * sizeof(pthread_t));  //线程ID
    if (NULL == pool->threads)
    {
        OutputLog("pool_init中 NULL==pool->threads");
        return -1;
    }

    int i;
    for(i=0;i<thread_num;i++)
    {
        nRet = pthread_create(&(pool->threads[i]),NULL,routine,NULL);
        if (nRet)
        {
            OutputLog("线程池中第%d个创建失败",i+1);
        }
        else
        {
//            LOG4CPLUS_ERROR(pTestLogger, "线程池中第" << i+1 <<"个创建成功");
        }
    }
    return 0;
}

/*
 * Add job into the pool
 * assign it to some thread
 */
int pool_add_job(void *(*process)(void *),void *arg)
{
    OutputLog("业务开始pool_add_job");
    //Job newjob = {};
    //newjob.process = process;
    //newjob.arg = arg;
    //newjob.next = NULL;
    Job * newjob=(Job *)malloc(sizeof(Job));
    if (NULL == newjob)
    {
        OutputLog("pool_add_job中 NULL == newjob");
        return -1;
    }
    newjob->process=process;  //业务处理函数名
    newjob->arg=arg;   //函数参数
    newjob->next=NULL;

    pthread_mutex_lock(&(pool->pool_lock));
    Job * temp=pool->jobs;
    if(temp!=NULL)
    {
        while(temp->next)
            temp=temp->next;
        temp->next = newjob;
    }
    else
    {
        pool->jobs = newjob;
    }

    /*For debug*/
    //assert(pool->jobs!=NULL);
    pool->size++;
    pthread_mutex_unlock(&(pool->pool_lock));
    /*Rouse a thread to process this new job*/
    pthread_cond_signal(&(pool->job_ready));
    return 0;
}
/*Destroy the thread pool*/
int pool_destroy(void)
{
    OutputLog("开始进行pool_destroy");
    /*Alread destroyed!*/
    if(pool->destroy)
        return -1;
    int i;
    pool->destroy=1;  //1代表销毁
    pthread_cond_broadcast(&(pool->job_ready));/*notify all threads*/
    for(i=0;i<pool->thread_num;i++)
        pthread_join(pool->threads[i],NULL);
    free(pool->threads);
    Job * head=NULL;
    while(pool->jobs)
    {
        head=pool->jobs;
        pool->jobs=pool->jobs->next;
        free(head);
    }
    pthread_mutex_destroy(&(pool->pool_lock));
    pthread_cond_destroy(&(pool->job_ready));
    free(pool);
    pool=NULL;
    return 0;
}
/*Every thread call this function*/
void * routine(void *arg)
{
    OutputLog("start thread: %u",pthread_self());
    while(1)
    {
        pthread_mutex_lock(&(pool->pool_lock));
        while(pool->size==0 && !pool->destroy)
        {
            OutputLog("thread is waiting: %u",pthread_self());
            pthread_cond_wait(&(pool->job_ready),&(pool->pool_lock));
        }
        if(pool->destroy)
        {
            OutputLog( "routine中因为pool->destroy执行unlock前");
            pthread_mutex_unlock(&(pool->pool_lock));
            OutputLog("routine中因为pool->destroy执行unlock后");
            OutputLog( "thread will exit: %u",pthread_self());
            pthread_exit(NULL);
        }
        OutputLog("thread is starting to work: %u",pthread_self());

        /*For debug*/
       // assert(pool->size!=0);
       // assert(pool->jobs!=NULL);
        pool->size--;
        OutputLog("pool->size = %d", pool->size);
        Job * job=pool->jobs;
        pool->jobs=job->next;

        pthread_mutex_unlock(&(pool->pool_lock));

        /*process job*/
        (*(job->process))(job->arg);  //此处为具体业务操作
        free(job);
        job=NULL;
    }
    /*You should never get here*/
    pthread_exit(NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////mazq mazq
int file_exists(char *filename)
{
    return (access(filename, 6) == 0);
}

//外廓
bool g_ProfileRet(int L,int W,int H,int lane,long etc, int etc2)
{
    g_Over->AnalysisProfileData(L,W,H,lane,etc,etc2);
    return 0;
}

void g_ProfileStateRet(int Status)
{
    g_Over->OutputLog2("外廓状态.......%d",Status);
    /////////////////////////////////////////////////////////////
    QString scal = Status==0?QStringLiteral("断线"):QStringLiteral("重连成功");
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString strdate = current_date_time.toString("yyyy-MM-dd hh:mm:ss"); //设置显示格式
    //保存数据
    QSqlQuery qsQuery = QSqlQuery(g_DB);
    QString strSqlText("INSERT INTO DEVSTATE (optiontime,name,state) VALUES (:optiontime,:name,:state)");
    qsQuery.prepare(strSqlText);
    qsQuery.bindValue(":optiontime", strdate);
    qsQuery.bindValue(":name", QStringLiteral("外廓设备"));
    qsQuery.bindValue(":state", scal);
    bool bsuccess = qsQuery.exec();
    if (!bsuccess)
    {
        QString querys = qsQuery.lastError().text();
        OutputLog("状态保存失败%s",querys.toStdString().c_str());
    }
}
//车检
void g_CarPass(int Lane, int Type)
{
//    if(Type == 1)
//        g_Over->OutputLog2("%d******到达******",Lane);
//    else
//    {
//        g_Over->OutputLog2("%d******离开******",Lane);
//        g_Over->m_DevCon.CarArrive(Lane,true);
//    }
    if(Lane == 4)
    {
        Lane = 1;
    }
    else if(Lane == 1)
    {
        Lane = 4;
    }

    if(Type == 1)
    {
        g_Over->OutputLog2("%d******到达******",Lane);
    }
    else
    {
        g_Over->OutputLog2("%d******离开******",Lane);
        g_Over->m_DevCon.CarArrive(Lane,true);
    }
}

void g_CarCamShoot(int Lane, char *filename, int pictype, char *carno, char *pcolor, char *cartype)
{
    g_Over->MSesGCallback(Lane,filename,pictype,carno,pcolor,cartype);
}

//称重
void g_CarWeight(const char* msg, int iUser, void *pUser)
{
    g_Over->AnalysisCarResult(msg);
}

void *process_request2(void *pmsg)
{
    OverWidget* pThis = g_Over;
    STR_MSG *pMsg = (STR_MSG *)pmsg;
    OutputLog("进入业务处理线程==========================%s",pMsg->strcarno);
    int iTC = ::GetTickCount();
    char *pUsr = NULL;
    pThis->m_CamCon.DownFile(pMsg->strtime,pMsg->laneno,pMsg->dataId,pMsg->strcarno,pUsr,1,1);
    OutputLog("业务处理线程exit  %s %d",pMsg->strcarno, ::GetTickCount()-iTC);
    memset(pMsg, 0, sizeof(STR_MSG));
    //此处不能释放，应该放回队列
    //free(soap);
    pthread_mutex_lock(&queue_cs);
    g_msgQueue.push(pMsg);
    OutputLog("处理完成,扔回队列，长度为:%d", g_msgQueue.size());
    pthread_mutex_unlock(&queue_cs);
    //pthread_exit(NULL);

    return NULL;
}

void *process_request(void *pmsg)
{
    OverWidget* pThis = g_Over;
    STR_MSG *pMsg = (STR_MSG *)pmsg;
    OutputLog("进入业务处理线程11==========================%s",pMsg->strcarno);
    int iTC = ::GetTickCount();
    char *pUsr = NULL;
    pThis->m_CamCon.DownFile(pMsg->strtime,pMsg->laneno,pMsg->dataId,pMsg->strcarno,pUsr,1,1);
    OutputLog("业务处理线程11exit  %s %d",pMsg->strcarno, ::GetTickCount()-iTC);
    //此处释放
    if(pMsg)
        delete pMsg;
    //pthread_exit(NULL);
    return NULL;
}

OverWidget::OverWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverWidget)
{
    ui->setupUi(this);
    g_Over = this;
    m_LaneNum=1;
    m_showError = true;
    m_bNeedFlush = false;
    m_GBXZ = 0;
    m_PrePort  = m_AfrPort = -1;
    bStart = true;

    this->InitOver();
}

OverWidget::~OverWidget()
{
    bStart = false;
    Sleep(200);

    delete ui;
}

void OverWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F7)
    {
        m_showError = true;
    }
    else if (event->key() == Qt::Key_F8)
    {
        m_showError = false;
    }
    else if (event->key() == Qt::Key_Up)
    {
//        char carno[] = "鲁B3Y0R2";
//        char sret[] = "合格";
//        m_LedCon.UpdateLed(carno,sret);
    }
    else if (event->key() == Qt::Key_Down)
    {
        m_LedCon.ResetLed();
    }
}

void OverWidget::slot_timeout_timer_pointer()
{
//    char strPath[256] = {};
//    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, strConfigfile.c_str());
//    QDateTime current_date_time = QDateTime::currentDateTime();
//    QString dir_str = QString(strPath)+PLATPATH+current_date_time.toString("yyyy-MM-dd");
//    QDir dir;
//    if (!dir.exists(dir_str))
//        dir.mkpath(dir_str);
//    QString dir_strCar = QString(strPath)+PLATPATH+current_date_time.toString("yyyy-MM-dd")+PLATPATH+"carinfo";
//    if (!dir.exists(dir_strCar))
//        dir.mkpath(dir_strCar);

#ifdef WIN32
#else
    char file_path[512] = {0};
    std::string shome = std::string(std::getenv("HOME"));
    strcpy(file_path,shome.c_str());
    strcat(file_path, "/VD/");
    if (file_path[strlen(file_path) - 1] != '/') {
        file_path[strlen(file_path)] = '/';
    }
    if(access(file_path, F_OK) != 0) {     //目录不存在
        std::string build_path ="mkdir -p ";
        build_path += file_path;
        system(build_path.c_str());
    }
    WritePrivateProfileString("SYS", "picpath", file_path, strConfigfile.c_str());
 #endif

    char szVal[100];
    {
        GetPrivateProfileString("DB", "数据库类型", "1", szVal, 100, strConfigfile.c_str());
        int type = atoi(szVal);
        GetPrivateProfileString("DB", "servername", "127.0.0.1", szVal, 100, strConfigfile.c_str());
        QString servername = QString::fromLocal8Bit(szVal);
        GetPrivateProfileString("DB", "DBnmae", "QODBC", szVal, 100, strConfigfile.c_str());
        QString DBname = QString::fromLocal8Bit(szVal);
        GetPrivateProfileString("DB", "ID", "sava", szVal, 100, strConfigfile.c_str());
        QString id = QString::fromLocal8Bit(szVal);
        GetPrivateProfileString("DB", "Password", "123456", szVal, 100, strConfigfile.c_str());
        QString pass = QString::fromLocal8Bit(szVal);
        m_DbCon.setDBname(type,servername,DBname,id,pass);
        g_DB = m_DbCon.Load();

        Sleep(200);
    }
    {
        GetPrivateProfileString("SYS", "相机类型", "1", szVal, 100, strConfigfile.c_str());
        m_CamCon.Load(atoi(szVal));
        m_CamCon.SetCallback(g_CarCamShoot);
        m_CamCon.Work();
        Sleep(200);
    }
    {
        GetPrivateProfileString("SYS", "仪表类型", "1", szVal, 100, strConfigfile.c_str());
        m_DevCon.Load(atoi(szVal));
        m_DevCon.SetCallback(g_CarWeight);
        m_DevCon.Work();
    }
    {
        GetPrivateProfileString("外廓", "是否使用", "0", szVal, 19, strConfigfile.c_str());
        if(atoi(szVal) == 1)
        {
            GetPrivateProfileString("外廓", "类型", "1", szVal, 19, strConfigfile.c_str());
            m_ProCon.Load(atoi(szVal));

            const char *strKey = "port";
            GetPrivateProfileString("外廓", strKey, "8888", szVal, 29, strConfigfile.c_str());
            int iPort = atoi(szVal);

            strKey = "ip";
            GetPrivateProfileString("外廓", strKey, "127.0.0.1", szVal, 29, strConfigfile.c_str());
            m_ProCon.SetCallBackFuc(g_ProfileRet);
            m_ProCon.SetStateCallBackFuc(g_ProfileStateRet);
            m_ProCon.Connect((char*)szVal,iPort);
        }
    }
    {
        GetPrivateProfileString("WEB", "upload", "0", szVal, 100, strConfigfile.c_str());
        if (atoi(szVal) == 1)
            m_NetCon[0].Load(2);
        GetPrivateProfileString("DW", "upload", "0", szVal, 100, strConfigfile.c_str());
        if (atoi(szVal) == 1)
            m_NetCon[4].Load(4);
        GetPrivateProfileString("GOV", "upload", "0", szVal, 100, strConfigfile.c_str());
        if (atoi(szVal) == 1)
            m_NetCon[1].Load(3);
        GetPrivateProfileString("SB", "upload", "0", szVal, 100, strConfigfile.c_str());
        if (atoi(szVal) == 1)
            m_NetCon[2].Load(1);
        GetPrivateProfileString("CQ", "upload", "0", szVal, 100, strConfigfile.c_str());
        if (atoi(szVal) == 1)
            m_NetCon[3].Load(5);
        GetPrivateProfileString("KS", "upload", "0", szVal, 100, strConfigfile.c_str());
        if (atoi(szVal) == 1)
            m_NetCon[5].Load(6);
        GetPrivateProfileString("AD", "upload", "0", szVal, 100, strConfigfile.c_str());
        if (atoi(szVal) == 1)
            m_NetCon[6].Load(7);
    }
    GetPrivateProfileString("车检", "是否使用", "0", szVal, 19, strConfigfile.c_str());
    if(atoi(szVal) == 1)
    {
        GetPrivateProfileString("SYS", "车检类型", "1", szVal, 100, strConfigfile.c_str());
        m_CarCon.Load(atoi(szVal));
        m_CarCon.Car_SetGasCallBackFuc(g_CarPass);
        m_CarCon.Car_Connect();
    }

    for (int i=1;i<=m_LaneNum;i++)
    {
        STR_MSG *pMsg = new STR_MSG;
        memset(pMsg, 0, sizeof(STR_MSG));
        m_maplaneAndmsg[i] = pMsg;
    }
    OutputLog2("m_maplaneAndmsg size = %d", m_maplaneAndmsg.size());

    {
        pthread_mutex_init(&queue_cs, NULL);
        int iNum = GetPrivateProfileInt("SYS", "DownNum", 50, strConfigfile.c_str());
        pool_init(iNum);
        for (int i=0;i<100;i++)
        {
            STR_MSG *pMsg = new STR_MSG;
            if (pMsg)
            {
                memset(pMsg,0,sizeof(STR_MSG));
                g_msgQueue.push(pMsg);
            }
            else
            {
                OutputLog("在%d上new STR_MSG失败!",i);
            }
        }
    }
    pthread_t m_dwHeartID2;//线程ID
    pthread_create(&m_dwHeartID2, nullptr, &SendMsgThreadProc, this);
    pthread_t m_dwHeartID6;//线程ID
    pthread_create(&m_dwHeartID6, nullptr, &CheckStoreThreadProc, this);

    if(m_VideoSave == 1)
    {
        pthread_t m_dwHeartID;//线程ID
        pthread_create(&m_dwHeartID, nullptr, &VedioMsgThreadProc, this);
        pthread_t m_dwHeartID8;//线程ID
        pthread_create(&m_dwHeartID8, nullptr, &HisVedioMsgThreadProc, this);        
    }
    StartLED();
}

int OverWidget::AnalysisProfileData(int l, int w, int h, int lane, long etc, int etc2)
{
    if(etc2 == 0)
    {
        //出队
        m_QueueSect.Lock();
        auto ifind = m_maplaneAndmsg.find(lane);
        if (ifind != m_maplaneAndmsg.end())
        {
            STR_MSG *msgdata = ifind->second;
            msgdata->w = w;
            msgdata->l = l;
            msgdata->h = h;
            msgdata->bprofile = true;
            msgdata->getticout = ::GetTickCount();
            msgdata->bstart = true;
            OutputLog2("车道%d 外廓长%d 宽%d 高%d ", lane, msgdata->l, msgdata->w, msgdata->h);
        }
        else
            OutputLog2("!!!车道%d 外廓长%d 宽%d 高%d ", lane, l, w, h);
        m_QueueSect.Unlock();
    }
    else  //1 come    2 go
    {
        g_CarPass(lane,etc2);
    }


    return 1;
}

int OverWidget::AnalysisCarResult(const char *ResponeValue)
{
    QString strData = QString::fromLocal8Bit(ResponeValue);
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(strData.toUtf8(),&parseJsonErr);
    if(parseJsonErr.error == QJsonParseError::NoError)
    {
        QJsonObject root_Obj = document.object();
        int lane = root_Obj["laneno"].toInt();
        //出队
        m_QueueSect.Lock();
        auto ifind = m_maplaneAndmsg.find(lane);
        if (ifind != m_maplaneAndmsg.end())
        {
            STR_MSG *msgdata = ifind->second;
            msgdata->laneno = lane;
            msgdata->axiscount = root_Obj["axiscount"].toInt();
            msgdata->carlen = root_Obj["carlen"].toInt();
            msgdata->speed = root_Obj["speed"].toInt();
            msgdata->weight = root_Obj["weight"].toInt();
            msgdata->Axis_weight[0] = root_Obj["Axis_weight1"].toInt();
            msgdata->Axis_weight[1] = root_Obj["Axis_weight2"].toInt();
            msgdata->Axis_weight[2] = root_Obj["Axis_weight3"].toInt();
            msgdata->Axis_weight[3] = root_Obj["Axis_weight4"].toInt();
            msgdata->Axis_weight[4] = root_Obj["Axis_weight5"].toInt();
            msgdata->Axis_weight[5] = root_Obj["Axis_weight6"].toInt();
            msgdata->Axis_weight[6] = root_Obj["Axis_weight7"].toInt();
            msgdata->Axis_weight[7] = root_Obj["Axis_weight8"].toInt();
            msgdata->Axis_space[0] = root_Obj["Axis_space1"].toDouble();
            msgdata->Axis_space[1] = root_Obj["Axis_space2"].toDouble();
            msgdata->Axis_space[2] = root_Obj["Axis_space3"].toDouble();
            msgdata->Axis_space[3] = root_Obj["Axis_space4"].toDouble();
            msgdata->Axis_space[4] = root_Obj["Axis_space5"].toDouble();
            msgdata->Axis_space[5] = root_Obj["Axis_space6"].toDouble();
            msgdata->Axis_space[6] = root_Obj["Axis_space7"].toDouble();
            strcpy(msgdata->deviceNo, root_Obj["deviceNo"].toString().toStdString().c_str());

            int iType = root_Obj["direction"].toInt();
            if (iType == 0)
                strcpy(msgdata->strdirection, "正向");
            else if (iType == 1)
                strcpy(msgdata->strdirection, "逆向");

            iType = root_Obj["crossroad"].toInt();
            if (iType == 0)
                strcpy(msgdata->strcrossroad, "正常");
            else if (iType == 1)
                strcpy(msgdata->strcrossroad, "跨道");

//            iType = root_Obj["cartype"].toInt();
//            if (iType == 0)
//                strcpy(msgdata->strcartype, "小型客车");
//            else if (iType == 1)
//                strcpy(msgdata->strcartype, "小型货车");
//            else if (iType == 2)
//                strcpy(msgdata->strcartype, "中型货车");
//            else if (iType == 3)
//                strcpy(msgdata->strcartype, "大型客车");
//            else if (iType == 4)
//                strcpy(msgdata->strcartype, "大型货车");
//            else if (iType == 5)
//                strcpy(msgdata->strcartype, "特大货车");

            msgdata->bweight = true;
            msgdata->getticout = ::GetTickCount();
            msgdata->bstart = true;
            OutputLog2("车道%d 轴数%d 车型%s 速度%d 重量%d  %s行驶  置信度%d",
                       msgdata->laneno, msgdata->axiscount, msgdata->strcartype, msgdata->speed,
                       msgdata->weight, msgdata->strcrossroad, msgdata->credible);
        }
        m_QueueSect.Unlock();
    }

    return 1;
}

void OverWidget::InitOver()
{
    this->setWindowFlags(Qt::Window \
    |Qt::FramelessWindowHint\
    |Qt::WindowSystemMenuHint\
    |Qt::WindowMinimizeButtonHint\
    |Qt::WindowMaximizeButtonHint);

    this->setAutoFillBackground(true);
    this->setGeometry(QApplication::desktop()->availableGeometry());
//    this->setFixedSize(1280,1024);

    //背景se
    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, Qt::white);
    this->setPalette(palette);

    QFile file(":/qss/widget.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    this->setStyleSheet(styleSheet);
    file.close();

    char szVal[100];
    std::string sPath = QCoreApplication::applicationDirPath().toStdString();
    strConfigfile = sPath + "/config.ini";
    GetPrivateProfileString("OVERXZ", "GB", "", szVal, 100, strConfigfile.c_str());
    m_GBXZ = atoi(szVal);
    GetPrivateProfileString("OVERXZ", "W", "2.550", szVal, 100, strConfigfile.c_str());
    m_GBXZW = atof(szVal);
    GetPrivateProfileString("OVERXZ", "L", "18.100", szVal, 100, strConfigfile.c_str());
    m_GBXZL = atof(szVal);
    GetPrivateProfileString("OVERXZ", "H", "4.000", szVal, 100, strConfigfile.c_str());
    m_GBXZH = atof(szVal);
    ::GetPrivateProfileString("SYS", "TimeOut", "", szVal,100,strConfigfile.c_str());
    m_Timeout = atoi(szVal);
    ::GetPrivateProfileString("SYS", "oknovideo", "0", szVal,100,strConfigfile.c_str());
    m_OkNoVideo = atoi(szVal);
    ::GetPrivateProfileString("SYS", "oknoshow", "0", szVal,100,strConfigfile.c_str());
    m_OkNoShow = atoi(szVal);
    ::GetPrivateProfileString("SYS", "unfullexit", "0", szVal,100,strConfigfile.c_str());
    m_UnFullExit = atoi(szVal);
    ::GetPrivateProfileString("SYS", "VideoSave", "1", szVal,100,strConfigfile.c_str());
    m_VideoSave = atoi(szVal);
    ::GetPrivateProfileString("SYS", "屏幕复位", "30000", szVal, 100,strConfigfile.c_str());
    m_TimeFlush = atoi(szVal);
    ::GetPrivateProfileString("SYS", "车道数量","", szVal,100,strConfigfile.c_str());
    m_LaneNum = atoi(szVal);
    ::GetPrivateProfileString("SYS", "站点名称", "科技治超", m_szAddr,256, strConfigfile.c_str());
    ::GetPrivateProfileString("SYS", "smallpass", "0", szVal,100,strConfigfile.c_str());
    m_SmallNoUp = atoi(szVal);
    ::GetPrivateProfileString("SYS", "wkzeropass", "0", szVal,100,strConfigfile.c_str());
    m_WkZeroNoUp = atoi(szVal);
    ::GetPrivateProfileString("SYS", "passenge", "0", szVal,100,strConfigfile.c_str());
    m_PassengerNoUp = atoi(szVal);
    ::GetPrivateProfileString("SYS", "weidu", "1", szVal, 100,strConfigfile.c_str());
    m_weidu = atoi(szVal);
    ::GetPrivateProfileString("SYS", "双侧", "0", szVal,100,strConfigfile.c_str());
    m_twoce = atoi(szVal);
    //主界面
    initTitle();
    initReal();
    initHis();
    m_bottomlay = new QHBoxLayout;
    m_bottomlay->addWidget(m_realwid);
    m_bottomlay->addWidget(m_hiswid);
    m_bottomlay->setSpacing(0);
    m_hiswid->hide();
    m_realwid->show();
    m_mainlayout = new QVBoxLayout(this);
    m_mainlayout->addWidget(m_titlewid);
    m_mainlayout->addLayout(m_dhtoplay);
    m_mainlayout->addLayout(m_bottomlay);
    m_mainlayout->setSpacing(5);
    m_mainlayout->setMargin(0);
    QObject::connect(this, SIGNAL(UpdateUISig(STR_MSG *)), this, SLOT(UpdateReal(STR_MSG *)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(LedUpdate(int,char *, char *)), this, SLOT(UpdateLedSlot(int,char *, char *)));

    m_Thread = new WorkThread(this);
    m_showwid = new QWidget;
    m_showwid->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_showpiclab = new PicLabel;
    m_showpiclab->setToolTip(QStringLiteral("按Esc返回"));
    m_piclay = new QHBoxLayout;
    m_piclay->addWidget(m_showpiclab);
    m_piclay->setMargin(0);
    m_showwid->setLayout(m_piclay);
    shortcut_exitfullscreen_esc = new QShortcut(QKeySequence("Esc"), m_showwid);
    QObject::connect(shortcut_exitfullscreen_esc, SIGNAL(activated()), m_showwid, SLOT(close()));

    m_preTimer = new QTimer;
    connect(m_preTimer, SIGNAL(timeout()), this, SLOT(slot_timeout_pre_pointer()));
    m_Timer = new QTimer;
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(slot_timeout_pointer()));

    m_proc = new QProcess(this);
    timer_pointer = new QTimer;
    timer_pointer->setSingleShot(true);
    timer_pointer->start(200);
    connect(timer_pointer, SIGNAL(timeout()), this, SLOT(slot_timeout_timer_pointer()));

    m_LedTimer = new QTimer;
    connect(m_LedTimer, SIGNAL(timeout()), this, SLOT(UpdateLedSlot(int,char*,char*)));
//    ResetLed();
    m_LedPageTimer = new QTimer;
    connect(m_LedPageTimer, SIGNAL(timeout()), this, SLOT(updateLedPage()));

}

void OverWidget::UpdateUI(STR_MSG *msg)
{
    emit UpdateUISig(msg);
}

void OverWidget::updateLedPage()
{
    m_LedCon.UpdateLedEx(nullptr);
    m_LedPageTimer->stop();
    m_LedTimer->start(10000);
}

void OverWidget::UpdateLedSlot(int iType, char *CarNo, char *ret)
{
    QSqlQuery msgQuery(g_DB);
    QString strQuery = QString::fromLocal8Bit("SELECT DetectionTime,CarNO,Weight FROM Task WHERE overWeight>0 and bUploadpic<>7 and CarNO<>'未识别' and CarNO<>'无车牌' order by id desc limit 1;");
    msgQuery.prepare(strQuery);
    bool overtime=false;
    QString oooo;
    if (!msgQuery.exec())
    {
        QString querys = msgQuery.lastError().text();
        OutputLog("KS---- Failed to query the records: %s",querys.toStdString().c_str());
    }
    else
    {
        int iCount = 0;
        if (msgQuery.last())
        {
            iCount = msgQuery.at() + 1;
            msgQuery.first();//重新定位指针到结果集首位
            msgQuery.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
            if(iCount > 0)
            {
//                int lineorc=1;
                while (msgQuery.next())
                {
                    QDateTime currentTime = QDateTime::currentDateTime();
                    QString targetTimeStr = msgQuery.value("DetectionTime").toString().toUtf8();
                    QDateTime targetTime = QDateTime::fromString(targetTimeStr, "yyyy-MM-dd hh:mm:ss");
                    qint64 diffSeconds = currentTime.secsTo(targetTime);
                    if (diffSeconds < 86400)
                    {
                        //没超过24小时，调用update()函数
                        QString carno1 = msgQuery.value("CarNO").toString().toUtf8();
                        int over1 = msgQuery.value("Weight").toString().toInt();
                        oooo+=carno1;
                        oooo+="-";
                        oooo+= QString::number(over1);
                        overtime=true;

                    }
//                    OutputLog("---------- json ----------: %s",oooo.toLocal8Bit().toStdString().c_str());
                    /*oooo+="-";
                    QString carno1 = msgQuery.value("CarNO").toString().toUtf8();
                    int over1 = msgQuery.value("overWeight").toString().toInt();
                    oooo+=carno1;
                    oooo+=QString::fromLocal8Bit(" 超载");
                    float result = over1/1000;
                    if(result<0.1)
                        result=0.1;
                    oooo+= QString::number(result, 'f', 1);
                    oooo+="t";
                    OutputLog("---------- json ----------: %f",result);*/
                }
            }
        }
    }
    if(overtime == false)
        m_LedCon.ResetLed();
    if(overtime == true)
    {
        m_LedCon.UpdateLedEx(oooo.toUtf8().data());
        m_LedPageTimer->start(10000);
        m_LedTimer->stop();
    }
}

int OverWidget::IsBlack(QString strcar)
{
    QSqlQuery query(g_DB);
    QString select_all_sql = QString("select * from BLACK where carno = '%1';").arg(strcar);
    query.prepare(select_all_sql);
    if (!query.exec())
    {
        qDebug() << query.lastError();
    }
    else
    {
        if(query.last())
        {
            int icount = query.at() + 1;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 0;
}
void OverWidget::initReal()
{
    //背景
    QPalette palette2;
    palette2.setColor(QPalette::Background, Qt::white);
    m_realwid = new QWidget;
    m_realwid->setAutoFillBackground(true);
    m_realwid->setPalette(palette2);

    //top
    m_realtopwid = new QGroupBox;
    m_realtopwid->setStyleSheet("QGroupBox{background-image:url(:/png/image/realtop.png);border: none;}QGroupBox*{background-image:url();}");
//    m_realtopwid->setFixedSize(1897,436);
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
    m_realmsgwid->setFixedSize(this->width()*0.8, 81);
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

    m_mode = new QStandardItemModel(m_realwid);
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
    m_realwid->setLayout(m_mainreallay);
}

void OverWidget::initHis()
{
    //背景
    QPalette palette2;
    palette2.setColor(QPalette::Background, Qt::white);
    m_hiswid = new QWidget;
    m_hiswid->setAutoFillBackground(true);
    m_hiswid->setPalette(palette2);

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
    m_modehis = new CustomModel(this, g_DB);
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
    m_hisrightwid->setFixedWidth(this->width()/3);
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
    m_hiswid->setLayout(m_mainlayhis);

    QObject::connect(m_querybut, SIGNAL(clicked()), this, SLOT(query()));
    QObject::connect(m_exportbut, SIGNAL(clicked()), this, SLOT(exportto()));
    QObject::connect(m_stime, SIGNAL(clicked()), this, SLOT(showSTimeLabel()));
    QObject::connect(m_etime, SIGNAL(clicked()), this, SLOT(showETimeLabel()));
    QObject::connect(m_starttime, SIGNAL(sendTime(const QString&)), this, SLOT(recvTime(const QString&)));
    QObject::connect(m_endttime, SIGNAL(sendTime(const QString&)), this, SLOT(recvTime(const QString&)));
    QObject::connect(m_viewhis, SIGNAL(clicked(QModelIndex)), this, SLOT(onDoubleClickedToDetails(QModelIndex)));
    QObject::connect(m_recordcusVideo, SIGNAL(clicked()), this, SLOT(onPlayRecord()));
    QObject::connect(m_recordcusCe, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusVideo, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusPre, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusPre1, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusHCe, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusAfr, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusSmall, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
}

void OverWidget::onFullProc()
{
    char szRatio[32];
    m_showwid->showFullScreen();
    if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusPre)
    {
        m_showpiclab->showpic(m_recordcusPre->m_bk);
    }
    else if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusPre1)
    {
        m_showpiclab->showpic(m_recordcusPre1->m_bk);
    }
    else if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusHCe)
    {
        m_showpiclab->showpic(m_recordcusHCe->m_bk);
    }
    else if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusAfr)
    {
        m_showpiclab->showpic(m_recordcusAfr->m_bk);
    }
    else if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusSmall)
    {
        m_showpiclab->showpic(m_recordcusSmall->m_bk);
    }
    else if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusCe)
    {
        m_showpiclab->showpic(m_recordcusCe->m_bk);
    }
    else if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusVideo)
    {
        if(m_preTimer->isActive())
            onStop();
        m_showpiclab->showpic(":/pnd/png/car_jc.png",1);

        // 获取播放库端口号
        bool bFlag = PlayM4_GetPort(&m_AfrPort);
        //打开待播放的文件
        bFlag = PlayM4_OpenFile(m_AfrPort,(char*)m_strPre.toStdString().c_str());
        if (bFlag == false)
        {
            int nErr = PlayM4_GetLastError(m_AfrPort);
            OutputLog2("========Open file failed %d!",nErr);
            QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("前录像打开失败！"), QMessageBox::Ok);
            return;
        }
        //开始播放文件
        PlayM4_Play(m_AfrPort, (HWND)m_showpiclab->winId());

        m_Timer->start(500);
    }
}

void OverWidget::slotUpdateUi()
{
    QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("文件导出完成！"), QMessageBox::Ok);
}

void OverWidget::updateRecord(ResultRecord &recod)
{
    m_strPre = recod.qstrpicvideo;
    recod.qstrpicpre1.replace(QString("/"),QString("\\"));
    m_recordcusPre->showpic(recod.qstrpicpre);
    m_recordcusPre1->showpic(recod.qstrpicpre1);
    m_recordcusHCe->showpic(recod.qstrpichce);
    m_recordcusAfr->showpic(recod.qstrpicafr);
    if(m_twoce == 1)
        m_recordcusSmall->showpic(recod.qstrpicce2);
    else
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

void OverWidget::onDoubleClickedToDetails(const QModelIndex &index)
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

void OverWidget::onPlayRecord()
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
        OutputLog2("========Open file failed %d!",nErr);
        QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("前录像打开失败！"), QMessageBox::Ok);
        return;
    }
    //开始播放文件
    PlayM4_Play(m_PrePort, (HWND)m_recordcusVideo->winId());
    m_preTimer->start(1000);
}

void OverWidget::onStop(bool bFull)
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

void OverWidget::slot_timeout_pre_pointer()
{
    float nPos = 0;
    nPos = PlayM4_GetPlayPos(m_PrePort);
    OutputLog2("pre Be playing... %.2f", nPos);
    if(nPos > 0.98)
        onStop();
}

void OverWidget::slot_timeout_pointer()
{
    float nPos = 0;
    nPos = PlayM4_GetPlayPos(m_AfrPort);
    OutputLog2("afr Be playing... %.2f", nPos);
    if(nPos > 0.98)
        onStop(true);
}

int OverWidget::StartLED()
{
    char strVal[100] = {};
    GetPrivateProfileString("LED", "type", "1", strVal, 100, strConfigfile.c_str());
    m_LedCon.Load(atoi(strVal));
    m_LedCon.Work();

    pthread_t m_dwHeartID;//线程ID
    pthread_create(&m_dwHeartID, nullptr, &FlushThreadProc, this);

    return 1;
}

void OverWidget::ResetLed()
{
    emit LedUpdate(3, nullptr,nullptr);
}

void OverWidget::UpdateLed(STR_MSG *msgdata)
{
    emit LedUpdate(1, msgdata->strcarno,msgdata->strret);
}

void* FlushThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    pThis->OutputLog2("刷新线程开启...");
    if(pThis->m_TimeFlush == -1)
        return 0;

    while(pThis->bStart)
    {
        Sleep(500);
        if (pThis->m_bNeedFlush)
        {
            if (::GetTickCount() - pThis->m_LastTic > pThis->m_TimeFlush)
            {
                pThis->OutputLog2("屏幕复位...");
                pThis->ResetLed();
                pThis->m_bNeedFlush = false;
            }
        }
    }

    return 0;
}

void OverWidget::UpdateReal(STR_MSG *msg)
{
    int iTC = ::GetTickCount();
    static int rownum = 0;
    if(rownum > 11)  //0~11 共12行
        rownum = 0;
    //插入新记录
    int co=0;
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->dataId),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString(msg->strtime),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::fromLocal8Bit(msg->strcarno),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::fromLocal8Bit(msg->strplatecolor),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->laneno),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->speed),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->l),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->w),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->h),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->axiscount),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->weight),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[0]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[1]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[2]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[3]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[4]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[5]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[6]),Qt::DisplayRole);
    m_mode->setData(m_mode->index(rownum,co++),QString::number(msg->Axis_weight[7]),Qt::DisplayRole);
    for(int i=0;i<co;i++)
        m_mode->item(rownum,i)->setTextAlignment(Qt::AlignCenter);

    QModelIndex mdidx = m_mode->index(rownum, 1); //获得需要编辑的单元格的位置
    m_view->setCurrentIndex(mdidx);	//设定需要编辑的单元格
    m_view->update();
    rownum++;

    char  szVal[1024] = {};
    sprintf(szVal,"检测时间:%s\t车牌号码:%10s\t轴数:%d\t车道:%d\t车速%4dkm/s\t总重量:%6dkg",msg->strtime,msg->strcarno,msg->axiscount,msg->laneno,msg->speed,msg->weight);
    m_wtmsglab->setText(QString::fromLocal8Bit(szVal));
    m_cusPre->showpic(QString::fromLocal8Bit(msg->strprepic));
    m_cusAfr->showpic(QString::fromLocal8Bit(msg->strafrpic));
    if(m_twoce == 0)
        m_cusCe->showpic(QString::fromLocal8Bit(msg->strsmallpic/*strcepic*/),1);
    else
        m_cusCe->showpic(QString::fromLocal8Bit(msg->strce2pic));
    m_cusAll->showpic(QString::fromLocal8Bit(msg->strcepic));
    int iTC2 = ::GetTickCount();
    OutputLog("use time= %d",iTC2-iTC);
}

void OverWidget::showSTimeLabel()
{
    QPoint origin = mapToGlobal(m_stime->pos());//原点
    QPoint terminal = QPoint(origin.x(), origin.y() + m_stime->height());//终点
    m_starttime->show();
    m_starttime->move(terminal);
    m_starttime->setCurrentTime();
    m_startedit->setText("");
}

void OverWidget::showETimeLabel()
{
    QPoint origin = mapToGlobal(m_etime->pos());//原点
    QPoint terminal = QPoint(origin.x(), origin.y() + m_etime->height());//终点
    m_endttime->show();
    m_endttime->move(terminal);
    m_endttime->setCurrentTime();
    m_endedit->setText("");
}

void OverWidget::query()
{
    OutputLog("query历史记录...");
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

void OverWidget::exportto()
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

void OverWidget::initTitle()
{
    m_titlewid = new QWidget;
    m_titlewid->setFixedHeight(112);
    m_titlewid->setAutoFillBackground(true);
    //背景图
    QPalette palette;
    QPixmap pixmap(":/png/image/topbj.png");
    palette.setBrush(QPalette::Window, QBrush(pixmap));
    m_titlewid->setPalette(palette);

    //设置图标底色和背景色一样
    QPalette myPalette;
    QColor myColor(0, 0, 0);
    myColor.setAlphaF(0);
    myPalette.setBrush(backgroundRole(), myColor);

    m_toplogo = new CustomPushButton;
    m_toplogo->setButtonBackground((":/png/image/logo.png"));
    m_closebut = new CustomPushButton;
    m_closebut->setButtonBackground((":/pnd/png/X.png"));
    m_minbut = new CustomPushButton;
    m_minbut->setButtonBackground((":/pnd/png/Min.png"));
    m_labtitle = new CustomPushButton;
    m_labtitle->setButtonBackground((":/png/image/toptitle.png"));
    m_labtitle->setPalette(myPalette);
    m_toplogo->setPalette(myPalette);
    m_closebut->setPalette(myPalette);
    m_minbut->setPalette(myPalette);
    QObject::connect(m_closebut, SIGNAL(clicked()), this, SLOT(onClose()));
    QObject::connect(m_minbut, SIGNAL(clicked()), this, SLOT(showMinimized()));

    m_toplayout = new QHBoxLayout;
    m_toplayout->addStretch();
    m_toplayout->addWidget(m_labtitle);
    m_toplayout->addStretch();
    m_toplayout->addWidget(m_toplogo,0,Qt::AlignTop);
    m_toplayout->addSpacing(20);
    m_toplayout->addWidget(m_minbut,0,Qt::AlignTop);
    m_toplayout->addWidget(m_closebut,0,Qt::AlignTop);
    m_toplayout->addSpacing(5);
    m_toplayout->setMargin(0);
    m_titlewid->setLayout(m_toplayout);

    m_dhtoplay = new QHBoxLayout;
    m_cReal = new CustomPushButton;
    m_cReal->setButtonBackground(":/png/image/real-press.png");
    m_cHis = new CustomPushButton;
    m_cHis->setButtonBackground(":/png/image/his.png");
    m_cReal->setPalette(myPalette);
    m_cHis->setPalette(myPalette);
    m_dhtoplay->addWidget(m_cReal);
    m_dhtoplay->addWidget(m_cHis);
    m_dhtoplay->addStretch();
    m_dhtoplay->setSpacing(10);
    m_dhtoplay->setMargin(0);
    QObject::connect(m_cReal, SIGNAL(clicked()), this, SLOT(cRealslot()));
    QObject::connect(m_cHis, SIGNAL(clicked()), this, SLOT(cHisslot()));
}

void OverWidget::cRealslot()
{
    m_cReal->setButtonBackground(":/png/image/real-press.png");
    m_cHis->setButtonBackground(":/png/image/his.png");
    m_hiswid->hide();
    m_realwid->show();
}

void OverWidget::cHisslot()
{
    m_cReal->setButtonBackground(":/png/image/real.png");
    m_cHis->setButtonBackground(":/png/image/his-press.png");
    m_realwid->hide();
    m_hiswid->show();
}

void OverWidget::recvTime(const QString& text)
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

void* CheckStoreThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);

    char strPath[256] = {};
    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, pThis->strConfigfile.c_str());
    QString sDir = strPath;
#ifdef WIN32
    sDir=sDir.left(1);
#else
    sDir="/home";
#endif

    GetPrivateProfileString("SYS", "RemainSize", "2", strPath, 100, pThis->strConfigfile.c_str());
    int iSize = atoi(strPath);

    while(pThis->bStart)
    {
        Sleep(5000);
        QList<QStorageInfo> storageInfoList = QStorageInfo::mountedVolumes();
        foreach(QStorageInfo storage ,storageInfoList)
        {
            QString sPerDir = storage.rootPath();
            if(sPerDir.indexOf(sDir)!= -1)
            {
                int iRemainSize = storage.bytesAvailable()/1024/1024/1024;
                if(iRemainSize < iSize)
                {
                    pThis->OutputLog2("%s盘空间%d不足，准备删除", sDir.toStdString().c_str(), iRemainSize);
                    /////////////////////////////////////////////////////////////////
                    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, pThis->strConfigfile.c_str());
                    std::string	strDelName = FindEarliest(QString(strPath),pThis->m_DelFialedVec);
                    QString strDelFullPath = QString(strPath) + PLATPATH +QString(strDelName.c_str());
                    pThis->OutputLog2("待删%s", (const char*)strDelFullPath.toLocal8Bit());
                    if (!MyDeleteDir(strDelFullPath))	//删不掉
                    {
                        OutputLog("无法删除%s,reason=%d", (const char*)strDelFullPath.toLocal8Bit(), GetLastError());
                        pThis->m_DelFialedVec[strDelName] = 1;
                    }
                    pThis->OutputLog2("空间下一轮");


                }
                else
                    pThis->OutputLog2("空间大小%d", iRemainSize);
            }
        }
    }

    return 0;
}

bool compareStrings(const char* str1, const char* str2) {
    bool issame= false;
    // 查找数字部分
    const char* digitStr1 = str1;
    const char* digitStr2 = str2;
    while (*digitStr1 != '\0' && *digitStr2 != '\0') {
        if(*digitStr1>47 && *digitStr1<57)
        {
            if(*digitStr2<47 || *digitStr2>57){
                issame=false;
                break;
            }
            if(atoi(digitStr1)==atoi(digitStr2)){
                issame=true;
//                OutputLog("--- compare true : %s  %s", str1,str2);
            }
            else{
                issame=false;
                break;
            }
        }
        digitStr1++;
        digitStr2++;
    }
    return issame;
}

void* SendMsgThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    pThis->OutputLog2("轮询线程开启...");

    char  szVal[200];
    GetPrivateProfileString("SYS", "IsCam", "1", szVal, 29, pThis->strConfigfile.c_str());
    bool bCamera = atoi(szVal);


    try
    {
        while (pThis->bStart)
        {
            Sleep(50);
            for (auto &it : pThis->m_maplaneAndmsg)
            {
                if (it.second->bstart)
                {
                    pThis->m_QueueSect.Lock();
                    bool bUpOver = false;
                    if(bCamera == 1)
                    {
//                        if (it.second->bpic && it.second->bpicpre && it.second->bweight)  //兰州
                        if (it.second->bpic && it.second->bpicpre && it.second->bweight && it.second->bpicsmall && it.second->bpicce && it.second->bpicpre1 && it.second->bpicce2)   //城阳
                        {
                            bUpOver=true;
                        }
                    }
                    else
                    {
                        if (it.second->bweight)
                        {
                            bUpOver=true;
                        }
                    }
                    if (bUpOver)
                    {
//                        pThis->m_QueueSect.Lock();
                        if (strlen(it.second->strcarno)<1)
                        {
                            pThis->OutputLog2("无车牌号");
                            strcpy(it.second->strplatecolor, "无");
                            strcpy(it.second->strcarno, "未识别");
                        }
                        if(1)
                        {
                            char sFilename[256] = {};    //====//     \\192.168.1.250\File\zzcal.ini
                            GetPrivateProfileString("CAL", "file", "./zzcal.ini", sFilename, 256, pThis->strConfigfile.c_str());

                            char sVal[256] = {};
                            GetPrivateProfileString("sys", "L", "3.5", sVal, 29, sFilename);
                            float fwc = atof(sVal);
                            OutputLog("%s  %.1f", sFilename,fwc);
                            srand(unsigned(time(0)));
//                            if (it.second->axiscount==6)
                            {
                                GetPrivateProfileString("six", "no", "111BP005", sVal, 29, sFilename);
                                if(strstr(it.second->strcarno, sVal) != NULL)
                                {
                                    it.second->axiscount=6;
                                    GetPrivateProfileString("six", "all", "39999", sVal, 29, sFilename);
                                    int iall = atoi(sVal);
                                    int iUpper = iall*fwc/100.0;
                                    int iBottom = iall*fwc*(-1)/100.0;
                                    int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                    int iaver = irall/6;
                                    OutputLog("six %d %d %d %d",iall,iUpper,iBottom,irall);
                                    GetPrivateProfileString("six", "z1", "5861", sVal, 29, sFilename);
                                    int z1 = atoi(sVal);
                                    GetPrivateProfileString("six", "z2", "6880", sVal, 29, sFilename);
                                    int z2 = atoi(sVal);
                                    GetPrivateProfileString("six", "z3", "6814", sVal, 29, sFilename);
                                    int z3 = atoi(sVal);
                                    GetPrivateProfileString("six", "z4", "7535", sVal, 29, sFilename);
                                    int z4 = atoi(sVal);
                                    GetPrivateProfileString("six", "z5", "7397", sVal, 29, sFilename);
                                    int z5 = atoi(sVal);
                                    GetPrivateProfileString("six", "z6", "6655", sVal, 29, sFilename);
                                    int z6 = atoi(sVal);
                                    it.second->Axis_weight[0] = z1+iaver;
                                    it.second->Axis_weight[1] = z2+iaver;
                                    it.second->Axis_weight[2] = z3+iaver;
                                    it.second->Axis_weight[3] = z4+iaver;
                                    it.second->Axis_weight[4] = z5+iaver;
                                    it.second->Axis_weight[5] = z6+iaver;
                                    it.second->Axis_weight[6] = 0;
                                    it.second->Axis_weight[7] = 0;
                                    it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]
                                            +it.second->Axis_weight[3]+it.second->Axis_weight[4]+it.second->Axis_weight[5];
                                }
                            }
//                            if (it.second->axiscount==6)
                            {
                                GetPrivateProfileString("six2", "no", "111BP005", sVal, 29, sFilename);
                                if(strstr(it.second->strcarno, sVal) != NULL)
                                {
                                    it.second->axiscount=6;
                                    GetPrivateProfileString("six2", "all", "39999", sVal, 29, sFilename);
                                    int iall = atoi(sVal);
                                    int iUpper = iall*fwc/100.0;
                                    int iBottom = iall*fwc*(-1)/100.0;
                                    int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                    int iaver = irall/6;
                                    OutputLog("six2 %d %d %d %d",iall,iUpper,iBottom,irall);
                                    GetPrivateProfileString("six2", "z1", "5861", sVal, 29, sFilename);
                                    int z1 = atoi(sVal);
                                    GetPrivateProfileString("six2", "z2", "6880", sVal, 29, sFilename);
                                    int z2 = atoi(sVal);
                                    GetPrivateProfileString("six2", "z3", "6814", sVal, 29, sFilename);
                                    int z3 = atoi(sVal);
                                    GetPrivateProfileString("six2", "z4", "7535", sVal, 29, sFilename);
                                    int z4 = atoi(sVal);
                                    GetPrivateProfileString("six2", "z5", "7397", sVal, 29, sFilename);
                                    int z5 = atoi(sVal);
                                    GetPrivateProfileString("six2", "z6", "6655", sVal, 29, sFilename);
                                    int z6 = atoi(sVal);
                                    it.second->Axis_weight[0] = z1+iaver;
                                    it.second->Axis_weight[1] = z2+iaver;
                                    it.second->Axis_weight[2] = z3+iaver;
                                    it.second->Axis_weight[3] = z4+iaver;
                                    it.second->Axis_weight[4] = z5+iaver;
                                    it.second->Axis_weight[5] = z6+iaver;
                                    it.second->Axis_weight[6] = 0;
                                    it.second->Axis_weight[7] = 0;
                                    it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]
                                            +it.second->Axis_weight[3]+it.second->Axis_weight[4]+it.second->Axis_weight[5];
                                }
                            }
//                            if (it.second->axiscount==4)
                            {
                                GetPrivateProfileString("four", "no", "2227886W", sVal, 29, sFilename);
                                if(strstr(it.second->strcarno, sVal) != NULL)
                                {
                                    it.second->axiscount=4;
                                    GetPrivateProfileString("four", "all", "39999", sVal, 29, sFilename);
                                    int iall = atoi(sVal);
                                    int iUpper = iall*fwc/100.0;
                                    int iBottom = iall*fwc*(-1)/100.0;
                                    int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                    int iaver = irall/4;
                                    OutputLog("four %d %d %d %d",iall,iUpper,iBottom,irall);

                                    GetPrivateProfileString("four", "z1", "4978", sVal, 29, sFilename);
                                    int z1 = atoi(sVal);
                                    GetPrivateProfileString("four", "z2", "5530", sVal, 29, sFilename);
                                    int z2 = atoi(sVal);
                                    GetPrivateProfileString("four", "z3", "9213", sVal, 29, sFilename);
                                    int z3 = atoi(sVal);
                                    GetPrivateProfileString("four", "z4", "9553", sVal, 29, sFilename);
                                    int z4 = atoi(sVal);
                                    it.second->Axis_weight[0] = z1+iaver;
                                    it.second->Axis_weight[1] = z2+iaver;
                                    it.second->Axis_weight[2] = z3+iaver;
                                    it.second->Axis_weight[3] = z4+iaver;
                                    it.second->Axis_weight[4] = 0;
                                    it.second->Axis_weight[5] = 0;
                                    it.second->Axis_weight[6] = 0;
                                    it.second->Axis_weight[7] = 0;
                                    it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]
                                            +it.second->Axis_weight[3];
                                }
                            }
                            //if (it.second->axiscount==3)
                            {
                                GetPrivateProfileString("three", "no", "2227886W", sVal, 29, sFilename);
                                if(strstr(it.second->strcarno, sVal) != NULL)
                                {
                                    it.second->axiscount=3;
                                    GetPrivateProfileString("three", "all", "39999", sVal, 29, sFilename);
                                    int iall = atoi(sVal);
                                    int iUpper = iall*fwc/100.0;
                                    int iBottom = iall*fwc*(-1)/100.0;
                                    int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                    int iaver = irall/3;
                                    OutputLog("three %d %d %d %d",iall,iUpper,iBottom,irall);

                                    GetPrivateProfileString("three", "z1", "4978", sVal, 29, sFilename);
                                    int z1 = atoi(sVal);
                                    GetPrivateProfileString("three", "z2", "5530", sVal, 29, sFilename);
                                    int z2 = atoi(sVal);
                                    GetPrivateProfileString("three", "z3", "9213", sVal, 29, sFilename);
                                    int z3 = atoi(sVal);
                                    it.second->Axis_weight[0] = z1+iaver;
                                    it.second->Axis_weight[1] = z2+iaver;
                                    it.second->Axis_weight[2] = z3+iaver;
                                    it.second->Axis_weight[3] = 0;
                                    it.second->Axis_weight[4] = 0;
                                    it.second->Axis_weight[5] = 0;
                                    it.second->Axis_weight[6] = 0;
                                    it.second->Axis_weight[7] = 0;
                                    it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2];
                                }
                            }
                            if (it.second->axiscount==2)
                            {
                                GetPrivateProfileString("two", "no", "333V1079", sVal, 29, sFilename);
                                if(strstr(it.second->strcarno, sVal) != NULL)
                                {
                                    it.second->axiscount=2;
                                    GetPrivateProfileString("two", "all", "39999", sVal, 29, sFilename);
                                    int iall = atoi(sVal);
                                    int iUpper = iall*fwc/100.0;
                                    int iBottom = iall*fwc*(-1)/100.0;
                                    int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                    int iaver = irall/2;
                                    OutputLog("two %d %d %d %d",iall,iUpper,iBottom,irall);

                                    GetPrivateProfileString("two", "z1", "3970", sVal, 29, sFilename);
                                    int z1 = atoi(sVal);
                                    GetPrivateProfileString("two", "z2", "9535", sVal, 29, sFilename);
                                    int z2 = atoi(sVal);
                                    it.second->Axis_weight[0] = z1+iaver;
                                    it.second->Axis_weight[1] = z2+iaver;
                                    it.second->Axis_weight[2] = 0;
                                    it.second->Axis_weight[3] = 0;
                                    it.second->Axis_weight[4] = 0;
                                    it.second->Axis_weight[5] = 0;
                                    it.second->Axis_weight[6] = 0;
                                    it.second->Axis_weight[7] = 0;
                                    it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1];
                                }
                            }
                            it.second->Axis_weight[0] = it.second->Axis_weight[0]/pThis->m_weidu*pThis->m_weidu;
                            it.second->Axis_weight[1] = it.second->Axis_weight[1]/pThis->m_weidu*pThis->m_weidu;
                            it.second->Axis_weight[2] = it.second->Axis_weight[2]/pThis->m_weidu*pThis->m_weidu;
                            it.second->Axis_weight[3] = it.second->Axis_weight[3]/pThis->m_weidu*pThis->m_weidu;
                            it.second->Axis_weight[4] = it.second->Axis_weight[4]/pThis->m_weidu*pThis->m_weidu;
                            it.second->Axis_weight[5] = it.second->Axis_weight[5]/pThis->m_weidu*pThis->m_weidu;
                            it.second->Axis_weight[6] = it.second->Axis_weight[6]/pThis->m_weidu*pThis->m_weidu;
                            it.second->Axis_weight[7] = it.second->Axis_weight[7]/pThis->m_weidu*pThis->m_weidu;
                            it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]+it.second->Axis_weight[3]
                                    +it.second->Axis_weight[4]+it.second->Axis_weight[5]+it.second->Axis_weight[6]+it.second->Axis_weight[7];
                        }

                        pThis->OutputLog2("数据准备完成 车道%d  %s", it.second->laneno, it.second->strcarno);
                        //无效处理
                        strcpy(it.second->strreason, " ");
                        int iRet = 0;
                        if(it.second->speed > 120)
                        {
                            strcpy(it.second->strreason, "速度异常");
                            iRet = 7;
                        }
                        if(it.second->weight < 500 || it.second->weight > 150000)
                        {
                            strcpy(it.second->strreason, "总重量异常");
                            iRet = 7;
                        }
                        if(strstr(it.second->strplatecolor, "蓝") != NULL)
                        {
                            if(it.second->axiscount != 2)
                            {
                                iRet = 7;
                                strcpy(it.second->strreason, "轴数异常2");
                            }
                            if(it.second->weight > 10000)
                            {
                                iRet = 7;
                                strcpy(it.second->strreason, "总重量异常2");
                            }
                        }
                        switch (it.second->axiscount) {
                        case 0:
                            iRet = 7;
                            strcpy(it.second->strreason, "轴数异常0");
                            break;
                        case 1:
                        case 7:
                        case 8:
                            iRet = 7;
                            strcpy(it.second->strreason, "轴数异常");
                            break;
                        case 2:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[0]>30000 || it.second->Axis_weight[1]>30000)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 3:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200
                                    || it.second->Axis_weight[0]>30000 || it.second->Axis_weight[1]>30000 || it.second->Axis_weight[2]>30000)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 4:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200 || it.second->Axis_weight[3]<200
                                    || it.second->Axis_weight[0]>30000 || it.second->Axis_weight[1]>30000 || it.second->Axis_weight[2]>30000 || it.second->Axis_weight[3]>30000)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 5:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200
                                    || it.second->Axis_weight[3]<200 || it.second->Axis_weight[4]<200 || it.second->Axis_weight[0]>30000 || it.second->Axis_weight[1]>30000 || it.second->Axis_weight[2]>30000
                                    || it.second->Axis_weight[3]>30000 || it.second->Axis_weight[4]>30000)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 6:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200
                                    || it.second->Axis_weight[3]<200 || it.second->Axis_weight[4]<200 || it.second->Axis_weight[5]<200
                                    || it.second->Axis_weight[0]>30000 || it.second->Axis_weight[1]>30000 || it.second->Axis_weight[2]>30000
                                    || it.second->Axis_weight[3]>30000 || it.second->Axis_weight[4]>30000 || it.second->Axis_weight[5]>30000)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        default:
                            break;
                        }
                        if(it.second->axiscount > 8)
                        {
                            iRet = 7;
                            strcpy(it.second->strreason, "轴数异常3");
                        }
                        //mazq 0324
                        /*if(strstr(it.second->strplatecolor, "黄") != NULL)
                        {
                            if(it.second->weight < 5000)
                            {
                                iRet = 7;
                                strcpy(it.second->strreason, "总重量异常4");
                            }
                        }*/

                        if(strstr(it.second->strcartype, "小型") != NULL)
                        {
                            if(it.second->axiscount > 2)
                            {
                                iRet = 7;
                                strcpy(it.second->strreason, "轴数异常6");
                            }
                        }

                        if(iRet != 7)
                        {
                            if (it.second->w < 1 || it.second->h < 1 || it.second->l < 1)
                            {
                                if(pThis->m_WkZeroNoUp == 1)
                                {
                                    iRet = 5;
                                    strcpy(it.second->strreason, "外廓异常");
                                }
                            }
                            //客车不要
                            if(strstr(it.second->strcartype,"客") != NULL && pThis->m_PassengerNoUp==1)
                                iRet = 5;
                            //小车不要
                            if(strstr(it.second->strcartype,"小") != NULL && pThis->m_SmallNoUp==1)
                                iRet = 5;
                        }
                        //超重
                        strcpy(it.second->strret, "合格");
                        if(0)
                        {
                            if (it.second->weight > pThis->m_GBXZ)
                            {
                                strcpy(it.second->strret, "不合格");
                                it.second->overWeight=it.second->weight - pThis->m_GBXZ;
                                it.second->overPercent=(it.second->overWeight/pThis->m_GBXZ)*100.0f;
                            }
                            else
                                strcpy(it.second->strret, "合格");
                        }
                        else
                        {
                            int GBxz = 18000;
                            if(it.second->axiscount == 6)
                                GBxz = 49000;
                            else if(it.second->axiscount == 5)
                                GBxz = 43000;
                            else if(it.second->axiscount == 4)
                            {
//                                GBxz = 36000;
                                GBxz = 31000;
                                if(it.second->Axis_space[0]<215 && it.second->Axis_space[2]<145)
                                    GBxz = 31000;
                            }
                            else if(it.second->axiscount == 3)
                            {
//                                GBxz = 27000;
                                GBxz = 25000;
                                if(it.second->Axis_space[0]<215 || it.second->Axis_space[1]<140)
                                    GBxz = 25000;
                            }
                            it.second->GBxz = GBxz;
                            if (it.second->weight > GBxz)
                            {
                                strcpy(it.second->strret, "不合格");
                                it.second->overWeight=it.second->weight - GBxz;
                                it.second->overPercent=(it.second->overWeight/GBxz)*100.0f;
                            }
                            else
                                strcpy(it.second->strret, "合格");
                        }
                        //超限
                        if (it.second->w > pThis->m_GBXZW*1000)
                            strcpy(it.second->strret, "不合格");
                        if (it.second->l > pThis->m_GBXZL*1000)
                            strcpy(it.second->strret, "不合格");
                        if (it.second->h > pThis->m_GBXZH*1000)
                            strcpy(it.second->strret, "不合格");

                        int flag_aft=1,flag_ce=1,flag_hce=1,flag_fro2=1,flag_small=1;
                        if(strcmp(it.second->strcarno,"未识别") != 0 && strcmp(it.second->strcarno,"无车牌") != 0)    //城阳
                        {
                            //前后不一致
                            if(strcmp(it.second->strcarno2, it.second->strcarno) != 0)
                            {
                                if((strstr(it.second->strcarno2,"挂") != NULL))
                                {
                                    if(strstr(it.second->strcarno, "无") != NULL)
                                        strcpy(it.second->strcarno, it.second->strcarno2);
                                }
                                else
                                {
                                    OutputLog("%s ***pass*** hou %s", it.second->strcarno, it.second->strcarno2);
                                    flag_aft=0;
                                    //遍历队列是否有该车的照片
                                    QQueue<QByteArray>::const_iterator iter;
                                    for (iter = afrQueue.constBegin(); iter != afrQueue.constEnd(); ++iter) {
//                                        OutputLog("ks----- str %s:%s ", it.second->strcarno,iter->data()+39);
//                                        if(strstr(iter->data(), it.second->strcarno+4) != NULL){
                                        if( compareStrings(iter->data()+39,it.second->strcarno)){
                                            OutputLog("ks----- houuu TTTTTT %s:%s",iter->data(),it.second->strafrpic);
                                            strcpy(it.second->strafrpic, iter->data());
                                            flag_aft=1;
                                            break;
                                        }
                                        else{
                                            flag_aft=0;
                                        }
                                    }
                                }
                            }
                            //前小不一致
                            if(strcmp(it.second->strcarno, it.second->strcarnos) != 0)
                            {
//                                if(strstr(it.second->strcarnos,"无") != NULL)
//                                {
//                                }
//                                else //if (strlen(it.second->strcarnos)>1)
                                {
                                    OutputLog("%s ***pcss*** small %s", it.second->strcarno, it.second->strcarnos);
                                    flag_small=0;
                                    //遍历队列是否有该车的照片
                                    QQueue<QByteArray>::const_iterator iter;
                                    for (iter = smallQueue.constBegin(); iter != smallQueue.constEnd(); ++iter) {
//                                        OutputLog("ks----- small %s:%s ", it.second->strcarno,iter->data()+39);
//                                        if (strstr(iter->data(), it.second->strcarno+4) != NULL){
                                        if( compareStrings(iter->data()+39,it.second->strcarno)){
                                            OutputLog("ks----- small TTTTTT %s:%s",iter->data(),it.second->strsmallpic);
                                            strcpy(it.second->strsmallpic, iter->data());
                                            flag_small=1;
                                            break;
                                        }
                                        else{
                                            flag_small=0;
                                        }
                                    }
                                }
                            }
                            //前侧不一致
                            if(strcmp(it.second->strcarno, it.second->strcarnoce) != 0)
                            {
//                                if(strstr(it.second->strcarnoce,"无") != NULL)
//                                {
//                                }
//                                else //if (strlen(it.second->strcarnoce)>1)
                                {
                                    OutputLog("%s ***pcss*** ce %s", it.second->strcarno, it.second->strcarnoce);
                                    flag_ce=0;
                                    //遍历队列是否有该车的照片
                                    QQueue<QByteArray>::const_iterator iter;
                                    for (iter = ceQueue.constBegin(); iter != ceQueue.constEnd(); ++iter) {
//                                        OutputLog("ks----- cestr %s:%s ", it.second->strcarno,iter->data()+39);
//                                        if (strstr(iter->data(), it.second->strcarno+4) != NULL){
                                        if( compareStrings(iter->data()+39,it.second->strcarno)){
                                            OutputLog("ks----- ceee TTTTTT %s:%s",iter->data(),it.second->strcepic);
                                            strcpy(it.second->strcepic, iter->data());
                                            flag_ce=1;
                                            break;
                                        }
                                        else{
                                            flag_ce=0;
                                        }
                                    }
                                }
                            }
                            //cpl 2024-1-24 kunshan
                            //前hou侧不一致
                            if(strcmp(it.second->strcarno, it.second->strcarnohce) != 0)
                            {
                                if((strstr(it.second->strcarnohce,"挂") != NULL))
                                {

                                }
                                else //if (strlen(it.second->strcarnohce)>1)
                                {
                                    OutputLog("%s ***pcss*** hce %s", it.second->strcarno, it.second->strcarnohce);
                                    //将不符合这个车的照片刚入队列
                                    flag_hce=0;
                                    //遍历队列是否有该车的照片
                                    QQueue<QByteArray>::const_iterator iter;
                                    for (iter = hceQueue.constBegin(); iter != hceQueue.constEnd(); ++iter) {
//                                        OutputLog("ks----- hcestr %s:%s ", it.second->strcarno,iter->data()+39);
//                                        if (strstr(iter->data(), it.second->strcarno+4) != NULL){
                                        if( compareStrings(iter->data()+39,it.second->strcarno)){
                                            OutputLog("ks----- hceee TTTTTT %s:%s",iter->data(),it.second->strhoucepic);
                                            strcpy(it.second->strhoucepic, iter->data());
                                            flag_hce=1;
                                            break;
                                        }
                                        else{
                                            flag_hce=0;
                                        }
                                    }
                                }
                            }
                            //前和前2不一致
                            if(strcmp(it.second->strcarno, it.second->strcarno3) != 0)
                            {
//                                if(strstr(it.second->strcarno3,"无") != NULL)
//                                {
//                                }
//                                else //if (strlen(it.second->strcarno3)>1)
                                {
                                    OutputLog("%s ***pcss*** qian1 %s", it.second->strcarno, it.second->strcarno3);
                                    //将不符合这个车的照片刚入队列
                                    flag_fro2=0;
                                    //遍历队列是否有该车的照片
                                    QQueue<QByteArray>::const_iterator iter;
                                    for (iter = fro1Queue.constBegin(); iter != fro1Queue.constEnd(); ++iter) {
//                                        OutputLog("ks----- hcestr %s:%s ", it.second->strcarno,iter->data()+39);
//                                        if (strstr(iter->data(), it.second->strcarno+4) != NULL){
                                        if( compareStrings(iter->data()+39,it.second->strcarno)){
                                            OutputLog("ks----- qian11 TTTTTT %s:%s",iter->data(),it.second->strprepic1);
                                            strcpy(it.second->strprepic1, iter->data());
                                            flag_fro2=1;
                                            break;
                                        }
                                        else{
                                            flag_fro2=0;
                                        }
                                    }
                                }
                            }
                        }

                        //保存数据
                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                        pThis->OutputLog2("保存分析任务 %s", it.second->strcarno);
                        QString strInsert = "INSERT INTO Task(DetectionTime,CarNO,laneno,Axles,CarType,deviceNo,carLength,Speed,Weight,PlateColor,DetectionResult,ImageBaseFront,ImageBaseBack,ImageBaseSide,\
                                ImageBasePanorama,ImageBaseCarNO,AxlesWeight1,AxlesWeight2,AxlesWeight3,AxlesWeight4,AxlesWeight5,AxlesWeight6,AxlesWeight7,AxlesWeight8,AxlesSpace1,AxlesSpace2,AxlesSpace3,AxlesSpace4,AxlesSpace5,AxlesSpace6,AxlesSpace7,\
                                bUploadpic,bUploadvedio,webId,overWeight,overPercent,direction,crossroad,XZ,reason,w,h,l,black,vedioBaseSide) VALUES(:DetectionTime,:CarNO,:laneno,:Axles,:CarType,:deviceNo,:carLength,:Speed,:Weight,:PlateColor,:DetectionResult,:ImageBaseFront,:ImageBaseBack,:ImageBaseSide,\
                                                                :ImageBasePanorama,:ImageBaseCarNO,:AxlesWeight1,:AxlesWeight2,:AxlesWeight3,:AxlesWeight4,:AxlesWeight5,:AxlesWeight6,:AxlesWeight7,:AxlesWeight8,\
                                                                :AxlesSpace1,:AxlesSpace2,:AxlesSpace3,:AxlesSpace4,:AxlesSpace5,:AxlesSpace6,:AxlesSpace7,:bUploadpic,:bUploadvedio,:webId,:overWeight,:overPercent,:direction,:crossroad,:XZ,:reason,:w,:h,:l,:black,:vedioBaseSide); ";
                        qsQuery.prepare(strInsert);
                        qsQuery.bindValue(":DetectionTime", it.second->strtime);
                        qsQuery.bindValue(":CarNO", QString::fromLocal8Bit(it.second->strcarno));
                        qsQuery.bindValue(":laneno", it.second->laneno);
                        qsQuery.bindValue(":Axles", it.second->axiscount);
                        qsQuery.bindValue(":CarType", QString::fromLocal8Bit(it.second->strcartype));
                        qsQuery.bindValue(":deviceNo", QString::fromLocal8Bit(it.second->deviceNo));
                        qsQuery.bindValue(":carLength", it.second->carlen);
                        qsQuery.bindValue(":Speed", it.second->speed);
                        qsQuery.bindValue(":Weight", it.second->weight);
                        qsQuery.bindValue(":PlateColor", QString::fromLocal8Bit(it.second->strplatecolor));
                        qsQuery.bindValue(":DetectionResult", QString::fromLocal8Bit(it.second->strret));
                        qsQuery.bindValue(":ImageBaseFront", QString::fromLocal8Bit(it.second->strprepic));
                        if(flag_aft==0){
                            qsQuery.bindValue(":ImageBaseBack", QString::fromLocal8Bit(""));
                        }
                        else{
                            qsQuery.bindValue(":ImageBaseBack", QString::fromLocal8Bit(it.second->strafrpic));
                        }

                        if(flag_ce==0){
                            qsQuery.bindValue(":ImageBaseSide", QString::fromLocal8Bit(""));
                        }
                        else{
                            qsQuery.bindValue(":ImageBaseSide", QString::fromLocal8Bit(it.second->strcepic));
                        }

                        if(flag_hce==0){
                            qsQuery.bindValue(":vedioBaseSide", QString::fromLocal8Bit(""));
                        }
                        else{
                            qsQuery.bindValue(":vedioBaseSide", QString::fromLocal8Bit(it.second->strhoucepic));
                        }

                        if(flag_fro2==0){
                            qsQuery.bindValue(":ImageBasePanorama", QString::fromLocal8Bit(""));
                        }
                        else{
                            qsQuery.bindValue(":ImageBasePanorama", QString::fromLocal8Bit(it.second->strprepic1));
                        }

                        if(flag_small==0){
                            qsQuery.bindValue(":ImageBaseCarNO", QString::fromLocal8Bit(""));
                        }
                        else{
                            qsQuery.bindValue(":ImageBaseCarNO", QString::fromLocal8Bit(it.second->strsmallpic));
                        }
//                        qsQuery.bindValue(":ImageBaseCarNO", QString::fromLocal8Bit(it.second->strsmallpic));
                        qsQuery.bindValue(":AxlesWeight1", it.second->Axis_weight[0]);
                        qsQuery.bindValue(":AxlesWeight2", it.second->Axis_weight[1]);
                        qsQuery.bindValue(":AxlesWeight3", it.second->Axis_weight[2]);
                        qsQuery.bindValue(":AxlesWeight4", it.second->Axis_weight[3]);
                        qsQuery.bindValue(":AxlesWeight5", it.second->Axis_weight[4]);
                        qsQuery.bindValue(":AxlesWeight6", it.second->Axis_weight[5]);
                        qsQuery.bindValue(":AxlesWeight7", it.second->Axis_weight[6]);
                        qsQuery.bindValue(":AxlesWeight8", it.second->Axis_weight[7]);
                        qsQuery.bindValue(":AxlesSpace1", it.second->Axis_space[0]);
                        qsQuery.bindValue(":AxlesSpace2", it.second->Axis_space[1]);
                        qsQuery.bindValue(":AxlesSpace3", it.second->Axis_space[2]);
                        qsQuery.bindValue(":AxlesSpace4", it.second->Axis_space[3]);
                        qsQuery.bindValue(":AxlesSpace5", it.second->Axis_space[4]);
                        qsQuery.bindValue(":AxlesSpace6", it.second->Axis_space[5]);
                        qsQuery.bindValue(":AxlesSpace7", it.second->Axis_space[6]);
                        qsQuery.bindValue(":bUploadpic", iRet);
                        qsQuery.bindValue(":bUploadvedio", iRet);
                        qsQuery.bindValue(":direction", QString::fromLocal8Bit(it.second->strdirection));
                        qsQuery.bindValue(":crossroad", QString::fromLocal8Bit(it.second->strcrossroad));
                        qsQuery.bindValue(":webId", 0);
                        qsQuery.bindValue(":overWeight", it.second->overWeight);
                        qsQuery.bindValue(":overPercent", QString::number(it.second->overPercent,'f',2));
                        qsQuery.bindValue(":XZ", it.second->GBxz);
                        qsQuery.bindValue(":w", it.second->w);
                        qsQuery.bindValue(":h", it.second->h);
                        qsQuery.bindValue(":l", it.second->l);
                        qsQuery.bindValue(":reason", QString::fromLocal8Bit(it.second->strreason));
                        bool bsuccess = qsQuery.exec();
                        if (!bsuccess)
                        {
                            QString querys = qsQuery.lastError().text();
                            pThis->OutputLog2("车辆保存失败%s",querys.toStdString().c_str());
                        }
                        QString strSelect = "Select max(id) from Task";
                        QSqlQuery query(g_DB);
                        query.prepare(strSelect);
                        if (!query.exec())
                        {
                            QString querys = query.lastError().text();
                            pThis->OutputLog2("查询最新记录失败 %s",querys.toStdString().c_str());
                        }
                        else
                        {
                            int nFID = -1;
                            int iCount = 0;
                            if (query.last())
                            {
                                iCount = query.at() + 1;
                            }
                            query.first();//重新定位指针到结果集首位
                            query.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
                            {
                                while (query.next())
                                {
                                    nFID = query.value(0).toInt();
                                }
                            }
                            pThis->OutputLog2("%s 记录ID=%d", it.second->strcarno, nFID);
                            //录像
                            STR_MSG msg = {0};
                            memcpy(&msg, it.second, sizeof(STR_MSG));
                            msg.dataId = nFID;
                            if(strlen(it.second->strtime)>5){
                                //if(msg.axiscount != 0 || !strcmp(it.second->strcarno,msg.strcarno))
                                    pThis->UpdateUI(&msg);
                            }
                            if(strstr(msg.strret,"不") == NULL && pThis->m_OkNoShow==1)
                            {
                                //合格且设置了不上屏
                            }
                            else
                            {
                                if(msg.overWeight > 0)
                                {
                                    pThis->OutputLog2("========update led==========");
                                    pThis->UpdateLed(&msg);
                                    pThis->m_bNeedFlush = true;
                                    pThis->m_LastTic = ::GetTickCount();
                                }else
                                    pThis->ResetLed();
                            }
                            if(pThis->m_VideoSave == 1)
                            {
                                if(strstr(msg.strret,"不") == NULL && pThis->m_OkNoVideo==1)
                                {
                                    //合格且设置了不录像
                                    pThis->OutputLog2("合格不录像 %s", it.second->strcarno);
                                }
                                else
                                {
                                    if(iRet == 0)  //7  5的都不处理了
                                    {
                                        pThis->m_QueueSectVedio.Lock();
                                        pThis->m_MsgVedioQueue.push(msg);
                                        pThis->m_QueueSectVedio.Unlock();
                                        pThis->OutputLog2("启动录像 %s, real size=%d", it.second->strcarno, pThis->m_MsgVedioQueue.size());
                                    }
                                }
                            }
                        }

                        while(ceQueue.size() > 50) {
                            ceQueue.dequeue();
                        }
                        while(afrQueue.size() > 50) {
                            afrQueue.dequeue();
                        }
                        while(fro1Queue.size() > 50) {
                            fro1Queue.dequeue();
                        }
                        while(hceQueue.size() > 50) {
                            hceQueue.dequeue();
                        }
                        while(smallQueue.size() > 50) {
                            smallQueue.dequeue();
                        }
                        //下个过车
                        memset(it.second, 0, sizeof(STR_MSG));
                        pThis->OutputLog2("================================================");
//                        pThis->m_QueueSect.Unlock();
                    }
                    else
                    {
                        if (::GetTickCount() - it.second->getticout > pThis->m_Timeout)
                        {
                            if ((it.second->bpic || it.second->bpicpre || it.second->bpicsmall || it.second->bpicce || it.second->bpicpre1 || it.second->bpicce2)  && pThis->m_UnFullExit==0) //&& it.second->bweight
                            {
                                pThis->OutputLog2("超时已有bpic carno:%s 前%d  后%d 小%d 侧%d 前1%d 后侧%d W%d  UF%d",it.second->strcarno, it.second->bpicpre, it.second->bpic,it.second->bpicsmall,it.second->bpicce,it.second->bpicpre1,it.second->bpicce2,it.second->bweight, pThis->m_UnFullExit);
//                                if(it.second->bpic == false)
//                                    strcpy(it.second->strafrpic, it.second->strprepic);
//                                if(it.second->bpicpre == false)
//                                {
//                                    pThis->OutputLog2("后车牌赋值前");
//                                    strcpy(it.second->strcarno, it.second->strcarno2);
//                                    strcpy(it.second->strprepic, it.second->strafrpic);
//                                }
                                it.second->bpicce = it.second->bpic = it.second->bpicpre = it.second->bweight = it.second->bpicsmall = it.second->bpicce2 = it.second->bpicpre1 = true;
                            }
                            else
                            {
                                /*bool bfindzz = false;
                                if(1)
                                {
                                    char sFilename[256] = {};
                                    GetPrivateProfileString("CAL", "file", "\\\\192.168.1.250\\File\\zzcal.ini", sFilename, 256, pThis->strConfigfile.c_str());

                                    char sVal[256] = {};
                                    GetPrivateProfileString("sys", "L", "3.5", sVal, 29, sFilename);
                                    float fwc = atof(sVal);
                                    OutputLog("%s  %.1f", sFilename,fwc);
                                    srand(unsigned(time(0)));
        //                            if (it.second->axiscount==6)
                                    {
                                        GetPrivateProfileString("six", "no", "111BP005", sVal, 29, sFilename);
                                        if(strstr(it.second->strcarno, sVal) != NULL)
                                        {
                                            it.second->axiscount=6;
                                            GetPrivateProfileString("six", "all", "39999", sVal, 29, sFilename);
                                            int iall = atoi(sVal);
                                            int iUpper = iall*fwc/100.0;
                                            int iBottom = iall*fwc*(-1)/100.0;
                                            int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                            int iaver = irall/6;
                                            OutputLog("six %d %d %d %d",iall,iUpper,iBottom,irall);
                                            GetPrivateProfileString("six", "z1", "5861", sVal, 29, sFilename);
                                            int z1 = atoi(sVal);
                                            GetPrivateProfileString("six", "z2", "6880", sVal, 29, sFilename);
                                            int z2 = atoi(sVal);
                                            GetPrivateProfileString("six", "z3", "6814", sVal, 29, sFilename);
                                            int z3 = atoi(sVal);
                                            GetPrivateProfileString("six", "z4", "7535", sVal, 29, sFilename);
                                            int z4 = atoi(sVal);
                                            GetPrivateProfileString("six", "z5", "7397", sVal, 29, sFilename);
                                            int z5 = atoi(sVal);
                                            GetPrivateProfileString("six", "z6", "6655", sVal, 29, sFilename);
                                            int z6 = atoi(sVal);
                                            it.second->Axis_weight[0] = z1+iaver;
                                            it.second->Axis_weight[1] = z2+iaver;
                                            it.second->Axis_weight[2] = z3+iaver;
                                            it.second->Axis_weight[3] = z4+iaver;
                                            it.second->Axis_weight[4] = z5+iaver;
                                            it.second->Axis_weight[5] = z6+iaver;
                                            it.second->Axis_weight[6] = 0;
                                            it.second->Axis_weight[7] = 0;
                                            it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]
                                                    +it.second->Axis_weight[3]+it.second->Axis_weight[4]+it.second->Axis_weight[5];
                                            bfindzz = true;
                                        }
                                    }
        //                            if (it.second->axiscount==6)
                                    {
                                        GetPrivateProfileString("six2", "no", "111BP005", sVal, 29, sFilename);
                                        if(strstr(it.second->strcarno, sVal) != NULL)
                                        {
                                            it.second->axiscount=6;
                                            GetPrivateProfileString("six2", "all", "39999", sVal, 29, sFilename);
                                            int iall = atoi(sVal);
                                            int iUpper = iall*fwc/100.0;
                                            int iBottom = iall*fwc*(-1)/100.0;
                                            int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                            int iaver = irall/6;
                                            OutputLog("six2 %d %d %d %d",iall,iUpper,iBottom,irall);
                                            GetPrivateProfileString("six2", "z1", "5861", sVal, 29, sFilename);
                                            int z1 = atoi(sVal);
                                            GetPrivateProfileString("six2", "z2", "6880", sVal, 29, sFilename);
                                            int z2 = atoi(sVal);
                                            GetPrivateProfileString("six2", "z3", "6814", sVal, 29, sFilename);
                                            int z3 = atoi(sVal);
                                            GetPrivateProfileString("six2", "z4", "7535", sVal, 29, sFilename);
                                            int z4 = atoi(sVal);
                                            GetPrivateProfileString("six2", "z5", "7397", sVal, 29, sFilename);
                                            int z5 = atoi(sVal);
                                            GetPrivateProfileString("six2", "z6", "6655", sVal, 29, sFilename);
                                            int z6 = atoi(sVal);
                                            it.second->Axis_weight[0] = z1+iaver;
                                            it.second->Axis_weight[1] = z2+iaver;
                                            it.second->Axis_weight[2] = z3+iaver;
                                            it.second->Axis_weight[3] = z4+iaver;
                                            it.second->Axis_weight[4] = z5+iaver;
                                            it.second->Axis_weight[5] = z6+iaver;
                                            it.second->Axis_weight[6] = 0;
                                            it.second->Axis_weight[7] = 0;
                                            it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]
                                                    +it.second->Axis_weight[3]+it.second->Axis_weight[4]+it.second->Axis_weight[5];
                                            bfindzz = true;
                                        }
                                    }
        //                            if (it.second->axiscount==4)
                                    {
                                        GetPrivateProfileString("four", "no", "2227886W", sVal, 29, sFilename);
                                        if(strstr(it.second->strcarno, sVal) != NULL)
                                        {
                                            it.second->axiscount=4;
                                            GetPrivateProfileString("four", "all", "39999", sVal, 29, sFilename);
                                            int iall = atoi(sVal);
                                            int iUpper = iall*fwc/100.0;
                                            int iBottom = iall*fwc*(-1)/100.0;
                                            int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                            int iaver = irall/4;
                                            OutputLog("four %d %d %d %d",iall,iUpper,iBottom,irall);

                                            GetPrivateProfileString("four", "z1", "4978", sVal, 29, sFilename);
                                            int z1 = atoi(sVal);
                                            GetPrivateProfileString("four", "z2", "5530", sVal, 29, sFilename);
                                            int z2 = atoi(sVal);
                                            GetPrivateProfileString("four", "z3", "9213", sVal, 29, sFilename);
                                            int z3 = atoi(sVal);
                                            GetPrivateProfileString("four", "z4", "9553", sVal, 29, sFilename);
                                            int z4 = atoi(sVal);
                                            it.second->Axis_weight[0] = z1+iaver;
                                            it.second->Axis_weight[1] = z2+iaver;
                                            it.second->Axis_weight[2] = z3+iaver;
                                            it.second->Axis_weight[3] = z4+iaver;
                                            it.second->Axis_weight[4] = 0;
                                            it.second->Axis_weight[5] = 0;
                                            it.second->Axis_weight[6] = 0;
                                            it.second->Axis_weight[7] = 0;
                                            it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]
                                                    +it.second->Axis_weight[3];
                                            bfindzz = true;
                                        }
                                    }
                                    //if (it.second->axiscount==3)
                                    {
                                        GetPrivateProfileString("three", "no", "2227886W", sVal, 29, sFilename);
                                        if(strstr(it.second->strcarno, sVal) != NULL)
                                        {
                                            it.second->axiscount=3;
                                            GetPrivateProfileString("three", "all", "39999", sVal, 29, sFilename);
                                            int iall = atoi(sVal);
                                            int iUpper = iall*fwc/100.0;
                                            int iBottom = iall*fwc*(-1)/100.0;
                                            int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                            int iaver = irall/3;
                                            OutputLog("three %d %d %d %d",iall,iUpper,iBottom,irall);

                                            GetPrivateProfileString("three", "z1", "4978", sVal, 29, sFilename);
                                            int z1 = atoi(sVal);
                                            GetPrivateProfileString("three", "z2", "5530", sVal, 29, sFilename);
                                            int z2 = atoi(sVal);
                                            GetPrivateProfileString("three", "z3", "9213", sVal, 29, sFilename);
                                            int z3 = atoi(sVal);
                                            it.second->Axis_weight[0] = z1+iaver;
                                            it.second->Axis_weight[1] = z2+iaver;
                                            it.second->Axis_weight[2] = z3+iaver;
                                            it.second->Axis_weight[3] = 0;
                                            it.second->Axis_weight[4] = 0;
                                            it.second->Axis_weight[5] = 0;
                                            it.second->Axis_weight[6] = 0;
                                            it.second->Axis_weight[7] = 0;
                                            it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2];
                                            bfindzz = true;
                                        }
                                    }
//                                    if (it.second->axiscount==2)
                                    {
                                        GetPrivateProfileString("two", "no", "333V1079", sVal, 29, sFilename);
                                        if(strstr(it.second->strcarno, sVal) != NULL)
                                        {
                                            it.second->axiscount=2;
                                            GetPrivateProfileString("two", "all", "39999", sVal, 29, sFilename);
                                            int iall = atoi(sVal);
                                            int iUpper = iall*fwc/100.0;
                                            int iBottom = iall*fwc*(-1)/100.0;
                                            int irall = (rand() % (iUpper - iBottom + 1) + iBottom);
                                            int iaver = irall/2;
                                            OutputLog("two %d %d %d %d",iall,iUpper,iBottom,irall);

                                            GetPrivateProfileString("two", "z1", "3970", sVal, 29, sFilename);
                                            int z1 = atoi(sVal);
                                            GetPrivateProfileString("two", "z2", "9535", sVal, 29, sFilename);
                                            int z2 = atoi(sVal);
                                            it.second->Axis_weight[0] = z1+iaver;
                                            it.second->Axis_weight[1] = z2+iaver;
                                            it.second->Axis_weight[2] = 0;
                                            it.second->Axis_weight[3] = 0;
                                            it.second->Axis_weight[4] = 0;
                                            it.second->Axis_weight[5] = 0;
                                            it.second->Axis_weight[6] = 0;
                                            it.second->Axis_weight[7] = 0;
                                            it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1];
                                            bfindzz = true;
                                        }
                                    }
                                    GetPrivateProfileString("sys", "SP", "40", sVal, 29, sFilename);
                                    it.second->speed = atoi(sVal) + (::rand()%5);
                                    it.second->Axis_weight[0] = it.second->Axis_weight[0]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->Axis_weight[1] = it.second->Axis_weight[1]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->Axis_weight[2] = it.second->Axis_weight[2]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->Axis_weight[3] = it.second->Axis_weight[3]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->Axis_weight[4] = it.second->Axis_weight[4]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->Axis_weight[5] = it.second->Axis_weight[5]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->Axis_weight[6] = it.second->Axis_weight[6]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->Axis_weight[7] = it.second->Axis_weight[7]/pThis->m_weidu*pThis->m_weidu;
                                    it.second->weight = it.second->Axis_weight[0]+it.second->Axis_weight[1]+it.second->Axis_weight[2]+it.second->Axis_weight[3]
                                            +it.second->Axis_weight[4]+it.second->Axis_weight[5]+it.second->Axis_weight[6]+it.second->Axis_weight[7];
                                }
                                if(bfindzz)
                                {
                                    pThis->OutputLog2("3s超时******bfindzz 前%d  后%d  W%d ce%d 车道%d  %s",
                                                      it.second->bpicpre, it.second->bpic,it.second->bweight,it.second->bpicce, it.second->laneno, it.second->strcarno);
                                    it.second->bpicce = it.second->bpic = it.second->bpicpre = it.second->bweight = it.second->bpicsmall = true;
                                }
                                else*/

                                {
                                    pThis->m_QueueSect.Lock();
                                    pThis->OutputLog2("3s超时无效 carno:%s,前%d  后%d  W%d ce%d 车道%d  %s",it.second->strcarno,
                                                      it.second->bpicpre, it.second->bpic,it.second->bweight,it.second->bpicce, it.second->laneno, it.second->strcarno);
                                    DropFile(it.second->strprepic);
                                    DropFile(it.second->strafrpic);
                                    DropFile(it.second->strsmallpic);
                                    DropFile(it.second->strcepic);
                                    memset(it.second, 0, sizeof(STR_MSG));
                                    pThis->m_QueueSect.Unlock();
                                }
                            }
                        }
                    }
                    pThis->m_QueueSect.Unlock();
                }
            }
        }
    }
    catch (...)
    {
        pThis->OutputLog2("数据处理异常");
    }

    return 0;
}

void* VedioMsgThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    STR_MSG nPmsg;
    while (pThis->bStart)
    {
        Sleep(20);
        try
        {
            if (!pThis->m_MsgVedioQueue.empty())
            {
                //出队
                pThis->m_QueueSectVedio.Lock();
                if (::GetTickCount() - pThis->m_MsgVedioQueue.front().getticout > 150*1000)
                {
                    pThis->OutputLog2("超3min,截取录像 %s,记录id %d, real size=%d", pThis->m_MsgVedioQueue.front().strcarno,
                                      pThis->m_MsgVedioQueue.front().dataId, pThis->m_MsgVedioQueue.size());
                    nPmsg = pThis->m_MsgVedioQueue.front();
                    pThis->m_MsgVedioQueue.pop();
                    pThis->m_QueueSectVedio.Unlock();
                    ////////////////////////////////
                    //队列不为空
                    STR_MSG *pMsg = NULL;
                    if (!g_msgQueue.empty())
                    {
                        pthread_mutex_lock(&queue_cs);
                        pMsg = g_msgQueue.front();
                        memcpy(pMsg, &nPmsg, sizeof(STR_MSG));
                        g_msgQueue.pop();
                        pthread_mutex_unlock(&queue_cs);
                        pool_add_job(process_request2,(void*)pMsg);
                    }
                    else
                    {
                        pMsg = new STR_MSG;
                        if(pMsg)
                        {
                            OutputLog("队列已用完，二次new OK");
                            memcpy(pMsg, &nPmsg, sizeof(STR_MSG));
                            pool_add_job(process_request,(void*)pMsg);
                        }
                        else
                            OutputLog("队列已用完，二次new 失败");
                    }
                    ////////////////////////////////
                }
                else
                {
                    pThis->m_QueueSectVedio.Unlock();
                    Sleep(10);
                    continue;
                }
            }
        }
        catch (...)
        {
            pThis->OutputLog2("bUploadvedio更新数据处理异常");
        }
    }

    return 0;
}


void* HisVedioMsgThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    //崩溃前的待下载录像
    QString strQuery = "SELECT * FROM Task WHERE bUploadvedio = 0 order by id desc;";
    QSqlQuery query(g_DB);
    query.prepare(strQuery);
    if (!query.exec())
    {
        QString querys = query.lastError().text();
        OutputLog("re车辆待录像记录查询失败 %s",querys.toStdString().c_str());
    }
    else
    {
        int iCount = 0;
        if (query.last())
        {
            iCount = query.at() + 1;
        }
        query.first();//重新定位指针到结果集首位
        query.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
        {
            int iii=0;
            while (query.next())
            {
                STR_MSG nPmsg;
                nPmsg.dataId = query.value("id").toString().toInt();
                nPmsg.laneno = query.value("laneno").toString().toInt();
                strcpy(nPmsg.strcarno,query.value("CarNO").toString().toLocal8Bit());
                QString strTime = query.value("DetectionTime").toString();
                strTime.replace(QString("T"), QString(" "));
                strcpy(nPmsg.strtime,(const char*)strTime.toLocal8Bit());
                strcpy(nPmsg.strcartype,query.value("CarType").toString().toLocal8Bit());
                strcpy(nPmsg.strret,query.value("DetectionResult").toString().toLocal8Bit());
                nPmsg.speed = query.value("Speed").toString().toInt();
                nPmsg.axiscount = query.value("Axles").toString().toInt();
                nPmsg.weight = query.value("Weight").toString().toInt();
                nPmsg.getticout = ::GetTickCount();
                //录像
                if(pThis->m_VideoSave == 1)
                {
                    if(strstr(nPmsg.strret,"不") == NULL && pThis->m_OkNoVideo==1)
                    {
                        //合格且设置了不录像
                        pThis->OutputLog2("合格不录像 %s", nPmsg.strcarno);
                    }
                    else
                    {
                        pThis->m_QueueSectVedioHis.Lock();
                        pThis->m_MsgVedioQueueHis.push(nPmsg);
                        pThis->m_QueueSectVedioHis.Unlock();
                        iii++;
                        OutputLog("启动录像111 His size=%d", pThis->m_MsgVedioQueueHis.size());
                        if(iii>50)
                            break;
                    }
                }
            }
        }
    }

    STR_MSG nPmsg;
    while (pThis->bStart)
    {
        Sleep(20);
        try
        {
            if (!pThis->m_MsgVedioQueueHis.empty())
            {
                //出队
                pThis->m_QueueSectVedioHis.Lock();
                if (::GetTickCount() - pThis->m_MsgVedioQueueHis.front().getticout > 150*1000)
                {
                    pThis->OutputLog2("超3min,截取his录像 %s,记录id %d,his size=%d", pThis->m_MsgVedioQueueHis.front().strcarno,
                                      pThis->m_MsgVedioQueueHis.front().dataId, pThis->m_MsgVedioQueueHis.size());
                    nPmsg = pThis->m_MsgVedioQueueHis.front();
                    pThis->m_MsgVedioQueueHis.pop();
                    pThis->m_QueueSectVedioHis.Unlock();
                    ////////////////////////////////
                    //队列不为空
                    STR_MSG *pMsg = NULL;
                    if (!g_msgQueue.empty())
                    {
                        pthread_mutex_lock(&queue_cs);
                        pMsg = g_msgQueue.front();
                        memcpy(pMsg, &nPmsg, sizeof(STR_MSG));
                        g_msgQueue.pop();
                        pthread_mutex_unlock(&queue_cs);
                        pool_add_job(process_request2,(void*)pMsg);
                    }
                    else
                    {
                        pMsg = new STR_MSG;
                        if(pMsg)
                        {
                            OutputLog("队列已用完，二次new OK");
                            memcpy(pMsg, &nPmsg, sizeof(STR_MSG));
                            pool_add_job(process_request,(void*)pMsg);
                        }
                        else
                            OutputLog("队列已用完，二次new 失败");
                    }
                    ////////////////////////////////
                }
                else
                {
                    pThis->m_QueueSectVedioHis.Unlock();
                    Sleep(10);
                    continue;
                }

            }
        }
        catch (...)
        {
            pThis->OutputLog2("bUploadvedio更新数据处理异常");
        }
    }

    return 0;
}

void OverWidget::MSesGCallback(int Lane, char *filename, int pictype, char *carno, char *pcolor, char *cartype)
{
    OverWidget * m_Socket = (OverWidget *)this;
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString strTemp = current_date_time.toString("yyyy-MM-dd hh:mm:ss");



    m_Socket->m_QueueSect.Lock();
    bool isDuplicate = false;
    QByteArray byteArrayqian1;
    QByteArray byteArrayhou;
    QByteArray byteArrayce;
    QByteArray byteArrayhce;
    QByteArray byteArraysmall;
    QQueue<QByteArray>::const_iterator iter1;
    auto ifind = m_Socket->m_maplaneAndmsg.find(Lane);
    if (ifind != m_Socket->m_maplaneAndmsg.end())
    {
        char* dd = strstr(filename,".jpg");
        STR_MSG *msg = ifind->second;
        //enum PicMode{ SMALL, FRONT, AFTER, SIDE,SIDE2, ALL,};
        switch (pictype) {
        case 0:
            strcpy(msg->strsmallpic, filename);
            msg->bpicsmall=true;
            strcpy(msg->strcarnos, carno);
            if(!strstr(msg->strsmallpic,"无")){
                byteArraysmall=QByteArray(msg->strsmallpic, sizeof(msg->strsmallpic));
                iter1 = smallQueue.constBegin();
                while (iter1 != smallQueue.constEnd()) {
                    if (strcmp(iter1->data(),msg->strsmallpic)==0) {
                        isDuplicate = true;
                        break;
                    }
                    ++iter1;
                }
                OutputLog("--------%d------small",isDuplicate);
                if(!isDuplicate){
                    smallQueue.enqueue(byteArraysmall);
                    OutputLog("***add*** small :%s",byteArraysmall.data());
                }
                isDuplicate=false;
            }
            break;
        case 1:
            if(*(dd-1)=='0')
            {

                strcpy(msg->strcarno, carno);
                strcpy(msg->strplatecolor, pcolor);
                strcpy(msg->strtime, strTemp.toStdString().c_str());
                msg->laneno = Lane;
                strcpy(msg->strprepic, filename);
                strcpy(msg->strcartype, cartype);
                msg->bpicpre = true;
                msg->getticout = ::GetTickCount();
                msg->bstart = true;
    //            msg->bweight = true; //
                break;
            }else{
                strcpy(msg->strcarno3, carno);
                strcpy(msg->strplatecolor, pcolor);
//                strcpy(msg->strtime, strTemp.toStdString().c_str());
                msg->laneno = Lane;
                strcpy(msg->strprepic1, filename);
                strcpy(msg->strcartype, cartype);
                msg->bpicpre1 = true;
                msg->getticout = ::GetTickCount();
                msg->bstart = true;
    //            msg->bweight = true;
                if(!strstr(msg->strprepic1,"无")){
                    byteArrayqian1=QByteArray(msg->strprepic1, sizeof(msg->strprepic1));
                    iter1 = fro1Queue.constBegin();
                    while (iter1 != fro1Queue.constEnd()) {
                        if (strcmp(iter1->data(),msg->strprepic1)==0) {
                            isDuplicate = true;
                            break;
                        }
                        ++iter1;
                    }
                    OutputLog("--------%d------qian1",isDuplicate);
                    if(!isDuplicate){
                        fro1Queue.enqueue(byteArrayqian1);
                        OutputLog("***add*** qian1 :%s",byteArrayqian1.data());
                    }
                    isDuplicate=false;
                }
                break;
            }
        case 2:
            strcpy(msg->strafrpic, filename);
            strcpy(msg->strtime, strTemp.toStdString().c_str());
            msg->bpic = true;
            msg->laneno = Lane;
            strcpy(msg->strcarno2, carno);
            strcpy(msg->strplatecolor, pcolor);
//            strcpy(msg->strcartype, cartype);
            msg->getticout = ::GetTickCount();
            msg->bstart = true;
            if(!strstr(msg->strafrpic,"无")){
                byteArrayhou=QByteArray(msg->strafrpic, sizeof(msg->strafrpic));
                iter1 = afrQueue.constBegin();
                while (iter1 != afrQueue.constEnd()) {
                    if (strcmp(iter1->data(),msg->strafrpic)==0) {
                        isDuplicate = true;
                        break;
                    }
                    ++iter1;
                }
                OutputLog("--------%d------hou",isDuplicate);
                if(!isDuplicate){
                    afrQueue.enqueue(byteArrayhou);
                    OutputLog("***add*** hou :%s",byteArrayhou.data());
                }
                isDuplicate=false;
            }
//            msg->bweight = true; //
            break;
        case 3:
            msg->bpicce = true;
            strcpy(msg->strcepic, filename);
            strcpy(msg->strcarnoce, carno);
            strcpy(msg->strplatecolor, pcolor);
            if(!strstr(msg->strcepic,"无")){
                byteArrayce=QByteArray(msg->strcepic, sizeof(msg->strcepic));
                iter1 = ceQueue.constBegin();
                while (iter1 != ceQueue.constEnd()) {
                    if (strcmp(iter1->data(),msg->strcepic)==0) {
                        isDuplicate = true;
                        break;
                    }
                    ++iter1;
                }
                OutputLog("--------%d------ce",isDuplicate);
                if(!isDuplicate){
                    ceQueue.enqueue(byteArrayce);
                    OutputLog("***add*** ce :%s",byteArrayce.data());
                }
                isDuplicate=false;
            }
            break;
        case 4:
            strcpy(msg->strcepic, filename);
            break;
        case 5:
            strcpy(msg->strallpic, filename);
            break;
        case 7:
            msg->bpicce2 = true;
            strcpy(msg->strcarnohce, carno);
            strcpy(msg->strhoucepic, filename);
            if(!strstr(msg->strcepic,"无")){
                byteArrayhce=QByteArray(msg->strhoucepic, sizeof(msg->strhoucepic));
                iter1 = hceQueue.constBegin();
                while (iter1 != hceQueue.constEnd()) {
                    if (strcmp(iter1->data(),msg->strhoucepic)==0) {
                        isDuplicate = true;
                        break;
                    }
                    ++iter1;
                }
                OutputLog("--------%d------hce",isDuplicate);
                if(!isDuplicate)
                {
                    OutputLog("***add*** hou ce :%s",byteArrayhce.data());
                    hceQueue.enqueue(byteArrayhce);
                }
                isDuplicate=false;
            }
            break;
        default:
            break;
        }
    }
    m_Socket->m_QueueSect.Unlock();
}

void OverWidget::onClose()
{
//    //队列清理  by mazq
//    pthread_mutex_lock(&queue_cs);
//    OutputLog("开始释放g_msgQueue");
//    for (;g_msgQueue.size()!=0;)
//    {
//        free(g_msgQueue.front());
//        g_msgQueue.pop();
//    }
//    pthread_mutex_unlock(&queue_cs);
//    OutputLog("释放后队列长度%d", g_msgQueue.size());

//    pthread_mutex_destroy(&queue_cs);
//    //线程池清理  mazq
//    pool_destroy();

    m_LedCon.Close();
    m_ProCon.Clean();
    this->close();
}

void OverWidget::OutputLog2(const char *msg, ...)
{
    va_list vl_list;
    va_start(vl_list, msg);
    char content[65535] = { 0 };
    vsprintf(content, msg, vl_list);   //格式化处理msg到字符串
    va_end(vl_list);
    OutputLog(content);
}

WorkThread::WorkThread(OverWidget *gui)
{
    m_gui = gui;
    connect(this, SIGNAL(onShowUI()), m_gui, SLOT(slotUpdateUi()));
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

///////////////////////////以下为无标题拖动////////////////////////////////
//重写鼠标按下事件
void OverWidget::mousePressEvent(QMouseEvent *event)
{
    mMoveing = true;
    //记录下鼠标相对于窗口的位置
    //event->globalPos()鼠标按下时，鼠标相对于整个屏幕位置
    //pos() this->pos()鼠标按下时，窗口相对于整个屏幕位置
    mMovePosition = event->globalPos() - pos();
}

//重写鼠标移动事件
void OverWidget::mouseMoveEvent(QMouseEvent *event)
{
    //(event->buttons() && Qt::LeftButton)按下是左键
    //鼠标移动事件需要移动窗口，窗口移动到哪里呢？就是要获取鼠标移动中，窗口在整个屏幕的坐标，然后move到这个坐标，怎么获取坐标？
    //通过事件event->globalPos()知道鼠标坐标，鼠标坐标减去鼠标相对于窗口位置，就是窗口在整个屏幕的坐标
    if (mMoveing && (event->buttons() && Qt::LeftButton)
        && (event->globalPos() - mMovePosition).manhattanLength() > QApplication::startDragDistance())
    {
        move(event->globalPos() - mMovePosition);
        mMovePosition = event->globalPos() - pos();
    }
}

void OverWidget::mouseReleaseEvent(QMouseEvent *event)
{
    mMoveing = false;
}
