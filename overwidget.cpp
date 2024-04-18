﻿#include "overwidget.h"
#include "ui_overwidget.h"
#include <QMetaType>
/******************线程池******************/
void * routine(void * arg);
int pool_add_job(void *(*process)(void * arg),void *arg);
int pool_init(unsigned int thread_num);
int pool_destroy(void);

/******************结构体******************/



/******************全局变量******************/
static Thread_pool * pool=NULL;
OverWidget *g_Over;
static pthread_mutex_t queue_cs;/*互斥量*/
typedef std::queue<STR_MSG*>  MsgQueue;  //by mazq
MsgQueue g_msgQueue;//todo 视频下载

/******************普通函数******************/
//线程池-函数
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
    for(i=0;i<(int)thread_num;i++)
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
/* Add job into the pool assign it to some thread*/
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
    for(i=0;i<(int)pool->thread_num;i++)
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
    Q_UNUSED(arg);
//    OutputLog("start thread: %u",pthread_self());
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
void DropFile(const char* szfilename)
{
    if(DeleteFileA(szfilename)==0)
        OutputLog("%s delete error %d",szfilename, GetLastError());
}
//删除指定文件夹和文件夹下的内容，删除失败则返回false 即使某个文件删除失败，也会继续将本文件夹中其他文件删除
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
//比较两个文件夹名，返回时间早的那一个 文件夹名格式：YYYY-MM-DD，2012-10-03，长度必须是10，否则抛出异常
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
//输入一个保存路径，返回时间最早的文件夹名 格式：E:\或D:\asdf
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


//动态库-接口
//外廓
bool g_ProfileRet(int L,int W,int H,int lane,long etc, int etc2)
{
    g_Over->AnalysisProfileData(L,W,H,lane,etc,etc2);
    return 0;
}
void g_ProfileStateRet(int Status)//todo database
{
    g_Over->OutputLog2("外廓状态.......%d",Status);
    /////////////////////////////////////////////////////////////
    QString scal = Status==0?QStringLiteral("断线"):QStringLiteral("重连成功");
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString strdate = current_date_time.toString("yyyy-MM-dd hh:mm:ss"); //设置显示格式
    //保存数据
    QSqlQuery qsQuery = QSqlQuery(g_Over->m_DbCon->getDatabase());
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
    //    if(Lane == 4)
    //        Lane = 1;
    //    else if(Lane == 1)
    //        Lane = 4;
    if(Type == 1){
        g_Over->OutputLog2("%d******到达******",Lane);
    }
    else{
        g_Over->OutputLog2("%d******离开******",Lane);
        g_Over->m_DevCon.CarArrive(Lane,true);
    }
}
//相机
void g_CarCamShoot(int Lane, char *filename, int pictype, char *carno, char *pcolor, char *cartype)
{
    g_Over->MSesGCallback(Lane,filename,pictype,carno,pcolor,cartype);
}
//称重
void g_CarWeight(const char* msg, int iUser, void *pUser)
{
    Q_UNUSED(iUser);
    Q_UNUSED(pUser);
    g_Over->AnalysisCarResult(msg);
}

//线程
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
            if ((int)::GetTickCount() - pThis->m_LastTic > pThis->m_TimeFlush)
            {
                pThis->OutputLog2("屏幕复位...");
                //                pThis->ResetLed();
                pThis->m_bNeedFlush = false;
            }
        }
    }

    return 0;
}
void* CheckStoreThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);

    char strPath[256] = {};
    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, pThis->strConfigfile.c_str());
    QString sDir = strPath;
    sDir=sDir.left(1);

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
void* SendMsgThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    pThis->OutputLog2("轮询线程开启...");

    char  szVal[200];
    GetPrivateProfileString("SYS", "IsCam", "1", szVal, 29, pThis->strConfigfile.c_str());
    atoi(szVal);
    //超时车辆是否保存
    GetPrivateProfileString("THROW", "NoWeight", "0", szVal, 29, pThis->strConfigfile.c_str());
    bool noWeightThrow = atoi(szVal);
    GetPrivateProfileString("THROW", "NoPic", "0", szVal, 29, pThis->strConfigfile.c_str());
    bool noPicThrow = atoi(szVal);
    Sleep(5000);

    try
    {
        while (pThis->bStart)
        {
            Sleep(50);/*
            pThis->m_CarResult.checkMsgState(pThis->m_Timeout);
            //enum PicType{ SMALL, FRONT, AFTER, SIDE,SIDE2,AFRSIDE, ALL,};
            char path[] = "c:/work/okok";
            char carno[] = "carno";
            char pcolor[] = "pcolor";
            char cartype[] = "cartype";
            pThis->MSesGCallback(2,path,AFRSIDE,carno,pcolor,cartype);
            pThis->MSesGCallback(2,path,AFTER,carno,pcolor,cartype);
            pThis->MSesGCallback(2,path,SMALL,carno,pcolor,cartype);
            pThis->MSesGCallback(2,path,FRONT,carno,pcolor,cartype);
            pThis->MSesGCallback(2,path,SIDE,carno,pcolor,cartype);
            char rest[] = "{\"1\":234,\"Axis_space2\":0,\"Axis_space3\":0,\"Axis_space4\":0,\"Axis_space5\":0,\"Axis_space6\":0,\"Axis_space7\":0,\"Axis_weight1\":664,\"Axis_weight2\":514,\"Axis_weight3\":0,\"Axis_weight4\":0,\"Axis_weight5\":0,\"Axis_weight6\":0,\"Axis_weight7\":0,\"Axis_weight8\":0,\"axiscount\":2,\"carlen\":0,\"cartype\":0,\"cmd\":\"data\",\"crossroad\":0,\"dataId\":760,\"datatime\":\"2024-04-16 08:59:49\",\"deviceNo\":\"110\",\"direction\":0,\"laneno\":2,\"speed\":56,\"weight\":1178}";
            pThis->AnalysisCarResult(rest);*/
            //检查列表里有没有准备好的数据
            STR_MSG msg = pThis->m_CarResult.checkMsgState(pThis->m_Timeout);
            if(msg.bstart > 1)//完成或者超时
            {
                pThis->optimize(msg);//优化数据
                if((noWeightThrow && msg.bstart==3) || (noPicThrow && msg.bstart==4))//超时车丢弃
                {
                    pThis->OutputLog2("**Time out**  throw laneno:%d carno:%s pic:%d weight:%d", msg.laneno, msg.strcarno,msg.picReady,msg.bweight);
                    //                        pThis->m_CarResult.deleteMsg(&msg);
                    break;
                }

                pThis->m_QueueSect.Lock();
                int iRet = pThis->analyzeData(msg);//根据规则分析数据判断车辆是否合格

                //保存数据
                QJsonObject jsonData;
                jsonData["DetectionTime"] = msg.strtime;
                jsonData["CarNO"] = QString::fromLocal8Bit(msg.strcarno);
                jsonData["CarType"] = QString::fromLocal8Bit(msg.strcartype);
                jsonData["deviceNo"] = QString::fromLocal8Bit(msg.deviceNo);
                jsonData["PlateColor"] = QString::fromLocal8Bit(msg.strplatecolor);
                jsonData["DetectionResult"] = QString::fromLocal8Bit(msg.strret);

                jsonData["ImageBaseFront"] = QString::fromLocal8Bit(msg.carpics[FRONT]);
                jsonData["ImageBaseBack"] = QString::fromLocal8Bit(msg.carpics[AFTER]);
                jsonData["vedioBaseSide"] = QString::fromLocal8Bit(msg.carpics[AFRSIDE]);
                jsonData["ImageBaseSide"] = QString::fromLocal8Bit(msg.carpics[SIDE]);
                jsonData["ImageBasePanorama"] = QString::fromLocal8Bit(msg.carpics[FRONT1]);
                jsonData["ImageBaseCarNO"] = QString::fromLocal8Bit(msg.carpics[SMALL]);

                jsonData["overWeight"] = QString::number(msg.overWeight);
                jsonData["direction"] = QString::fromLocal8Bit(msg.strdirection);
                jsonData["crossroad"] = QString::fromLocal8Bit(msg.strcrossroad);
                jsonData["reason"] = QString::fromLocal8Bit(msg.strreason);
                jsonData["AxlesWeight1"] = QString::number(msg.Axis_weight[0]);
                jsonData["AxlesWeight2"] = QString::number(msg.Axis_weight[1]);
                jsonData["AxlesWeight3"] = QString::number(msg.Axis_weight[2]);
                jsonData["AxlesWeight4"] = QString::number(msg.Axis_weight[3]);
                jsonData["AxlesWeight5"] = QString::number(msg.Axis_weight[4]);
                jsonData["AxlesWeight6"] = QString::number(msg.Axis_weight[5]);
                jsonData["AxlesWeight7"] = QString::number(msg.Axis_weight[6]);
                jsonData["AxlesWeight8"] = QString::number(msg.Axis_weight[7]);
                jsonData["AxlesSpace1"] = QString::number(msg.Axis_space[0]);
                jsonData["AxlesSpace2"] = QString::number(msg.Axis_space[1]);
                jsonData["AxlesSpace3"] = QString::number(msg.Axis_space[2]);
                jsonData["AxlesSpace4"] = QString::number(msg.Axis_space[3]);
                jsonData["AxlesSpace5"] = QString::number(msg.Axis_space[4]);
                jsonData["AxlesSpace6"] = QString::number(msg.Axis_space[5]);
                jsonData["AxlesSpace7"] = QString::number(msg.Axis_space[6]);
                jsonData["bUploadpic"] = QString::number(iRet);
                jsonData["bUploadvedio"] = QString::number(iRet);
                jsonData["laneno"] = QString::number(msg.laneno);
                jsonData["Axles"] = QString::number(msg.axiscount);
                jsonData["carLength"] = QString::number(msg.carlen);
                jsonData["Speed"] = QString::number(msg.speed);
                jsonData["Weight"] = QString::number(msg.weight);
                jsonData["overPercent"] = QString::number(msg.overPercent,'f',2);
                jsonData["XZ"] = QString::number(msg.GBxz);
                jsonData["w"] = QString::number(msg.w);
                jsonData["h"] = QString::number(msg.h);
                jsonData["l"] = QString::number(msg.l);

                bool insertRes = pThis->m_DbCon->insertData("TASK",jsonData);
                if(!insertRes)
                    OutputLog("----sendMsg: insert sql %d",insertRes);
                int nFID = pThis->m_DbCon->findMaxID();
                pThis->OutputLog2("数据准备完成 id:%d laneno:%d carno:%s pic:%d weight:%d", nFID, msg.laneno, msg.strcarno,msg.picReady,msg.bweight);

                msg.dataId = nFID;
                //实时界面
                if(msg.bstart > 1){
                    OutputLog("-------- update ui");
                    //                    pThis->UpdateUI(msg);//todo 为什么信号发送了没反应
                    pThis->m_realwid->updateRealMsg(msg);
                }
                //大屏
                if(strstr(msg.strret,"不") == NULL && pThis->m_OkNoShow==1)
                {
                    //合格且设置了不上屏
                }
                else
                {
                    //                    if(msg.overWeight > 0)
                    if(msg.bstart > 1)
                    {
                        pThis->OutputLog2("========update led==========");
                        //                        pThis->UpdateLed(&msg);
                        char rule[256] = {0};
                        GetPrivateProfileString("LED", "page", "1", rule, 265, pThis->strConfigfile.c_str());
                        if(strlen(rule)<4)//说明没有数据库,实时获取
                            emit pThis->LedUpdate(1,msg);
                    }
                }
                //录像
                if(pThis->m_VideoSave == 1)
                {
                    if(strstr(msg.strret,"不") == NULL && pThis->m_OkNoVideo==1)
                    {
                        //合格且设置了不录像
                        pThis->OutputLog2("合格不录像 %s", msg.strcarno);
                    }
                    else
                    {
                        if(iRet == 0)  //7  5的都不处理了
                        {
                            pThis->m_QueueSectVedio.Lock();
                            pThis->m_MsgVedioQueue.push(msg);
                            pThis->m_QueueSectVedio.Unlock();
                            pThis->OutputLog2("启动录像 %s, real size=%d", msg.strcarno, pThis->m_MsgVedioQueue.size());
                        }
                    }
                }

                //下个过车
                //                        memset(msg, 0, sizeof(STR_MSG));
                //                pThis->m_CarResult.deleteMsg(msg);
                pThis->OutputLog2("================================================");
                pThis->m_QueueSect.Unlock();
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
    QVector<STR_MSG> msglist;
    if(pThis->m_DbCon->queryNoVedio(msglist))
    {
        //录像queryNoVedio
        if(pThis->m_VideoSave == 1)
        {
            for(int i = 0;i< msglist.size();i++)
            {
                if(strstr(msglist[i].strret,"不") == NULL && pThis->m_OkNoVideo==1)
                {
                    //合格且设置了不录像
//                    pThis->OutputLog2("合格不录像 %s", msglist[i].strcarno);
                }
                else
                {
                    pThis->m_QueueSectVedioHis.Lock();
                    pThis->m_MsgVedioQueueHis.push(msglist[i]);
                    pThis->m_QueueSectVedioHis.Unlock();
                    OutputLog("启动录像111 His size=%d", pThis->m_MsgVedioQueueHis.size());
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

/******************extra******************/

/******************类OverWidget******************/
OverWidget::OverWidget(QWidget *parent):QWidget(parent),ui(new Ui::OverWidget)
{
//    qRegisterMetaType<RealTimeWindow>("RealTimeWindow");
//    qRegisterMetaType<RealTimeWindow>("RealTimeWindow&");
    ui->setupUi(this);
    g_Over = this;
    m_LaneNum=1;
    m_showError = true;
    m_bNeedFlush = false;
    m_GBXZ = 0;
    m_PrePort  = m_AfrPort = -1;
    std::string sPath = QCoreApplication::applicationDirPath().toStdString();
    strConfigfile = sPath + "/config.ini";
    char szVal[100];
    {
        GetPrivateProfileString("DB", "数据库类型", "1", szVal, 100, strConfigfile.c_str());
        int type = atoi(szVal);
        GetPrivateProfileString("DB", "servername", "127.0.0.1", szVal, 100, strConfigfile.c_str());
        QString servername = QString::fromLocal8Bit(szVal);
        GetPrivateProfileString("DB", "DBname", "QODBC", szVal, 100, strConfigfile.c_str());
        QString DBname = QString::fromLocal8Bit(szVal);
        GetPrivateProfileString("DB", "ID", "sava", szVal, 100, strConfigfile.c_str());
        QString id = QString::fromLocal8Bit(szVal);
        GetPrivateProfileString("DB", "Password", "123456", szVal, 100, strConfigfile.c_str());
        QString pass = QString::fromLocal8Bit(szVal);
        m_DbCon= new CWTDBControl(type,servername,DBname,id,pass);
    }
    this->InitOver();
    bStart = true;
    m_picPool=new PicPoll(6,60);
}
OverWidget::~OverWidget()
{
    bStart = false;
    Sleep(200);

    delete ui;
}
//事件
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
        m_LedCon->ResetLed();
    }
}
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
    Q_UNUSED(event);
    mMoveing = false;
}

//槽函数
void OverWidget::slot_timeout_timer_pointer()
{
    char szVal[100];
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
//    m_realwid->moveToThread(QThread(m_dwHeartID2));
    StartLED();
}
int OverWidget::AnalysisProfileData(int l, int w, int h, int lane, long etc, int etc2)
{
    Q_UNUSED(etc);
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
        //出队
        m_QueueSect.Lock();
        STR_MSG IncompleteMsg = {0};
        int laneno = root_Obj["laneno"].toInt();
        int cruntTime = ::GetTickCount();
        IncompleteMsg.laneno = root_Obj["laneno"].toInt();
        IncompleteMsg.axiscount = root_Obj["axiscount"].toInt();
        IncompleteMsg.carlen = root_Obj["carlen"].toInt();
        IncompleteMsg.speed = root_Obj["speed"].toInt();
        IncompleteMsg.weight = root_Obj["weight"].toInt();
        IncompleteMsg.Axis_weight[0] = root_Obj["Axis_weight1"].toInt();
        IncompleteMsg.Axis_weight[1] = root_Obj["Axis_weight2"].toInt();
        IncompleteMsg.Axis_weight[2] = root_Obj["Axis_weight3"].toInt();
        IncompleteMsg.Axis_weight[3] = root_Obj["Axis_weight4"].toInt();
        IncompleteMsg.Axis_weight[4] = root_Obj["Axis_weight5"].toInt();
        IncompleteMsg.Axis_weight[5] = root_Obj["Axis_weight6"].toInt();
        IncompleteMsg.Axis_weight[6] = root_Obj["Axis_weight7"].toInt();
        IncompleteMsg.Axis_weight[7] = root_Obj["Axis_weight8"].toInt();
        IncompleteMsg.Axis_space[0] = root_Obj["Axis_space1"].toDouble();
        IncompleteMsg.Axis_space[1] = root_Obj["Axis_space2"].toDouble();
        IncompleteMsg.Axis_space[2] = root_Obj["Axis_space3"].toDouble();
        IncompleteMsg.Axis_space[3] = root_Obj["Axis_space4"].toDouble();
        IncompleteMsg.Axis_space[4] = root_Obj["Axis_space5"].toDouble();
        IncompleteMsg.Axis_space[5] = root_Obj["Axis_space6"].toDouble();
        IncompleteMsg.Axis_space[6] = root_Obj["Axis_space7"].toDouble();
        strcpy(IncompleteMsg.deviceNo, root_Obj["deviceNo"].toString().toStdString().c_str());

        int iType = root_Obj["direction"].toInt();
        if (iType == 0)
            strcpy(IncompleteMsg.strdirection, "正向");
        else if (iType == 1)
            strcpy(IncompleteMsg.strdirection, "逆向");

        iType = root_Obj["crossroad"].toInt();
        if (iType == 0)
            strcpy(IncompleteMsg.strcrossroad, "正常");
        else if (iType == 1)
            strcpy(IncompleteMsg.strcrossroad, "跨道");

        /*iType = root_Obj["cartype"].toInt();
        if (iType == 0)
            strcpy(IncompleteMsg.strcartype, "小型客车");
        else if (iType == 1)
            strcpy(IncompleteMsg.strcartype, "小型货车");
        else if (iType == 2)
            strcpy(IncompleteMsg.strcartype, "中型货车");
        else if (iType == 3)
            strcpy(IncompleteMsg.strcartype, "大型客车");
        else if (iType == 4)
            strcpy(IncompleteMsg.strcartype, "大型货车");
        else if (iType == 5)
            strcpy(IncompleteMsg.strcartype, "特大货车");*/

        IncompleteMsg.bweight = true;
        IncompleteMsg.getticout = ::GetTickCount();
        IncompleteMsg.bstart = 1;
        m_CarResult.getSuitMsg(cruntTime,laneno,-1,IncompleteMsg,0);
        OutputLog2("车道%d 轴数%d 车型%s 速度%d 重量%d  %s行驶  置信度%d",
                   IncompleteMsg.laneno, IncompleteMsg.axiscount, IncompleteMsg.strcartype, IncompleteMsg.speed,
                   IncompleteMsg.weight, IncompleteMsg.strcrossroad, IncompleteMsg.credible);
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

    //背景se
    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, Qt::white);
    this->setPalette(palette);

    QFile file(":/qss/widget.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    this->setStyleSheet(styleSheet);
    file.close();
    {
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
    }
    //主界面
    initTitle();
    m_realwid = new RealTimeWindow(this);
    m_hiswid = new HisWindow(this,m_DbCon->getDatabase());
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

    QObject::connect(this, SIGNAL(LedUpdate(int,STR_MSG)), this, SLOT(slot_timeout_updateLedSlot(int,STR_MSG)));

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

    m_proc = new QProcess(this);
    timer_pointer = new QTimer;
    timer_pointer->setSingleShot(true);
    timer_pointer->start(200);
    connect(timer_pointer, SIGNAL(timeout()), this, SLOT(slot_timeout_timer_pointer()));
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
    QObject::connect(m_cReal, SIGNAL(clicked()), this, SLOT(slot_showRealTitle()));
    QObject::connect(m_cHis, SIGNAL(clicked()), this, SLOT(slot_showHisTitle()));
}
void OverWidget::optimize(STR_MSG msg)//todo 优化,太复杂了
{
    char sFilename[256] = {};
    GetPrivateProfileString("CAL", "file", "./zzcal.ini", sFilename, 256, strConfigfile.c_str());

    char sVal[256] = {};
    GetPrivateProfileString("sys", "L", "3.5", sVal, 29, sFilename);
    float fwc = atof(sVal);
    OutputLog("%s  %.1f", sFilename,fwc);
    srand(unsigned(time(0)));
    {
        {
            GetPrivateProfileString("six", "no", "111BP005", sVal, 29, sFilename);
            if(strstr(msg.strcarno, sVal) != NULL)
            {
                msg.axiscount=6;
                msg.bstart = 2;
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
                msg.Axis_weight[0] = z1+iaver;
                msg.Axis_weight[1] = z2+iaver;
                msg.Axis_weight[2] = z3+iaver;
                msg.Axis_weight[3] = z4+iaver;
                msg.Axis_weight[4] = z5+iaver;
                msg.Axis_weight[5] = z6+iaver;
                msg.Axis_weight[6] = 0;
                msg.Axis_weight[7] = 0;
                msg.weight = msg.Axis_weight[0]+msg.Axis_weight[1]+msg.Axis_weight[2]
                        +msg.Axis_weight[3]+msg.Axis_weight[4]+msg.Axis_weight[5];
            }
        }
        {
            GetPrivateProfileString("six2", "no", "111BP005", sVal, 29, sFilename);
            if(strstr(msg.strcarno, sVal) != NULL)
            {
                msg.axiscount=6;
                msg.bstart = 2;
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
                msg.Axis_weight[0] = z1+iaver;
                msg.Axis_weight[1] = z2+iaver;
                msg.Axis_weight[2] = z3+iaver;
                msg.Axis_weight[3] = z4+iaver;
                msg.Axis_weight[4] = z5+iaver;
                msg.Axis_weight[5] = z6+iaver;
                msg.Axis_weight[6] = 0;
                msg.Axis_weight[7] = 0;
                msg.weight = msg.Axis_weight[0]+msg.Axis_weight[1]+msg.Axis_weight[2]
                        +msg.Axis_weight[3]+msg.Axis_weight[4]+msg.Axis_weight[5];
            }
        }
        {
            GetPrivateProfileString("four", "no", "2227886W", sVal, 29, sFilename);
            if(strstr(msg.strcarno, sVal) != NULL)
            {
                msg.axiscount=4;
                msg.bstart = 2;
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
                msg.Axis_weight[0] = z1+iaver;
                msg.Axis_weight[1] = z2+iaver;
                msg.Axis_weight[2] = z3+iaver;
                msg.Axis_weight[3] = z4+iaver;
                msg.Axis_weight[4] = 0;
                msg.Axis_weight[5] = 0;
                msg.Axis_weight[6] = 0;
                msg.Axis_weight[7] = 0;
                msg.weight = msg.Axis_weight[0]+msg.Axis_weight[1]+msg.Axis_weight[2]
                        +msg.Axis_weight[3];
            }
        }
        {
            GetPrivateProfileString("three", "no", "2227886W", sVal, 29, sFilename);
            if(strstr(msg.strcarno, sVal) != NULL)
            {
                msg.axiscount=3;
                msg.bstart = 2;
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
                msg.Axis_weight[0] = z1+iaver;
                msg.Axis_weight[1] = z2+iaver;
                msg.Axis_weight[2] = z3+iaver;
                msg.Axis_weight[3] = 0;
                msg.Axis_weight[4] = 0;
                msg.Axis_weight[5] = 0;
                msg.Axis_weight[6] = 0;
                msg.Axis_weight[7] = 0;
                msg.weight = msg.Axis_weight[0]+msg.Axis_weight[1]+msg.Axis_weight[2];
            }
        }
        if (msg.axiscount==2)
        {
            GetPrivateProfileString("two", "no", "333V1079", sVal, 29, sFilename);
            if(strstr(msg.strcarno, sVal) != NULL)
            {
                msg.axiscount=2;
                msg.bstart = 2;
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
                msg.Axis_weight[0] = z1+iaver;
                msg.Axis_weight[1] = z2+iaver;
                msg.Axis_weight[2] = 0;
                msg.Axis_weight[3] = 0;
                msg.Axis_weight[4] = 0;
                msg.Axis_weight[5] = 0;
                msg.Axis_weight[6] = 0;
                msg.Axis_weight[7] = 0;
                msg.weight = msg.Axis_weight[0]+msg.Axis_weight[1];
            }
        }
        msg.Axis_weight[0] = msg.Axis_weight[0]/m_weidu*m_weidu;
        msg.Axis_weight[1] = msg.Axis_weight[1]/m_weidu*m_weidu;
        msg.Axis_weight[2] = msg.Axis_weight[2]/m_weidu*m_weidu;
        msg.Axis_weight[3] = msg.Axis_weight[3]/m_weidu*m_weidu;
        msg.Axis_weight[4] = msg.Axis_weight[4]/m_weidu*m_weidu;
        msg.Axis_weight[5] = msg.Axis_weight[5]/m_weidu*m_weidu;
        msg.Axis_weight[6] = msg.Axis_weight[6]/m_weidu*m_weidu;
        msg.Axis_weight[7] = msg.Axis_weight[7]/m_weidu*m_weidu;
        msg.weight = msg.Axis_weight[0]+msg.Axis_weight[1]+msg.Axis_weight[2]+msg.Axis_weight[3]
                +msg.Axis_weight[4]+msg.Axis_weight[5]+msg.Axis_weight[6]+msg.Axis_weight[7];
    }
}
int OverWidget::analyzeData(STR_MSG msg)
{
    //无效处理
    strcpy(msg.strreason, " ");
    int iRet = 0;
    if(msg.speed > 120){
        strcpy(msg.strreason, "速度异常");
        iRet = 7;
    }
    if(msg.weight < 500 || msg.weight > 150000){
        strcpy(msg.strreason, "总重量异常");
        iRet = 7;
    }
    if(strstr(msg.strplatecolor, "蓝") != NULL)
    {
        if(msg.axiscount != 2){
            iRet = 7;
            strcpy(msg.strreason, "轴数异常2");
        }
        if(msg.weight > 10000){
            iRet = 7;
            strcpy(msg.strreason, "总重量异常2");
        }
    }
    //判断2,3,4,5,6轴车 的单轴重是否正常
    bool weightException = false;
    for (int i = 0; i < msg.axiscount; ++i) {
        if (msg.Axis_weight[i] < 200 || msg.Axis_weight[i] > 30000) {
            weightException = true;
            break;
        }
    }
    if (weightException) {
        strcpy(msg.strreason, "单轴重异常");
        iRet = 7;
    }
    if(msg.axiscount==0)
    {
        iRet = 7;
        strcpy(msg.strreason, "轴数异常0");
    }
    if(msg.axiscount > 8)
    {
        iRet = 7;
        strcpy(msg.strreason, "轴数异常8");
    }
    //mazq 0324
    /*if(strstr(msg.strplatecolor, "黄") != NULL)
    {
        if(msg.weight < 5000)
        {
            iRet = 7;
            strcpy(msg.strreason, "总重量异常4");
        }
    }*/

    if(strstr(msg.strcartype, "小型") != NULL)
    {
        if(msg.axiscount > 2){
            iRet = 7;
            strcpy(msg.strreason, "小车轴数异常6");
        }
    }

    if(iRet != 7)
    {
        if (msg.w < 1 || msg.h < 1 || msg.l < 1)
        {
            if(m_WkZeroNoUp == 1)
            {
                iRet = 5;
                strcpy(msg.strreason, "外廓异常");
            }
        }
        //客车不要
        if(strstr(msg.strcartype,"客") != NULL && m_PassengerNoUp==1)
            iRet = 5;
        //小车不要
        if(strstr(msg.strcartype,"小") != NULL && m_SmallNoUp==1)
            iRet = 5;
    }
    //超重
    strcpy(msg.strret, "合格");
    if(0)
    {
        if (msg.weight > m_GBXZ)
        {
            strcpy(msg.strret, "不合格");
            msg.overWeight=msg.weight - m_GBXZ;
            msg.overPercent=(msg.overWeight/m_GBXZ)*100.0f;
        }
        else
            strcpy(msg.strret, "合格");
    }
    else
    {
        int GBxz = 18000;
        if(msg.axiscount == 6)
            GBxz = 49000;
        else if(msg.axiscount == 5)
            GBxz = 43000;
        else if(msg.axiscount == 4)
        {
            GBxz = 31000;
            if(msg.Axis_space[0]<215 && msg.Axis_space[2]<145)//间距不同标准不同
                GBxz = 31000;
        }
        else if(msg.axiscount == 3)
        {
            GBxz = 25000;
            if(msg.Axis_space[0]<215 || msg.Axis_space[1]<140)
                GBxz = 25000;
        }
        msg.GBxz = GBxz;
        if (msg.weight > GBxz)
        {
            strcpy(msg.strret, "不合格");
            msg.overWeight=msg.weight - GBxz;
            msg.overPercent=(msg.overWeight/GBxz)*100.0f;
        }
        else
            strcpy(msg.strret, "合格");
    }
    //超限
    if (msg.w > m_GBXZW*1000)
        strcpy(msg.strret, "不合格");
    if (msg.l > m_GBXZL*1000)
        strcpy(msg.strret, "不合格");
    if (msg.h > m_GBXZH*1000)
        strcpy(msg.strret, "不合格");
    return iRet;
}
void OverWidget::UpdateUI(STR_MSG msg)
{
    OutputLog("-----emit ui");
    emit UpdateUISig(msg);
}
//主程序只负责要显示的内容text是什么.ledcontrol负责刷新操作
void OverWidget::slot_timeout_updateLedSlot(int iType, STR_MSG msg)
{
    Q_UNUSED(iType);
    QJsonObject jsonObject = m_LedCon->getTextJson();//大屏规则
    QJsonDocument jsonDocument(jsonObject);
    QString jsonString = jsonDocument.toJson(QJsonDocument::Compact);

    //获取列名
    QStringList result;
    QString lieToSql;//规则中出现的列名用来拼接sql语句
    // 定义 [ ] 对象
    QJsonArray SqlNameArray;//存放需要查询列的名称
    int startPos = 0;
    while (startPos < jsonString.length()) {
        int openPos = jsonString.indexOf('(', startPos);
        if (openPos == -1) {
            break;
        }
        int closePos = jsonString.indexOf(')', openPos);
        if (closePos == -1) {
            break;
        }
        result << jsonString.mid(openPos + 1, closePos - openPos - 1);
        SqlNameArray.append(jsonString.mid(openPos + 1, closePos - openPos - 1));
        lieToSql += jsonString.mid(openPos + 1, closePos - openPos - 1) +",";
        startPos = closePos + 1;
    }
    lieToSql.chop(1);

    QStringList columnNames;//关键字段:(carno)
    //根据rule查询列名// 获取查询结果// 获取规则
    QString rule = jsonObject["rule"].toString();
    if(rule.length()>4)//通过查询数据库获取关键字段
    {
        QString sql = "SELECT "+lieToSql+" FROM task "+rule;
        QJsonArray name = m_DbCon->query(SqlNameArray,sql);
        for (int i = 0; i < name.size(); i++ ) {
            columnNames.append(name.at(i).toString());
        }
    }
    else{//根据实时传来的参数
        for(int i=0;i<SqlNameArray.size();i++)
        {
            columnNames.append(msg.msgToMeta(SqlNameArray.at(i).toString()));
        }
    }

    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++ it) {
        QString page = it.key();
        QJsonValue value = it.value();
        // 处理每个页面中的字段
        if (value.isArray()) {
            QJsonArray fields = value.toArray();
            for (int i = 0; i < fields.size(); i++ ) {
                QString field = fields[i].toString();
                // 替换列名
                for (const QString& columnName : columnNames) {
                    field.replace("(" + columnName + ")", columnName);
                }
                // 更新字符串
                jsonObject.value(page).toArray()[i] = field;
                //                jsonObject[page][i] = field;
            }
        }
    }
    QJsonDocument jsonD(jsonObject);
    QString ha = jsonD.toJson(QJsonDocument::Compact);
    OutputLog("----- LED JSON :%s",ha.toLocal8Bit().toStdString().c_str());
    m_LedCon->UpdateText(jsonObject);
}
int OverWidget::StartLED()
{
    char strVal[256] = {};
    GetPrivateProfileString("LED", "type", "1", strVal, 256, strConfigfile.c_str());
    int type = atoi(strVal);
    GetPrivateProfileString("LED", "page", "1", strVal, 265, strConfigfile.c_str());
    int page = atoi(strVal);
    GetPrivateProfileString("LED", "time", "5000", strVal, 256, strConfigfile.c_str());
    int time = atoi(strVal);
    GetPrivateProfileString("LED", "text", "5000", strVal, 256, strConfigfile.c_str());
    QString text = QString::fromLocal8Bit(strVal);

    m_LedCon = new CWTLEDControl(type,page,time,text);
    m_LedCon->Work();
    pthread_t m_dwHeartID;//线程ID
    pthread_create(&m_dwHeartID, nullptr, &FlushThreadProc, this);
    m_LedCon->ResetLed();
    return 1;
}

void OverWidget::slot_showRealTitle()
{
    m_cReal->setButtonBackground(":/png/image/real-press.png");
    m_cHis->setButtonBackground(":/png/image/his.png");
    m_hiswid->hide();
    m_realwid->show();
}
void OverWidget::slot_showHisTitle()
{
    m_cReal->setButtonBackground(":/png/image/real.png");
    m_cHis->setButtonBackground(":/png/image/his-press.png");
    m_realwid->hide();
    m_hiswid->show();
}
void OverWidget::MSesGCallback(int Lane, char *filename, int pictype, char *carno, char *pcolor, char *cartype)
{
    OverWidget * m_Socket = (OverWidget *)this;
    STR_MSG *msg = nullptr;
    STR_MSG IncompleteMsg = {0};
    IncompleteMsg.laneno = Lane;
    strcpy(IncompleteMsg.carnos[pictype],carno);//复制到  车牌号  数组中
    strcpy(IncompleteMsg.carpics[pictype],filename);//复制到  车牌照片  中
    strcpy(IncompleteMsg.strplatecolor,pcolor);
    strcpy(IncompleteMsg.strcartype,cartype);

    //车辆信息: 车重(carresult),车照片(picpool),车轮廓(todo)
    //车辆到达时间以哪个为准? 重量不可靠有时候不一定给.车照片:具体是哪个先到为主?还是固定前相机?
    //todo 所有的msg最好在carresult中修改避免

    m_Socket->m_QueueSect.Lock();
    {
        //        char* dd = strstr(filename,".jpg");
        //enum PicType{ SMALL, FRONT, AFTER, SIDE,SIDE2,AFRSIDE, ALL,};
        switch (pictype) {
        case SMALL:
            OutputLog("----MSesGCallback: SMALL");
            msg = m_CarResult.getSuitMsg(::GetTickCount(),Lane,SMALL,IncompleteMsg,m_picPool->numQueues);
            if(!strstr(filename,"无")){
                QByteArray picStrByteArray(filename, sizeof(filename));
                //添加到队列
                m_picPool->addToQueue(SMALL,picStrByteArray);//添加到picpool中
            }
            m_picPool->matchingPicFromPool(msg,SMALL);
            break;
        case FRONT://todo 前车牌在有两张情况下需要特殊处理
            OutputLog("----MSesGCallback: FRONT");
            msg = m_CarResult.getSuitMsg(::GetTickCount(),Lane,FRONT,IncompleteMsg,m_picPool->numQueues);
            if(!strstr(filename,"无")){
                QByteArray picStrByteArray(filename, sizeof(filename));
                //添加到队列
                m_picPool->addToQueue(FRONT,picStrByteArray);//添加到picpool中
            }
            m_picPool->matchingPicFromPool(msg,FRONT);
            break;
        case FRONT1:
            OutputLog("----MSesGCallback: FRONT1");
            msg = m_CarResult.getSuitMsg(::GetTickCount(),Lane,FRONT1,IncompleteMsg,m_picPool->numQueues);
            if(!strstr(filename,"无")){
                QByteArray picStrByteArray(filename, sizeof(filename));
                //添加到队列
                m_picPool->addToQueue(FRONT1,picStrByteArray);//添加到picpool中
            }
            m_picPool->matchingPicFromPool(msg,FRONT1);
            break;
        case AFTER:
            OutputLog("----MSesGCallback: AFTER");
            msg = m_CarResult.getSuitMsg(::GetTickCount(),Lane,AFTER,IncompleteMsg,m_picPool->numQueues);
            if(!strstr(filename,"无")){
                QByteArray picStrByteArray(filename, sizeof(filename));
                //添加到队列
                m_picPool->addToQueue(AFTER,picStrByteArray);//添加到picpool中
            }
            m_picPool->matchingPicFromPool(msg,AFTER);
            break;
        case SIDE:
            OutputLog("----MSesGCallback: SIDE");
            msg = m_CarResult.getSuitMsg(::GetTickCount(),Lane,SIDE,IncompleteMsg,m_picPool->numQueues);
            if(!strstr(filename,"无")){
                QByteArray picStrByteArray(filename, sizeof(filename));
                //添加到队列
                m_picPool->addToQueue(SIDE,picStrByteArray);//添加到picpool中
            }
            m_picPool->matchingPicFromPool(msg,SIDE);
            break;
        case AFRSIDE:
            OutputLog("----MSesGCallback: AFRSIDE");
            msg = m_CarResult.getSuitMsg(::GetTickCount(),Lane,AFRSIDE,IncompleteMsg,m_picPool->numQueues);
            if(!strstr(filename,"无")){
                QByteArray picStrByteArray(filename, sizeof(filename));
                //添加到队列
                m_picPool->addToQueue(AFRSIDE,picStrByteArray);//添加到picpool中
            }
            m_picPool->matchingPicFromPool(msg,AFRSIDE);
            break;
        default:
            break;
        }
    }
    m_Socket->m_QueueSect.Unlock();
}
void OverWidget::onClose()
{
    m_LedCon->Close();
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
