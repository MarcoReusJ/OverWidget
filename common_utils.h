#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <QWidget>
#include <QShortcut>
#include <QGroupBox>
#include <stdio.h>
#include <string>
#include <queue>
#include <QLCDNumber>
#include <list>
#include <map>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTabWidget>
#include <QTimer>
#include <QProcess>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QThread>
#include <QQueue>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QtSql>
#include <QJsonDocument>
#include <QVector>
#include <QString>

#include "include/LogEx.h"
#include "resultMode.h"
#include "custom_time_control.h"
#include "custom_button.h"
#include "picshow.h"

#ifdef WIN32
#include <windows.h>
#include <winuser.h>
#include <io.h>
#include <iostream>
#include "include/pthread/pthread.h"
#include "CriticalSection.h"
#include "wtnetcontrol.h"
#include "wtdevcontrol.h"
#include "wtprocontrol.h"
#include "wtcamcontrol.h"
#include "wtledcontrol.h"
#include "wtcarcontrol.h"
#define GetPrivateProfileString GetPrivateProfileStringA
#define GetPrivateProfileInt GetPrivateProfileIntA
#define WritePrivateProfileString WritePrivateProfileStringA
#define closeSocket closesocket
#define socklen_t int
#define PLATPATH "\\"
#define CHINESELEN 2
#else
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>
#include "getconfig.h"
#include "CustomST.h"
typedef void *LPVOID;
#define PLATPATH "/"
#define CHINESELEN 3
#define SOCKLEN unsigned int
#define byte unsigned int
#define SOCKET int
#define  INVALID_SOCKET -1
#define closeSocket close
#define SOCKET_ERROR -1
#define Sleep(x) usleep((x)*1000)
#define sprintf_s sprintf
#endif

#include "include/PlayM4.h"
using namespace std;
#define STR_MSG_MAXCAR 8 //结构体最大存储车照片数


enum PicType{ SMALL, FRONT,FRONT1, AFTER, SIDE,AFRSIDE,SIDE2, ALL};
//消息结构
struct STR_MSG
{
    bool botherpic;
    int dataId;
    bool hasWK;                 //是否有外廓
    bool hasCam;                //是否有相机
    int bstart;                //工作状态 0:空闲 1:正在工作 2:工作结束可以保存,保存后置为0 3:超时
    char carnos[STR_MSG_MAXCAR][20];         //车牌数组,最多8个车牌
    char carpics[STR_MSG_MAXCAR][256];       //车照片数组,最多8个
    char pictime[30];           //获取到车辆第一张照片的时间
    int firstcarpicpos;         //第一张照片是哪个PicType{ SMALL, FRONT, AFTER, SIDE,SIDE2, ALL,};
    int numpic;                 //已经有了几张照片了
    bool picReady;             //当所有照片都有了,或者超时了就开始保存
    bool wkReady;               //外廓数据有了,
    bool bweight;               //重量有了
    char strcarno[20];
    char strplatecolor[20];
    char strtime[30];
    int laneno;                 //车道
    int axiscount;              //轴数
    int carlen;
    char strweighttime[30];
    char strcartype[30];
    char strdirection[30];
    char strcrossroad[30];
    int speed;
    int credible;
    int weight;
    char strcarnos[20];
    float overWeight;
    float overPercent;
    int getticout;
    int GBxz;
    int Axis_weight[8];
    int Axis_space[7];
    int Axis_group[4];
    char strret[30];
    char deviceNo[30];
    char strreason[300];
    //wk
    int w;
    int h;
    int l;
    bool bprofile;
    QString msgToMeta(QString key){
        if(key=="strcarno")
            return QString::fromLocal8Bit(strcarno);
        if(key=="overWeight")
            return QString::number(overWeight);
        return QString();
    }
};


//设备信息
typedef struct _DeviceNode
{
    int ID;
    char Type;          //设备类型:0LED，1仪表 2DVR 3车道前  4车道后  5侧  6全
    int ilane;          //设备编号
    long LoginID;       //设备登录iD
    char sDvrNo[30];	//设备编号
    char sname[100];
    char sIP[20];
    int iPort;
    char user[50];
    char pwd[50];
    int  bIsOnline;		//是否已登录标志
    int nOnlineFlag;	//标志上次检测是否在线：0正常、1掉线
}DeviceNode;

//线程池-结构
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

#define UNCONNECT 					  0	//未连接
#define NORMAL	 					  1	//正常

#endif // COMMON_UTILS_H
