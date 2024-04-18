#include "common_utils.h"
#ifndef CARRESULT_H
#define CARRESULT_H

class CarResult
{
public:
    CarResult(){
        //基础配置
        std::string sPath = QCoreApplication::applicationDirPath().toStdString();
        std::string strConfigfile = sPath + "/config.ini";
        char  szVal[200];
        GetPrivateProfileString("SYS", "IsCam", "1", szVal, 29, strConfigfile.c_str());
        m_hasCam = atoi(szVal);
        GetPrivateProfileString("外廓", "是否使用", "0", szVal, 29, strConfigfile.c_str());
        m_hasWK = atoi(szVal);

        for(int i =0;i<100;++i)
        {
            m_QVectorResult[i].time=0;
            m_QVectorResult[i].msg = new STR_MSG;
            memset(m_QVectorResult[i].msg, 0, sizeof(STR_MSG));
            m_QVectorResult[i].msg->hasCam = m_hasCam;
            m_QVectorResult[i].msg->hasWK = m_hasWK;
        }
    }
    ~CarResult(){
    }
    STR_MSG* newMsg(int time)
    {
        int pos = findMinTimeMember();
        m_QVectorResult[pos].time=time;
        memset(m_QVectorResult[pos].msg, 0, sizeof(STR_MSG));
        m_QVectorResult[pos].msg->bstart=1;//开始工作
        m_QVectorResult[pos].msg->hasCam = m_hasCam;
        m_QVectorResult[pos].msg->hasWK = m_hasWK;
        OutputLog("----carresult: IMPORT new msg[%d]",pos);
        return m_QVectorResult[pos].msg;
    }
    //向已经工作的结构添加新的数据
    //获取该车道上最早一次车辆信息
    //todo 怎么确保重量信息与照片匹配
    //根据车道.type找到最近时间没有type的结构
    STR_MSG* getSuitMsg(int time,int laneno,int type,STR_MSG& IncompleteMsg,int picNum){
        mutex.lock();
        QDateTime current_date_time = QDateTime::currentDateTime();
        QString strTemp = current_date_time.toString("yyyy-MM-dd hh:mm:ss");
        int mintime=INT_MAX;
        int minTimePos=-1;
        for (int i = 0; i < 100; ++i) {
            //获取车道中处于工作中状态的最早一个结构
            //同一车道
            if(m_QVectorResult[i].msg->laneno == laneno){
                //开始工作的结构
                if(m_QVectorResult[i].msg->bstart == 1){
                    if(type > -1) {
                        //此结构没有存储type的照片和车牌
                        if(strlen(m_QVectorResult[i].msg->carpics[type])==0){
                            if(mintime>qAbs(m_QVectorResult[i].time-time))
                            {
                                mintime = qAbs(m_QVectorResult[i].time-m_QVectorResult[minTimePos].time);
                                minTimePos = i;
                            }
                        }
                    }
                    else {
                        if(!m_QVectorResult[i].msg->bweight){//没有重量
                            if(mintime>qAbs(m_QVectorResult[i].time-time))
                            {
                                mintime = qAbs(m_QVectorResult[i].time-time);
                                minTimePos = i;
                            }
                        }
                    }
                }
            }
        }
        OutputLog("----carresult: suit msg laneno:%d type:%d msg[%d]",laneno,type,minTimePos);
        if(minTimePos>-1){
            STR_MSG* msg = m_QVectorResult[minTimePos].msg;
            if(type > -1){//照片
                if(!(strlen(msg->pictime)>0)){//第一张照片还没到
                    strcpy(msg->pictime, strTemp.toStdString().c_str());
                    msg->firstcarpicpos = type;
                }
                strcpy(msg->carnos[type],IncompleteMsg.carnos[type]);//复制到  车牌号  数组中
                strcpy(msg->carpics[type],IncompleteMsg.carpics[type]);//复制到  车牌照片  中
                msg->numpic+=1;//每次来一张照片就保存
                if(msg->numpic == picNum)
                    msg->picReady=true;
                if(type == FRONT)
                {
                    strcpy(msg->strcarno, IncompleteMsg.carnos[FRONT]);
                    strcpy(msg->strplatecolor, IncompleteMsg.strplatecolor);
                    strcpy(msg->strcartype, IncompleteMsg.strcartype);
                    strcpy(msg->strtime, strTemp.toStdString().c_str());
                }
            }
            else{//重量
                msg->bweight = true;
                msg->axiscount = IncompleteMsg.axiscount;
                msg->carlen = IncompleteMsg.carlen;
                msg->speed = IncompleteMsg.speed;
                msg->weight = IncompleteMsg.weight;
                msg->Axis_weight[0] = IncompleteMsg.Axis_weight[0];
                msg->Axis_weight[1] = IncompleteMsg.Axis_weight[1];
                msg->Axis_weight[2] = IncompleteMsg.Axis_weight[2];
                msg->Axis_weight[3] = IncompleteMsg.Axis_weight[3];
                msg->Axis_weight[4] = IncompleteMsg.Axis_weight[4];
                msg->Axis_weight[5] = IncompleteMsg.Axis_weight[5];
                msg->Axis_weight[6] = IncompleteMsg.Axis_weight[6];
                msg->Axis_weight[7] = IncompleteMsg.Axis_weight[7];
                msg->Axis_space[0] = IncompleteMsg.Axis_space[0];
                msg->Axis_space[1] = IncompleteMsg.Axis_space[1];
                msg->Axis_space[2] = IncompleteMsg.Axis_space[2];
                msg->Axis_space[3] = IncompleteMsg.Axis_space[3];
                msg->Axis_space[4] = IncompleteMsg.Axis_space[4];
                msg->Axis_space[5] = IncompleteMsg.Axis_space[5];
                msg->Axis_space[6] = IncompleteMsg.Axis_space[6];
                strcpy(msg->deviceNo,IncompleteMsg.deviceNo);
                strcpy(msg->strdirection,IncompleteMsg.strdirection);
                strcpy(msg->strcrossroad,IncompleteMsg.strcrossroad);
            }
            mutex.unlock();
            return m_QVectorResult[minTimePos].msg;
        }
        else{
            int pos = findMinTimeMember();
            m_QVectorResult[pos].time=time;
            memset(m_QVectorResult[pos].msg, 0, sizeof(STR_MSG));
            m_QVectorResult[pos].msg->bstart=1;//开始工作
            m_QVectorResult[pos].msg->hasCam = m_hasCam;
            m_QVectorResult[pos].msg->hasWK = m_hasWK;
            OutputLog("----carresult: IMPORT new msg[%d]",pos);
            STR_MSG* msg = m_QVectorResult[pos].msg;
            msg->laneno = laneno;
            if(type > -1){//照片
                if(!(strlen(msg->pictime)>0)){//第一张照片还没到
                    strcpy(msg->pictime, strTemp.toStdString().c_str());
                    msg->firstcarpicpos = type;
                }
                strcpy(msg->carnos[type],IncompleteMsg.carnos[type]);//复制到  车牌号  数组中
                strcpy(msg->carpics[type],IncompleteMsg.carpics[type]);//复制到  车牌照片  中
                msg->numpic+=1;//每次来一张照片就保存
                if(msg->numpic == picNum)
                    msg->picReady=true;
                if(type == FRONT)
                {
                    strcpy(msg->strcarno, IncompleteMsg.carnos[FRONT]);
                    strcpy(msg->strplatecolor, IncompleteMsg.strplatecolor);
                    strcpy(msg->strcartype, IncompleteMsg.strcartype);
                    strcpy(msg->strtime, strTemp.toStdString().c_str());
                }
            }
            else{//重量
                msg->bweight = true;
                msg->axiscount = IncompleteMsg.axiscount;
                msg->carlen = IncompleteMsg.carlen;
                msg->speed = IncompleteMsg.speed;
                msg->weight = IncompleteMsg.weight;
                msg->Axis_weight[0] = IncompleteMsg.Axis_weight[0];
                msg->Axis_weight[1] = IncompleteMsg.Axis_weight[1];
                msg->Axis_weight[2] = IncompleteMsg.Axis_weight[2];
                msg->Axis_weight[3] = IncompleteMsg.Axis_weight[3];
                msg->Axis_weight[4] = IncompleteMsg.Axis_weight[4];
                msg->Axis_weight[5] = IncompleteMsg.Axis_weight[5];
                msg->Axis_weight[6] = IncompleteMsg.Axis_weight[6];
                msg->Axis_weight[7] = IncompleteMsg.Axis_weight[7];
                msg->Axis_space[0] = IncompleteMsg.Axis_space[0];
                msg->Axis_space[1] = IncompleteMsg.Axis_space[1];
                msg->Axis_space[2] = IncompleteMsg.Axis_space[2];
                msg->Axis_space[3] = IncompleteMsg.Axis_space[3];
                msg->Axis_space[4] = IncompleteMsg.Axis_space[4];
                msg->Axis_space[5] = IncompleteMsg.Axis_space[5];
                msg->Axis_space[6] = IncompleteMsg.Axis_space[6];
                strcpy(msg->deviceNo,IncompleteMsg.deviceNo);
                strcpy(msg->strdirection,IncompleteMsg.strdirection);
                strcpy(msg->strcrossroad,IncompleteMsg.strcrossroad);
            }
            mutex.unlock();
            return msg;
        }
    }
    void deleteMsg(STR_MSG* msg)
    {
        mutex.lock();
        if(msg != nullptr){
            for (int i = 0; i < 100; ++i) {
                if(m_QVectorResult[i].msg == msg){
                    memset(msg, 0, sizeof(STR_MSG));
                    m_QVectorResult[i].time=0;
                    OutputLog("----carresult: IMPORT delete msg[%d]",i);
                }
            }
        }
        mutex.unlock();
    }
    STR_MSG checkMsgState(int outTime){
        mutex.lock();
        STR_MSG ret = {0};
        int cruntTime = ::GetTickCount();
        for(int i =0;i<100;++i){
            if(m_QVectorResult[i].msg==nullptr)
                continue;
            if(m_QVectorResult[i].msg->bstart==1){//工作结构
                //没有超时
                if((cruntTime-m_QVectorResult[i].time)<outTime){
                    if(m_QVectorResult[i].msg->bweight && (m_QVectorResult[i].msg->hasCam ? m_QVectorResult[i].msg->picReady : 1) && (m_QVectorResult[i].msg->hasWK ? m_QVectorResult[i].msg->wkReady : 1))
                    {
                        m_QVectorResult[i].msg->bstart=2;
                        //拷过去
                        memcpy(&ret,m_QVectorResult[i].msg,sizeof(STR_MSG));
                        //删除
                        memset(m_QVectorResult[i].msg, 0, sizeof(STR_MSG));
                        m_QVectorResult[i].time=0;
                        OutputLog("----carresult: checkMsgState msg[%d]",i);
                        mutex.unlock();
                        return ret;
                    }
                }else{//超时了
                    if(!m_QVectorResult[i].msg->bweight)
                        m_QVectorResult[i].msg->bstart=3;//没重量
                    if(!m_QVectorResult[i].msg->picReady)
                        m_QVectorResult[i].msg->bstart=4;//没照片
                    if(!m_QVectorResult[i].msg->picReady && !m_QVectorResult[i].msg->bweight)
                        m_QVectorResult[i].msg->bstart=5;//都没有
                    //拷过去
                    memcpy(&ret,m_QVectorResult[i].msg,sizeof(STR_MSG));
                    //删除
                    memset(m_QVectorResult[i].msg, 0, sizeof(STR_MSG));
                    m_QVectorResult[i].time=0;
//                    if(!ret.picReady && !ret.bweight)
//                        ret.bstart = 0;//丢掉
                    OutputLog("----carresult: checkMsgState start:%d OUT TIME msg[%d]",ret.bstart,i);
                    mutex.unlock();
                    return ret;
                }
            }
        }
        mutex.unlock();
        return ret;
    }
private:
    //基础设置
    int m_hasCam;
    int m_hasWK;
    QMutex mutex;
    QMutex weightMutex;
    typedef struct STR_Result{
        int time;       //程序从开启到收到数据的时间--不是系统时间
        STR_MSG *msg;
    }result;
    result m_QVectorResult[100];
    int findMinTimeMember() {
        int minTimePos=0;
        for (int i = 0; i < 100; ++i) {
            if(m_QVectorResult[i].msg->bstart==0){//空闲状态
                minTimePos = i;
                return minTimePos;
            }
        }
        return minTimePos;
    }

};

#endif // CARRESULT_H
