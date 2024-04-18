#include "overwidget.h"
#include "ui_overwidget.h"
#include <QFile>
#include <QPainter>
#include <QDebug>
#include <QRect>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QGraphicsWidget>
#include <QGraphicsLayout>
#include <QMessageBox>
#include <QValueAxis>
#include <QtMath>
#include <QTextCodec>

int loglane = 1;

#ifdef WIN32
Logger pTestLogger;
#else
void DeleteFileA(const char* szfilename)
{
    QFile file(QString::fromLocal8Bit(szfilename));
    file.remove();
}
#endif
#include <QStorageInfo>
inline QString GBK2UTF8(QByteArray &inStr)
{
    QTextCodec *gbk = QTextCodec::codecForName("gbk");
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");

    char *p = inStr.data();
    QString str = gbk->toUnicode(p);

    QByteArray utf8_bytes=utf8->fromUnicode(str);
    p = utf8_bytes.data();
    str = p;

    return str;

}

inline QString UTF82GBK(QByteArray &inStr)
{
    QTextCodec *gbk = QTextCodec::codecForName("gbk");
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");

    char *p = inStr.data();
    QString str = utf8->toUnicode(p);

    QByteArray utf8_bytes=gbk->fromUnicode(str);
    p = utf8_bytes.data();
    str = p;

    return str;
}


#if defined(__WIN32__) || defined(_WIN32)
#define WS_VERSION_CHOICE1 0x202/*MAKEWORD(2,2)*/
#define WS_VERSION_CHOICE2 0x101/*MAKEWORD(1,1)*/
int initializeWinsockIfNecessary(void)
{
    static int _haveInitializedWinsock = 0;
    WSADATA	wsadata;
    if (!_haveInitializedWinsock)
    {
        if ((WSAStartup(WS_VERSION_CHOICE1, &wsadata) != 0)
                && ((WSAStartup(WS_VERSION_CHOICE2, &wsadata)) != 0))
        {
            return 0; /* error in initialization */
        }
        if ((wsadata.wVersion != WS_VERSION_CHOICE1)
                && (wsadata.wVersion != WS_VERSION_CHOICE2))
        {
            WSACleanup();
            return 0; /* desired Winsock version was not available */
        }
        _haveInitializedWinsock = 1;
    }

    return 1;
}

int releaseWinsockIfNecessary(void)
{
    WSACleanup();
    return 1;
}
#else
int initializeWinsockIfNecessary(void) { return 1; }
int releaseWinsockIfNecessary(void){ return 1; }
unsigned long GetTickCount()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

void* ThreadDetectHK(LPVOID param);
void* ReConnectDVR(LPVOID pParam);
void* FlushThreadProc(LPVOID pParam);
void* VedioMsgThreadProc(LPVOID pParam);
void* HisVedioMsgThreadProc(LPVOID pParam);
void* SendMsgThreadProc(LPVOID pParam);
void* CheckStoreThreadProc(LPVOID pParam);
void* ChnlDataThreadProc(LPVOID pParam);
void* ParseDataThreadProc(LPVOID pParam);
void* UploadMsgThreadProc(LPVOID pParam);
void* ReUploadMsgThreadProc(LPVOID pParam);
void* UploadDevStateThreadProc(LPVOID pParam);
void* UploadVideoThreadProc(LPVOID pParam);
void* CSocketThread(LPVOID pParam);
void* CClientThread(LPVOID pParam);
OverWidget *g_Over;
void Split(vector<string>& strVecResult, const string& strResource, const string& strSplitOptions = " ");
extern QSqlDatabase g_DB;
int file_exists(char *filename)
{
    return (access(filename, 6) == 0);
}

void g_pArrival()  //车辆到达时的回调函数类型
{
    g_Over->OutputLog2("---车辆到达---");
}

void g_ppLeave()  //车辆离开时的回调函数类型
{
    g_Over->OutputLog2("---车辆离开---");
}

void g_ppResult(RESUTL *ret)//检测结果
{
    g_Over->AnalysisCarResult(ret);
}

QString Encode(const char* Data, int DataByte)
{
    //编码表
    const char EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    //返回值
    QString strEncode;
    unsigned char Tmp[4] = { 0 };
    int LineLength = 0;
    for (int i = 0; i < (int)(DataByte / 3); i++)
    {
        Tmp[1] = *Data++;
        Tmp[2] = *Data++;
        Tmp[3] = *Data++;
        strEncode += EncodeTable[Tmp[1] >> 2];
        strEncode += EncodeTable[((Tmp[1] << 4) | (Tmp[2] >> 4)) & 0x3F];
        strEncode += EncodeTable[((Tmp[2] << 2) | (Tmp[3] >> 6)) & 0x3F];
        strEncode += EncodeTable[Tmp[3] & 0x3F];
        if (LineLength += 4, LineLength == 76)
        {
            strEncode += "\r\n"; LineLength = 0;
        }
    }
    //对剩余数据进行编码
    int Mod = DataByte % 3;
    if (Mod == 1)
    {
        Tmp[1] = *Data++;
        strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
        strEncode += EncodeTable[((Tmp[1] & 0x03) << 4)];
        strEncode += "==";
    }
    else if (Mod == 2)
    {
        Tmp[1] = *Data++;
        Tmp[2] = *Data++;
        strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
        strEncode += EncodeTable[((Tmp[1] & 0x03) << 4) | ((Tmp[2] & 0xF0) >> 4)];
        strEncode += EncodeTable[((Tmp[2] & 0x0F) << 2)];
        strEncode += "=";
    }
    return strEncode;
}

QString GetPicBase64Data(QString picName)
{
    QString strData;
    QFile file(picName);
    if (file.open(QIODevice::ReadOnly/* | QIODevice::Text*/))//file)//
    {
        long filelen = (long)file.size();
        QDataStream in(&file);
        unsigned char* fileBuff = new unsigned char[filelen+1];
        memset(fileBuff, 0x00, filelen+1);
        in.readRawData((char *)fileBuff, filelen);
        file.close();
        strData = Encode((const char*)fileBuff, filelen);
        delete[]fileBuff;
        fileBuff = NULL;
    }
    return strData;
}

time_t StringToDatetime(string str)
{
    char *cha = (char*)str.data();             // 将string转换成char*。
    tm tm_;                                    // 定义tm结构体。
    int year, month, day, hour, minute, second;// 定义时间的各个int临时变量。
    sscanf(cha, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);// 将string存储的日期时间，转换为int临时变量。
    tm_.tm_year = year - 1900;                 // 年，由于tm结构体存储的是从1900年开始的时间，所以tm_year为int临时变量减去1900。
    tm_.tm_mon = month - 1;                    // 月，由于tm结构体的月份存储范围为0-11，所以tm_mon为int临时变量减去1。
    tm_.tm_mday = day;                         // 日。
    tm_.tm_hour = hour;                        // 时。
    tm_.tm_min = minute;                       // 分。
    tm_.tm_sec = second;                       // 秒。
    tm_.tm_isdst = 0;                          // 非夏令时。
    time_t t_ = mktime(&tm_);                  // 将tm结构体转换成time_t格式。
    return t_;                                 // 返回值。
}


void CALLBACK MSesGCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
    OverWidget * m_Socket = (OverWidget *)pUser;
    int i = 0;
    FILE *fSnapPic = NULL;
    char filename[256] = {};
    char CarColor[12] = {};
    char CarNO[16] = {};
    // 检查目录是否存在，若不存在则新建
    char strPath[256] = {};
    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, m_Socket->strConfigfile.c_str());

    QDateTime current_date_time = QDateTime::currentDateTime();
    QString dir_str = QString(strPath)+PLATPATH+current_date_time.toString("yyyy-MM-dd");
    QDir dir;
    if (!dir.exists(dir_str))
        dir.mkpath(dir_str);
    QString dir_strCar = QString(strPath)+PLATPATH+"carinfo";
    if (!dir.exists(dir_strCar))
        dir.mkpath(dir_strCar);

    switch (lCommand)
    {
    case COMM_UPLOAD_PLATE_RESULT:
    {
        NET_DVR_PLATE_RESULT struPlateResult = { 0 };
        memcpy(&struPlateResult, pAlarmInfo, sizeof(struPlateResult));
        m_Socket->OutputLog2("车牌号: %s", struPlateResult.struPlateInfo.sLicense);//车牌号
        switch (struPlateResult.struPlateInfo.byColor)//车牌颜色
        {
        case VCA_BLUE_PLATE:
            m_Socket->OutputLog2("车辆颜色: 蓝色");
            sprintf_s(CarColor, "蓝色");
            break;
        case VCA_YELLOW_PLATE:
            m_Socket->OutputLog2("车辆颜色: 黄色");
            sprintf_s(CarColor, "黄色");
            break;
        case VCA_WHITE_PLATE:
            m_Socket->OutputLog2("车辆颜色: 白色");
            sprintf_s(CarColor, "白色");
            break;
        case VCA_BLACK_PLATE:
            m_Socket->OutputLog2("车辆颜色: 黑色");
            sprintf_s(CarColor, "黑色");
            break;
        case VCA_GREEN_PLATE:
            m_Socket->OutputLog2("车辆颜色: 绿色");
            sprintf_s(CarColor, "绿色");
            break;
        case VCA_RED_PLATE:
            m_Socket->OutputLog2("车辆颜色: 红色");
            sprintf_s(CarColor, "红色");
            break;
        case VCA_ORANGE_PLATE:
            m_Socket->OutputLog2("车辆颜色: 橙色");
            sprintf_s(CarColor, "橙色");
            break;
        case VCA_BKAIR_PLATE:
            m_Socket->OutputLog2("车辆颜色: 民航黑色");
            sprintf_s(CarColor, "民航黑色");
            break;
        default:
            m_Socket->OutputLog2("车辆颜色: 其他");
            sprintf_s(CarColor, "其他");
            break;
        }
        //场景图
        if (struPlateResult.dwPicLen != 0 && struPlateResult.byResultType == 1)
        {
            QDateTime current_date_time = QDateTime::currentDateTime();
            QString strTemp = current_date_time.toString("yyyy-MM-dd hh:mm:ss");
            sprintf(filename, "%s%s%s-%s.jpg", dir_str.toStdString().c_str(),PLATPATH, strTemp.toStdString().c_str(), struPlateResult.struPlateInfo.sLicense);
            fSnapPic = fopen(filename, "wb");
            fwrite(struPlateResult.pBuffer1, struPlateResult.dwPicLen, 1, fSnapPic);
            fclose(fSnapPic);
        }
        //其他信息处理......
        break;
    }
    case COMM_ITS_PLATE_RESULT:
    {
        NET_ITS_PLATE_RESULT struITSPlateResult = { 0 };
        memcpy(&struITSPlateResult, pAlarmInfo, sizeof(struITSPlateResult));
        for (i = 0; i < struITSPlateResult.dwPicNum; i++)
        {
#ifdef WIN32
            strcpy(CarNO, struITSPlateResult.struPlateInfo.sLicense);
#else
            QTextCodec *gbk = QTextCodec::codecForName("gbk");
            QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
            char *p = struITSPlateResult.struPlateInfo.sLicense;
            QString str = gbk->toUnicode(p);
            QByteArray utf8_bytes=utf8->fromUnicode(str);
            p = utf8_bytes.data();
            m_Socket->OutputLog2("%s",p);
            //str = p;
            //m_Socket->OutputLog2("%s",str.toStdString().c_str());
            strcpy(CarNO, p);
#endif
            if (strstr(CarNO, "无") != NULL)
                return;

            m_Socket->OutputLog2("识别车牌号: %s   车道号: %d  方向: %d  车型: %d", CarNO,
                struITSPlateResult.byDriveChan, struITSPlateResult.struPicInfo[i].byPicRecogMode, struITSPlateResult.struVehicleInfo.byVehicleType);//车牌号
            switch (struITSPlateResult.struPlateInfo.byColor)//车牌颜色
            {
            case VCA_BLUE_PLATE:
                m_Socket->OutputLog2("车辆颜色: 蓝色");
                sprintf_s(CarColor, "蓝色");
                break;
            case VCA_YELLOW_PLATE:
                m_Socket->OutputLog2("车辆颜色: 黄色");
                sprintf_s(CarColor, "黄色");
                break;
            case VCA_WHITE_PLATE:
                m_Socket->OutputLog2("车辆颜色: 白色");
                sprintf_s(CarColor, "白色");
                break;
            case VCA_BLACK_PLATE:
                m_Socket->OutputLog2("车辆颜色: 黑色");
                sprintf_s(CarColor, "黑色");
                break;
            case VCA_GREEN_PLATE:
                m_Socket->OutputLog2("车辆颜色: 绿色");
                sprintf_s(CarColor, "绿色");
                break;
            case VCA_RED_PLATE:
                m_Socket->OutputLog2("车辆颜色: 红色");
                sprintf_s(CarColor, "红色");
                break;
            case VCA_ORANGE_PLATE:
                m_Socket->OutputLog2("车辆颜色: 橙色");
                sprintf_s(CarColor, "橙色");
                break;
            case VCA_BKAIR_PLATE:
                m_Socket->OutputLog2("车辆颜色: 民航黑色");
                sprintf_s(CarColor, "民航黑色");
                break;
            default:
                m_Socket->OutputLog2("车辆颜色: 其他");
                sprintf_s(CarColor, "其他");
                break;
            }

            QDateTime current_date_time = QDateTime::currentDateTime();
            QString s = current_date_time.toString("yyyy-MM-dd hh-mm-ss");
            QString sss = current_date_time.toString("yyyy-MM-dd hh:mm:ss");
            //车牌小图片
            if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 0))
            {
                //if (struITSPlateResult.struPicInfo[i].byPicRecogMode == 0)  //正向
                {
                    m_Socket->OutputLog2("%s", "小拍照");
                    sprintf(filename, "%s%s%d-%s-%s.jpg", dir_str.toStdString().c_str(), PLATPATH, struITSPlateResult.byDriveChan, s.toStdString().c_str(), CarNO);
                    fSnapPic = fopen(filename, "wb");
                    fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, fSnapPic);
                    fclose(fSnapPic);
                    m_Socket->m_QueueSect.Lock();
                    auto ifind = m_Socket->m_maplaneAndmsg.find(struITSPlateResult.byDriveChan);
                    if (ifind != m_Socket->m_maplaneAndmsg.end())
                    {
                        STR_MSG *msg = ifind->second;
                        strcpy(msg->strsmallpic, filename);
                    }
                    m_Socket->m_QueueSect.Unlock();
                }
            }
            //保存场景图
            if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 1) || (struITSPlateResult.struPicInfo[i].byType == 2))
            {
                if (struITSPlateResult.struPicInfo[i].byPicRecogMode == 0)  //正向
                {
                    m_Socket->OutputLog2("%s","前拍照");
                    sprintf(filename, "%s%s%d-%s-%s前.jpg", dir_str.toStdString().c_str(),PLATPATH, struITSPlateResult.byDriveChan, s.toStdString().c_str(), CarNO);
                    //m_Socket->OutputLog2("%s",filename);
                    fSnapPic = fopen(filename, "wb");
                    fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, fSnapPic);
                    fclose(fSnapPic);                  
                    //入队
                    m_Socket->m_QueueSect.Lock();
                    auto ifind = m_Socket->m_maplaneAndmsg.find(struITSPlateResult.byDriveChan);
                    if (ifind != m_Socket->m_maplaneAndmsg.end())
                    {
                        STR_MSG *msg = ifind->second;
                        if (strstr(CarNO, "无") == NULL)
                        {
                            strcpy(msg->strplatecolor, CarColor);
                            strcpy(msg->strcarno, CarNO + CHINESELEN);
                        }
                        strcpy(msg->strtime, sss.toStdString().c_str());
                        msg->laneno = struITSPlateResult.byDriveChan;
                        strcpy(msg->strprepic, filename);
                        //====================================
                        char filecar[256] = {};
                        sprintf(filecar, "%s%s%s.ini", dir_strCar.toStdString().c_str(),PLATPATH,CarNO);
                        msg->byPlateType = struITSPlateResult.struPlateInfo.byPlateType;
                        msg->byColor = struITSPlateResult.struPlateInfo.byColor;
                        msg->byBright = struITSPlateResult.struPlateInfo.byBright;
                        msg->byLicenseLen = struITSPlateResult.struPlateInfo.byLicenseLen;
                        msg->byEntireBelieve = struITSPlateResult.struPlateInfo.byEntireBelieve;
                        msg->byVehicleType = struITSPlateResult.struVehicleInfo.byVehicleType;
                        msg->byColorDepth = struITSPlateResult.struVehicleInfo.byColorDepth;
                        msg->byColorCar = struITSPlateResult.struVehicleInfo.byColor;
                        msg->wLength = struITSPlateResult.struVehicleInfo.wLength;
                        msg->byDir = struITSPlateResult.byDir;
                        msg->byVehicleLogoRecog = struITSPlateResult.struVehicleInfo.byVehicleLogoRecog;
                        msg->byVehicleType2 = struITSPlateResult.byVehicleType;
                        WritePrivateProfileString("Car", "byPlateType", QString::number(msg->byPlateType).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byColor", QString::number(msg->byColor).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byBright", QString::number(msg->byBright).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byLicenseLen", QString::number(msg->byLicenseLen).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byEntireBelieve", QString::number(msg->byEntireBelieve).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byVehicleType", QString::number(msg->byVehicleType).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byColorDepth", QString::number(msg->byColorDepth).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byColorCar", QString::number(msg->byColorCar).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "wLength", QString::number(msg->wLength).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byDir", QString::number(msg->byDir).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byVehicleLogoRecog", QString::number(msg->byVehicleLogoRecog).toStdString().c_str(), filecar);
                        WritePrivateProfileString("Car", "byVehicleType2", QString::number(msg->byVehicleType2).toStdString().c_str(), filecar);
                        //====================================
                        //if(msg->botherpic==false)
                        {
                            char filenameCe[256] = {};
                            if (m_Socket->m_lSideNum > 0)  //有侧相机
                            {
                                char  szVal[200], strSection[20];
                                sprintf(strSection, "车道%d", msg->laneno);
                                GetPrivateProfileString(strSection, "侧相机", "", szVal, 19, m_Socket->strConfigfile.c_str());
                                int iCeID = atoi(szVal);
                                auto ifind2 = m_Socket->m_maplaneAndDevice.find(iCeID);
                                if (ifind2 != m_Socket->m_maplaneAndDevice.end())
                                {
                                    char tmfile[256] = {};
                                    sprintf(tmfile, "%s%s%d-%s-%s侧.jpg", dir_str.toStdString().c_str(), PLATPATH, struITSPlateResult.byDriveChan, s.toStdString().c_str(), CarNO);
                                    NET_DVR_JPEGPARA jpeginfo;
                                    jpeginfo.wPicQuality = 2;
                                    jpeginfo.wPicSize = 0xff;
                                    if (!NET_DVR_CaptureJPEGPicture(m_Socket->m_lHandleDVR, ifind2->second->lChanlNumCe + m_Socket->m_lStartChnDVR - 1, &jpeginfo, tmfile))
                                       m_Socket->OutputLog2("侧相机NET_DVR_CaptureJPEGPicture error, %d", NET_DVR_GetLastError());
                                    else
                                        strcpy(filenameCe,tmfile);
                                }
                            }
                            char filenameAll[256] = {};
                            if (m_Socket->m_lAllNum > 0)
                            {
                                char tmfile[256] = {};
                                sprintf(tmfile, "%s%s%d-%s-%s全.jpg", dir_str.toStdString().c_str(), PLATPATH, struITSPlateResult.byDriveChan, s.toStdString().c_str(), CarNO);
                                NET_DVR_JPEGPARA jpeginfo;
                                jpeginfo.wPicQuality = 2;
                                jpeginfo.wPicSize = 0xff;
                                if (!NET_DVR_CaptureJPEGPicture(m_Socket->m_lHandleDVR, m_Socket->m_lChannelAll + m_Socket->m_lStartChnDVR - 1, &jpeginfo, tmfile))
                                    m_Socket->OutputLog2("全相机NET_DVR_CaptureJPEGPicture error, %d", NET_DVR_GetLastError());
                                else
                                    strcpy(filenameAll,tmfile);
                            }
                            strcpy(msg->strcepic, filenameCe);
                            strcpy(msg->strallpic, filenameAll);
                            msg->botherpic = true;
                            m_Socket->OutputLog2("other");
                        }
                        msg->bpicpre = true;
                        msg->getticout = ::GetTickCount();
                        msg->bstart = true;
                    }
                    m_Socket->m_QueueSect.Unlock();
                }
                else
                {
                    m_Socket->OutputLog2("%s","后拍照");
                    sprintf(filename, "%s%s%d-%s-%s后.jpg", dir_str.toStdString().c_str(), PLATPATH, struITSPlateResult.byDriveChan, s.toStdString().c_str(), CarNO);
                    //m_Socket->OutputLog2("%s",filename);
                    fSnapPic = fopen(filename, "wb");
                    fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, fSnapPic);
                    fclose(fSnapPic);
                    //入队
                    m_Socket->m_QueueSect.Lock();
                    auto ifind = m_Socket->m_maplaneAndmsg.find(struITSPlateResult.byDriveChan);
                    if (ifind != m_Socket->m_maplaneAndmsg.end())
                    {
                        STR_MSG *msg = ifind->second;
                        strcpy(msg->strafrpic, filename);
                        msg->bpic = true;
                        strcpy(msg->strtime, sss.toStdString().c_str());
                        msg->laneno = struITSPlateResult.byDriveChan;
                        if (strstr(CarNO,"无") == NULL)
                        {
                            strcpy(msg->strplatecolor, CarColor);
                            strcpy(msg->strcarno, CarNO + CHINESELEN);
                        }
                        if(msg->botherpic==false)
                        {
                            char filenameCe[256] = {};
                            if (m_Socket->m_lSideNum > 0)  //有侧相机
                            {
                                char  szVal[200], strSection[20];
                                sprintf(strSection, "车道%d", msg->laneno);
                                GetPrivateProfileString(strSection, "侧相机", "", szVal, 19, m_Socket->strConfigfile.c_str());
                                int iCeID = atoi(szVal);
                                auto ifind2 = m_Socket->m_maplaneAndDevice.find(iCeID);
                                if (ifind2 != m_Socket->m_maplaneAndDevice.end())
                                {
                                    char tmfile[256] = {};
                                    sprintf(tmfile, "%s%s%d-%s-%s侧.jpg", dir_str.toStdString().c_str(), PLATPATH, struITSPlateResult.byDriveChan, s.toStdString().c_str(), CarNO);
                                    NET_DVR_JPEGPARA jpeginfo;
                                    jpeginfo.wPicQuality = 2;
                                    jpeginfo.wPicSize = 0xff;
                                    if (!NET_DVR_CaptureJPEGPicture(m_Socket->m_lHandleDVR, ifind2->second->lChanlNumCe + m_Socket->m_lStartChnDVR - 1, &jpeginfo, tmfile))
                                       m_Socket->OutputLog2("侧相机NET_DVR_CaptureJPEGPicture error, %d", NET_DVR_GetLastError());
                                    else
                                        strcpy(filenameCe,tmfile);
                                }
                            }
                            char filenameAll[256] = {};
                            if (m_Socket->m_lAllNum > 0)
                            {
                                char tmfile[256] = {};
                                sprintf(tmfile, "%s%s%d-%s-%s全.jpg", dir_str.toStdString().c_str(), PLATPATH,struITSPlateResult.byDriveChan, s.toStdString().c_str(), CarNO);
                                NET_DVR_JPEGPARA jpeginfo;
                                jpeginfo.wPicQuality = 2;
                                jpeginfo.wPicSize = 0xff;
                                if (!NET_DVR_CaptureJPEGPicture(m_Socket->m_lHandleDVR, m_Socket->m_lChannelAll + m_Socket->m_lStartChnDVR - 1, &jpeginfo, tmfile))
                                    m_Socket->OutputLog2("全相机NET_DVR_CaptureJPEGPicture error, %d", NET_DVR_GetLastError());
                                else
                                    strcpy(filenameAll,tmfile);
                            }
                            strcpy(msg->strcepic, filenameCe);
                            strcpy(msg->strallpic, filenameAll);
                            msg->botherpic = true;
                            //m_Socket->OutputLog2("other");
                        }
                        msg->getticout = ::GetTickCount();
                        msg->bstart = true;
                    }
                    m_Socket->m_QueueSect.Unlock();
                }
            }
        }
        //其他信息处理......
        break;
    }
    default:
        break;
    }
    return;
}

OverWidget::OverWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverWidget)
{
    ui->setupUi(this);
    g_Over = this;
    m_hMsgEvent = m_hEvent = m_hShutDownEvent = NULL;
    m_iCardNum = 1;
    m_LaneNum=1;
    m_SenorNum=6;
    m_bNeedFlush = m_bStartGetSample = false;
    m_lSideNum = m_GBXZ = m_midlane = 0;
    m_msgQuery =  QSqlQuery(g_DB);
    m_msgQueryRe =  QSqlQuery(g_DB);
    m_videoQuery =  QSqlQuery(g_DB);
    m_PrePort  = m_AfrPort = -1;

//#ifdef WIN32
//    //创建一个VLC实例
//    m_Prevlc_ins = libvlc_new(0, NULL);
//    //创建一个VLC播放器
//    m_Prevlc_player = libvlc_media_player_new(m_Prevlc_ins);
//    //创建一个VLC实例
//    m_vlc_ins = libvlc_new(0, NULL);
//    //创建一个VLC播放器
//    m_vlc_player = libvlc_media_player_new(m_vlc_ins);
//#endif
    bStart = true;
    initializeWinsockIfNecessary();
    this->InitOver();
}

OverWidget::~OverWidget()
{
    if(m_iLedType == 1)
        m_Ledwid->close();
    bStart = false;
    Sleep(200);

//#ifdef WIN32
//    // 释放
//    libvlc_media_player_release(m_vlc_player);
//    // 释放
//    libvlc_release(m_vlc_ins);
//    // 释放
//    libvlc_media_player_release(m_Prevlc_player);
//    // 释放
//    libvlc_release(m_Prevlc_ins);
//#endif

    //释放SDK 资源
    NET_DVR_Cleanup();
    releaseWinsockIfNecessary();

    delete ui;
}

void OverWidget::DrawPicSY(STR_MSG &msg)
{
    char szVal[256] ={};
    //实例QImage
    QPixmap pix = QPixmap(QString::fromLocal8Bit(msg.strprepic));
    if(!pix.isNull())
    {
        QImage image = pix.toImage();
        int w = image.width();
        //实例QPainter
        QPainter painter(&image);
        //设置画刷模式
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        //改变画笔和字体
        QPen pen = painter.pen();
        pen.setColor(Qt::white);
        QFont font = painter.font();
        font.setBold(true);//加粗
        if(w>2000)
            font.setPixelSize(80);//改变字体大小
        else
            font.setPixelSize(30);//改变字体大小
        painter.setPen(pen);
        painter.setFont(font);
        if(w > 2000)
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,500,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,600,QString::fromLocal8Bit(szVal));            
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,700,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,800,QString::fromLocal8Bit(szVal));
        }
        else
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,50,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,150,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,250,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));            
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,350,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
        }
        //保存图片
        image.save(QString::fromLocal8Bit(msg.strprepic),"JPG");
    }

    pix = QPixmap(QString::fromLocal8Bit(msg.strafrpic));
    if(!pix.isNull())
    {
        QImage image = pix.toImage();
        int w = image.width();
        //实例QPainter
        QPainter painter(&image);
        //设置画刷模式
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        //改变画笔和字体
        QPen pen = painter.pen();
        pen.setColor(Qt::white);
        QFont font = painter.font();
        font.setBold(true);//加粗
        if(w>2000)
            font.setPixelSize(80);//改变字体大小
        else
            font.setPixelSize(30);//改变字体大小
        painter.setPen(pen);
        painter.setFont(font);
        if(w > 2000)
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,500,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,600,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,700,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,800,QString::fromLocal8Bit(szVal));
        }
        else
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,50,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,150,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,250,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,350,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
        }
        //保存图片
        image.save(QString::fromLocal8Bit(msg.strafrpic),"JPG");
    }

    pix = QPixmap(QString::fromLocal8Bit(msg.strallpic));
    if(!pix.isNull())
    {
        QImage image = pix.toImage();
        int w = image.width();
        //实例QPainter
        QPainter painter(&image);
        //设置画刷模式
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        //改变画笔和字体
        QPen pen = painter.pen();
        pen.setColor(Qt::white);
        QFont font = painter.font();
        font.setBold(true);//加粗
        if(w>2000)
            font.setPixelSize(80);//改变字体大小
        else
            font.setPixelSize(30);//改变字体大小
        painter.setPen(pen);
        painter.setFont(font);
        if(w > 2000)
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,500,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,600,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,700,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,800,QString::fromLocal8Bit(szVal));
        }
        else
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,50,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,150,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,250,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,350,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
        }
        //保存图片
        image.save(QString::fromLocal8Bit(msg.strallpic),"JPG");
    }

    pix = QPixmap(QString::fromLocal8Bit(msg.strcepic));
    if(!pix.isNull())
    {
        QImage image = pix.toImage();
        int w = image.width();
        //实例QPainter
        QPainter painter(&image);
        //设置画刷模式
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        //改变画笔和字体
        QPen pen = painter.pen();
        pen.setColor(Qt::white);
        QFont font = painter.font();
        font.setBold(true);//加粗
        if(w>2000)
            font.setPixelSize(80);//改变字体大小
        else
            font.setPixelSize(30);//改变字体大小
        painter.setPen(pen);
        painter.setFont(font);
        if(w > 2000)
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,500,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,600,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,700,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,800,QString::fromLocal8Bit(szVal));
        }
        else
        {
            sprintf_s(szVal,"检测时间:%s", msg.strtime);
            painter.drawText(10,50,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车牌号码:%s", msg.strcarno);
            painter.drawText(10,100,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"行车方向:%s", msg.strdirection);
            painter.drawText(10,150,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆速度:%d km/h", msg.speed);
            painter.drawText(10,200,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"车辆轴数:%d 轴", msg.axiscount);
            painter.drawText(10,250,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"是否跨道:%s", msg.strcrossroad);
            painter.drawText(10,300,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"总质量:%d kg", msg.weight);
            painter.drawText(10,350,QString::fromLocal8Bit(szVal));
            sprintf_s(szVal,"结果:%s", msg.strret);
            painter.drawText(10,400,QString::fromLocal8Bit(szVal));
        }
        //保存图片
        image.save(QString::fromLocal8Bit(msg.strcepic),"JPG");
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
    GetPrivateProfileString("SYS", "OwnDevice", "0", szVal, 19, strConfigfile.c_str());
    if(atoi(szVal) == 1)
        StartUnitCom();     //仪表com
    else
        StartServerSocket();    //第三方仪表

    //相机
    StartDVR();
    StartLED();

    pthread_t m_dwHeartID;//线程ID
    pthread_create(&m_dwHeartID, nullptr, &VedioMsgThreadProc, this);
    pthread_t m_dwHeartID8;//线程ID
    pthread_create(&m_dwHeartID8, nullptr, &HisVedioMsgThreadProc, this);
    pthread_t m_dwHeartID2;//线程ID
    pthread_create(&m_dwHeartID2, nullptr, &SendMsgThreadProc, this);
    pthread_t m_dwHeartID3;//线程ID
    pthread_create(&m_dwHeartID3, nullptr, &UploadMsgThreadProc, this);
    pthread_t m_dwHeartID7;//线程ID
    pthread_create(&m_dwHeartID7, nullptr, &ReUploadMsgThreadProc, this);
#ifdef WIN32
    pthread_t m_dwHeartID4;//线程ID
    pthread_create(&m_dwHeartID4, nullptr, &UploadVideoThreadProc, this);
#endif
    pthread_t m_dwHeartID5;//线程ID
    pthread_create(&m_dwHeartID5, nullptr, &UploadDevStateThreadProc, this);
    pthread_t m_dwHeartID6;//线程ID
    pthread_create(&m_dwHeartID6, nullptr, &CheckStoreThreadProc, this);

    //崩溃前的待下载录像
    QString strQuery = "SELECT * FROM Task WHERE bUploadvedio = 0 order by id desc;";
    QSqlQuery query(g_DB);
    query.prepare(strQuery);
    if (!query.exec())
    {
        QString querys = query.lastError().text();
        OutputLog2("re车辆待录像记录查询失败 %s",querys.toStdString().c_str());
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
            while (query.next())
            {
                STR_MSG nPermsg;
                nPermsg.dataId = query.value("id").toString().toInt();
                nPermsg.laneno = query.value("laneno").toString().toInt();
                strcpy(nPermsg.strcarno,query.value("CarNO").toString().toLocal8Bit());
                strcpy(nPermsg.strtime,query.value("DetectionTime").toString().toLocal8Bit());
                strcpy(nPermsg.strcartype,query.value("CarType").toString().toLocal8Bit());
                strcpy(nPermsg.strret,query.value("DetectionResult").toString().toLocal8Bit());
                nPermsg.speed = query.value("Speed").toString().toInt();
                nPermsg.axiscount = query.value("Axles").toString().toInt();
                nPermsg.weight = query.value("Weight").toString().toInt();
                nPermsg.getticout = ::GetTickCount();
                //录像
                m_QueueSectVedioHis.Lock();
                m_MsgVedioQueueHis.push(nPermsg);
                m_QueueSectVedioHis.Unlock();
                OutputLog2("启动录像111 %s His size=%d", nPermsg.strcarno, m_MsgVedioQueueHis.size());
            }
        }
    }
}

int OverWidget::StartServerSocket()
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET)
    {
        OutputLog2("Error  at  socket()");
        releaseWinsockIfNecessary();
        return  -1;
    }

    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr(sServerIP); //设置IP
    service.sin_port = htons(nSocketPort);			//端口号

    int opt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    if (::bind(m_socket, (struct sockaddr*)&service, sizeof(service)) == SOCKET_ERROR)
    {
        OutputLog2("bind()  failed");
        ::closeSocket(m_socket);
        releaseWinsockIfNecessary();
        return  -1;
    }

    if (listen(m_socket, 20) == SOCKET_ERROR)
    {
        OutputLog2("Error  listening  on  socket");
        releaseWinsockIfNecessary();
        return -1;
    }

    //启动监听线程
    pthread_t m_dwHeartID0;//线程ID
    pthread_create(&m_dwHeartID0, nullptr, &CSocketThread, this);

    OutputLog2("开启服务端");
    return 1;
}

int OverWidget::StopServerSocket()
{

    return 1;
}

void* CClientThread(LPVOID pParam)
{
    ClientContext *Client = (ClientContext*)pParam;
    OverWidget * m_Socket = (OverWidget *)Client->pthis;
    char revData[10 * 1024] = {};
    int nLenth = 0, nGetLenth = 0;

    //进入消息读取
    while (m_Socket->bStart)
    {
        memset(revData, 0x00, 10 * 1024);
        int bytesRecv = recv(Client->s, revData, 1024 * 10, 0);
        if (bytesRecv == SOCKET_ERROR)
        {
            m_Socket->OutputLog2("网络断开2");
            break;
        }
        else if (bytesRecv == 0)
        {
            m_Socket->OutputLog2("网络断开");
            break;
        }
        else		//接收到数据开始分析
        {
            /////////////////////////////////////////////////////////////////////////////////////////
            if (!Client->m_bNeedRecv)//第一次获取到的数据
            {
                nGetLenth = bytesRecv;
                m_Socket->OutputLog2("已经接收数据的长[%d]", bytesRecv);

                int value = (int)((revData[0] & 0xff) | ((revData[1] & 0xff) << 8) | ((revData[2] & 0xff) << 16) | ((revData[3] & 0xff) << 24));
                byte flag = (byte)(value >> 22);//得到包头标识，验证是否为0xff
                if (flag != 0xff)
                {
                    m_Socket->OutputLog2("包头error");
                    continue;
                }
                nLenth = value & 0xff;
                //注:得到包体长度，从缓存中读取nLenth长度的字节数据即为数据包
                m_Socket->OutputLog2("得到包体长度[%d]", nLenth);

                for (int i = 0; i < nGetLenth; i++)
                {
                    Client->m_Buffer[i] = revData[i];
                }
                if (nGetLenth >= nLenth +4) //可以确认已经完毕
                {
                    m_Socket->AnalysisCarResult(Client->m_Buffer, nLenth);
                    Client->m_bNeedRecv = FALSE;
                    memset(Client->m_Buffer, 0, MAXBUFFSIZE);
                }
                else  //没完毕 复制到缓冲区 继续接收
                {
                    Client->m_bNeedRecv = TRUE;
                    m_Socket->OutputLog2("接收数据未到位.继续等待接收");
                }
            }
            else
            {
                //判断下当前数据是否接收完毕
                for (int i = 0; i < bytesRecv; i++)
                {
                    Client->m_Buffer[i + nGetLenth] = revData[i];
                }
                nGetLenth += bytesRecv;
                m_Socket->OutputLog2("又接收到数据帧长[%d]", bytesRecv);
                if (nGetLenth >= nLenth+4)//可以确认已经完毕
                {
                    m_Socket->AnalysisCarResult(Client->m_Buffer, nLenth);
                    Client->m_bNeedRecv = FALSE;
                    memset(Client->m_Buffer, 0, MAXBUFFSIZE);
                }
            }
        }
    }

    m_Socket->deleteSocketinfo(Client->s);
    m_Socket->OutputLog2("触发 停止信号 CClientThread线程结束,size=%d ", m_Socket->m_clientGroup.size());
    return 0;
}

void* CSocketThread(LPVOID pParam)
{
    OverWidget * m_Socket = (OverWidget *)pParam;
    while (m_Socket->bStart)
    {
        sockaddr_in addrClient = {};// 连接上的客户端ip地址
        socklen_t len = sizeof(addrClient);
        SOCKET sockClient = accept(m_Socket->m_socket, (struct sockaddr*)&addrClient, &len);
        if (sockClient == INVALID_SOCKET)
        {
            continue;
        }
        ClientContext *Client = new ClientContext();
        if (Client == NULL)
        {
            closeSocket(sockClient);
            continue;
        }
        //记录
        Client->s = sockClient;
        Client->pthis = pParam;
        strcpy(Client->m_chIP, inet_ntoa(addrClient.sin_addr));
        m_Socket->m_csSend.Lock();
        m_Socket->m_clientGroup.push_back(Client);
        m_Socket->m_csSend.Unlock();

        char  msg[250] = {};
        sprintf(msg, "%s连接成功, size=%d ", Client->m_chIP, m_Socket->m_clientGroup.size());
        m_Socket->OutputLog2(msg);

        pthread_t m_dwHeartID;//线程ID
        pthread_create(&m_dwHeartID, nullptr, &CClientThread, Client);
    }

    m_Socket->OutputLog2("触发 停止信号 CSocketThread线程结束");
    return 0;
}

int OverWidget::StartDVR()
{
    char  szVal[200], strSection[20];
    GetPrivateProfileString("SYS", "IsCam", "", szVal, 29, strConfigfile.c_str());
    if(atoi(szVal) == 0)
        return 0;

    // 设备连接
    OutputLog2("开始连接摄像机...");
    int i=0;
    //DVR
    {
        DeviceNode *node = new DeviceNode;
        memset(node,0,sizeof(DeviceNode));
        node->Type = 2;
        //login
        sprintf(strSection, "DVR");
        const char *strKey = "ip";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        strcpy(node->sIP,szVal);

        strKey = "port";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        node->iPort = atoi(szVal);

        strKey = "user";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        strcpy(node->user,szVal);

        strKey = "pwd";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        strcpy(node->pwd,szVal);

        strKey = "设备ID";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        strcpy(node->sDvrNo,szVal);
        strcpy(node->sname,"硬盘录像机");
        NET_DVR_DEVICEINFO_V30 struDeviceInfo = { 0 };
        m_lHandleDVR = NET_DVR_Login_V30(node->sIP, node->iPort, node->user, node->pwd, &struDeviceInfo);
        if (m_lHandleDVR < 0)
        {
            OutputLog2("DVR Login error, %d", NET_DVR_GetLastError());
        }
        else
        {
            node->bIsOnline = NORMAL;
            m_lStartChnDVR = struDeviceInfo.byStartDChan;
            OutputLog2("DVR起始通道 %d", m_lStartChnDVR);
        }
        node->LoginID = m_lHandleDVR;
        lstDevice.push_back(node);
    }
    GetPrivateProfileString("SYS", "中车道", "2", szVal, 29, strConfigfile.c_str());
    m_midlane = atoi(szVal);
    GetPrivateProfileString("SYS", "侧相机数量", "2", szVal, 29, strConfigfile.c_str());
    m_lSideNum = atoi(szVal);
    for (i = 1; i <= m_lSideNum; i++)
    {
        STR_Device *pMsgDevice = new STR_Device;
        memset(pMsgDevice, 0, sizeof(STR_Device));

        sprintf(strSection, "侧相机%d", i);
        const char *strKey = "channel";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        pMsgDevice->lChanlNumCe = atoi(szVal);

        strKey = "设备ID";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        int ID = atoi(szVal);
        m_maplaneAndDevice[ID] = pMsgDevice;

        OutputLog2("侧相机%d 通道%d", ID,pMsgDevice->lChanlNumCe);
    }

    GetPrivateProfileString("SYS", "全景相机数量", "0", szVal, 29, strConfigfile.c_str());
    m_lAllNum = atoi(szVal);
    if (m_lAllNum > 0)
    {
        //login
        sprintf(strSection, "全景相机");
        const char *strKey = "channel";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        m_lChannelAll = atoi(szVal);
        OutputLog2("全景相机通道 %d", m_lChannelAll);
    }

    for (i=1;i<=m_PreNum;i++)
    {
        STR_Device *pMsgDevice = new STR_Device;
        memset(pMsgDevice, 0, sizeof(STR_Device));
        sprintf(strSection, "前相机%d", i);
        //注册设备
        DeviceNode *node = new DeviceNode;
        memset(node,0,sizeof(DeviceNode));
        node->Type = 3;

        const char *strKey = "ip";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        strcpy(node->sIP,szVal);

        strKey = "port";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        node->iPort = atoi(szVal);

        strKey = "user";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        strcpy(node->user,szVal);

        strKey = "pwd";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        strcpy(node->pwd,szVal);

        strKey = "channel";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        pMsgDevice->lChanlNumPre = atoi(szVal);

        strKey = "设备ID";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        strcpy(node->sDvrNo,szVal);
        node->ilane = atoi(szVal);
        sprintf(node->sname, "%s", strSection);

        //登录参数，包括设备地址、登录用户、密码等
        NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
        struLoginInfo.bUseAsynLogin = 0;                    //同步登录方式
        strcpy(struLoginInfo.sDeviceAddress, node->sIP);    //设备IP地址
        struLoginInfo.wPort = node->iPort;                  //设备服务端口
        strcpy(struLoginInfo.sUserName, node->user);        //设备登录用户名
        strcpy(struLoginInfo.sPassword, node->pwd);         //设备登录密码
        //设备信息, 输出参数
        NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
        pMsgDevice->lUerIDPre = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
        //        NET_DVR_DEVICEINFO_V30 struDeviceInfo;
        //        pMsgDevice->lUerIDPre = NET_DVR_Login_V30(node->sIP, node->iPort, node->user, node->pwd, &struDeviceInfo);
        if (pMsgDevice->lUerIDPre < 0)
        {
            OutputLog2("%s Login error, %d", strSection, NET_DVR_GetLastError());
        }
        else
        {
            OutputLog2("code %d", struDeviceInfoV40.byCharEncodeType);
            node->bIsOnline = NORMAL;
            //设置报警回调函数
            NET_DVR_SetDVRMessageCallBack_V30(MSesGCallback, this);
            //启用布防
            NET_DVR_SETUPALARM_PARAM struSetupParam = { 0 };
            struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
            struSetupParam.byAlarmInfoType = 1;
            struSetupParam.byLevel = 1;  //级别
            pMsgDevice->lHandlePre = NET_DVR_SetupAlarmChan_V41(pMsgDevice->lUerIDPre, &struSetupParam);
            if (pMsgDevice->lHandlePre < 0)
            {
                OutputLog2("%s NET_DVR_SetupAlarmChan_V41 error1, %d", strSection, NET_DVR_GetLastError());
                Sleep(1000);
                pMsgDevice->lHandlePre = NET_DVR_SetupAlarmChan_V41(pMsgDevice->lUerIDPre, &struSetupParam);
                if (pMsgDevice->lHandlePre < 0)
                {
                    OutputLog2("%s NET_DVR_SetupAlarmChan_V41 error2, %d", strSection, NET_DVR_GetLastError());
                    Sleep(1000);
                    pMsgDevice->lHandlePre = NET_DVR_SetupAlarmChan_V41(pMsgDevice->lUerIDPre, &struSetupParam);
                    if (pMsgDevice->lHandlePre < 0)
                    {
                        OutputLog2("%s NET_DVR_SetupAlarmChan_V41 error3, %d", strSection, NET_DVR_GetLastError());
                        NET_DVR_Logout(pMsgDevice->lUerIDPre);
                    }
                }
            }
        }
        node->LoginID = pMsgDevice->lUerIDPre;
        node->ID = i;
        lstDevice.push_back(node);
        m_maplaneAndDevice[node->ilane] = pMsgDevice;
    }

    for (i=1;i<=m_AfrNum;i++)
    {
        STR_Device *pMsgDevice = new STR_Device;
        memset(pMsgDevice, 0, sizeof(STR_Device));
        sprintf(strSection, "后相机%d", i);
        //注册设备
        DeviceNode *node = new DeviceNode;
        memset(node,0,sizeof(DeviceNode));
        node->Type = 4;

        const char *strKey = "ip";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        strcpy(node->sIP,szVal);

        strKey = "port";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        node->iPort = atoi(szVal);

        strKey = "user";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        strcpy(node->user,szVal);

        strKey = "pwd";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        strcpy(node->pwd,szVal);

        strKey = "channel";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        pMsgDevice->lChanlNumAfr = atoi(szVal);

        strKey = "设备ID";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        strcpy(node->sDvrNo,szVal);
        node->ilane = atoi(szVal);
        sprintf(node->sname, "%s", strSection);

        NET_DVR_DEVICEINFO_V30 struDeviceInfo;
        pMsgDevice->lUerIDAfr = NET_DVR_Login_V30(node->sIP, node->iPort, node->user, node->pwd, &struDeviceInfo);
        if (pMsgDevice->lUerIDAfr < 0)
        {
            OutputLog2("%s Login error, %d", strSection, NET_DVR_GetLastError());
        }
        else
        {
            node->bIsOnline = NORMAL;
            //设置报警回调函数
            NET_DVR_SetDVRMessageCallBack_V30(MSesGCallback, this);
            //启用布防
            NET_DVR_SETUPALARM_PARAM struSetupParam = { 0 };
            struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
            struSetupParam.byAlarmInfoType = 1;
            struSetupParam.byLevel = 1;  //级别
            pMsgDevice->lHandleAfr = NET_DVR_SetupAlarmChan_V41(pMsgDevice->lUerIDAfr, &struSetupParam);
            if (pMsgDevice->lHandleAfr < 0)
            {
                OutputLog2("%s NET_DVR_SetupAlarmChan_V41 error1, %d", strSection, NET_DVR_GetLastError());
                Sleep(1000);
                pMsgDevice->lHandleAfr = NET_DVR_SetupAlarmChan_V41(pMsgDevice->lUerIDAfr, &struSetupParam);
                if (pMsgDevice->lHandleAfr < 0)
                {
                    OutputLog2("%s NET_DVR_SetupAlarmChan_V41 error2, %d", strSection, NET_DVR_GetLastError());
                    Sleep(1000);
                    pMsgDevice->lHandleAfr = NET_DVR_SetupAlarmChan_V41(pMsgDevice->lUerIDAfr, &struSetupParam);
                    if (pMsgDevice->lHandleAfr < 0)
                    {
                        OutputLog2("%s NET_DVR_SetupAlarmChan_V41 error3, %d", strSection, NET_DVR_GetLastError());
                        NET_DVR_Logout(pMsgDevice->lUerIDAfr);
                    }
                }
            }
        }
        node->LoginID = pMsgDevice->lUerIDAfr;
        node->ID = i;
        lstDevice.push_back(node);
        m_maplaneAndDevice[node->ilane] = pMsgDevice;
    }

    for (i=1;i<=m_LaneNum;i++)
    {
        STR_MSG *pMsg = new STR_MSG;
        memset(pMsg, 0, sizeof(STR_MSG));
        m_maplaneAndmsg[i] = pMsg;
    }

    OutputLog2("m_maplaneAndmsg size = %d", m_maplaneAndmsg.size());
    OutputLog2("m_maplaneAndCam size = %d", m_maplaneAndDevice.size());

    pthread_t m_dwHeartID;//线程ID
    pthread_create(&m_dwHeartID, nullptr, &ReConnectDVR, this);
    pthread_t m_dwHeartID2;//线程ID
    pthread_create(&m_dwHeartID2, nullptr, &ThreadDetectHK, this);


    return 1;
}

int OverWidget::StartUnitCom()
{   
#ifdef WIN32
    //算法
    Init();
    SetCallBack(g_pArrival,g_ppLeave,g_ppResult);
    PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("./log4cplus.cfg"));
    pTestLogger = Logger::getRoot();
    LOG4CPLUS_ERROR(pTestLogger, "打开...");
#endif

    //连仪表
    char  szVal[200], strSection[20];
    int i, iNum = 0;
    GetPrivateProfileString("SYS", "仪表数量", "1", szVal, 29, strConfigfile.c_str());
    iNum = atoi(szVal);
    for (i = 0; i < iNum; i++)
    {
        Device_MSG *pMsg = new Device_MSG;
        sprintf(strSection, "仪表%d", i+1);
        const char *strKey = "设备ID";
        GetPrivateProfileString(strSection, strKey, "", szVal, 29, strConfigfile.c_str());
        strcpy(pMsg->deviceid,szVal);

        strKey = "ip";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        strcpy(pMsg->ip,szVal);

        strKey = "port";
        GetPrivateProfileString(strSection, strKey, "", szVal, 19, strConfigfile.c_str());
        pMsg->port = atoi(szVal);

        m_mapComAndID[i] = pMsg;
        m_comwraper[i] = new ComContext;
        //
        int ii, iNum = 0;
        char temp[99], temp2[20];
        ::GetPrivateProfileString(strSection, "通道使用数量", "0", temp, 5, strConfigfile.c_str());
        iNum = atoi(temp);
        for (ii = 1; ii <= iNum; ii++)
        {
            sprintf(temp2, "%s,%d",pMsg->deviceid, ii);
            ::GetPrivateProfileString(strSection, temp2, "", temp, 90, strConfigfile.c_str());
            QString str(temp);
            QStringList strdatalist = str.split(",");
            if(strdatalist.size() < 2)
                continue;
            DataNode *node = new DataNode;
            node->ilane = strdatalist.at(0).toInt();   //车道
            node->iSensor = strdatalist.at(1).toInt();  //传感器
            m_mapComAndDeviceID[temp2] = node;
        }

        pthread_t m_dwHeartID;//线程ID
        pthread_create(&m_dwHeartID, nullptr, &ChnlDataThreadProc, (void*)i);

        pthread_t m_dwHeartID2;//线程ID
        pthread_create(&m_dwHeartID2, nullptr, &ParseDataThreadProc, (void*)i);

    }
    OutputLog2("m_mapComAndDeviceID size = %d", m_mapComAndDeviceID.size());

    return 1;
}

int OverWidget::AnalysisCarResult(char *ResponeValue, int nLenthResponeValue)
{
    const char *getdata = "getdata";
    std::string str = ResponeValue + 4;
    OutputLog2("%s", str.c_str());
    if (str.find("ready") != std::string::npos)
    {
        OutputLog2("设备就绪!");
        Send((char*)getdata, 7);
    }
    else if (str.find("getdata") != std::string::npos)
    {
        std::string strdata = str.substr(10, nLenthResponeValue - 12);
        std::vector<std::string> strResult;
        Split(strResult, strdata, ",");
        if (strResult.size() < 1)
            return 1;

        int iLane = atoi(strResult.at(4).c_str());
        OutputLog2("接收车道%d称重数据: ", iLane);
        //出队
        m_QueueSect.Lock();
        auto ifind = m_maplaneAndmsg.find(iLane);
        if (ifind != m_maplaneAndmsg.end())
        {
            STR_MSG *msgdata = ifind->second;
            msgdata->axiscount = atoi(strResult.at(7).c_str());
            msgdata->laneno = atoi(strResult.at(4).c_str());
            strcpy(msgdata->strweighttime, strResult.at(3).c_str());
            strcpy(msgdata->deviceNo, strResult.at(46).c_str());
            msgdata->speed = atoi(strResult.at(11).c_str());
            msgdata->credible = atoi(strResult.at(22).c_str());
            msgdata->weight = atoi(strResult.at(20).c_str());
            msgdata->carlen = atoi(strResult.at(14).c_str());
            msgdata->Axis_weight[0] = atoi(strResult.at(29).c_str());
            msgdata->Axis_weight[1] = atoi(strResult.at(30).c_str());
            msgdata->Axis_weight[2] = atoi(strResult.at(31).c_str());
            msgdata->Axis_weight[3] = atoi(strResult.at(32).c_str());
            msgdata->Axis_weight[4] = atoi(strResult.at(33).c_str());
            msgdata->Axis_weight[5] = atoi(strResult.at(34).c_str());
            msgdata->Axis_weight[6] = atoi(strResult.at(35).c_str());
            msgdata->Axis_weight[7] = atoi(strResult.at(36).c_str());
            msgdata->Axis_space[0] = atoi(strResult.at(37).c_str());
            msgdata->Axis_space[1] = atoi(strResult.at(38).c_str());
            msgdata->Axis_space[2] = atoi(strResult.at(39).c_str());
            msgdata->Axis_space[3] = atoi(strResult.at(40).c_str());
            msgdata->Axis_space[4] = atoi(strResult.at(41).c_str());
            msgdata->Axis_space[5] = atoi(strResult.at(42).c_str());
            msgdata->Axis_space[6] = atoi(strResult.at(43).c_str());
            std::string sType = strResult.at(6);
            if (strcmp(sType.c_str(),"A") == 0)
                strcpy(msgdata->strcartype, "小型客车");
            else if (strcmp(sType.c_str(), "B") == 0)
                strcpy(msgdata->strcartype, "小型货车");
            else if (strcmp(sType.c_str(), "C") == 0)
                strcpy(msgdata->strcartype, "中型货车");
            else if (strcmp(sType.c_str(), "D") == 0)
                strcpy(msgdata->strcartype, "大型客车");
            else if (strcmp(sType.c_str(), "E") == 0)
                strcpy(msgdata->strcartype, "大型货车");
            else if (strcmp(sType.c_str(), "F") == 0)
                strcpy(msgdata->strcartype, "特大货车");

            int iType = atoi(strResult.at(8).c_str());
            if (iType == 0)
                strcpy(msgdata->strdirection, "正向");
            else if (iType == 1)
                strcpy(msgdata->strdirection, "逆向");

            iType = atoi(strResult.at(9).c_str());
            if (iType == 0)
                strcpy(msgdata->strcrossroad, "正常");
            else if (iType == 1)
                strcpy(msgdata->strcrossroad, "跨道");

            msgdata->bweight = true;
            if (msgdata->weight > m_GBXZ)
            {
                strcpy(msgdata->strret, "不合格");
                msgdata->overWeight=msgdata->weight - m_GBXZ;
                msgdata->overPercent=(msgdata->overWeight/m_GBXZ)*100.0f;
            }
            else
                strcpy(msgdata->strret, "合格");
            {
                msgdata->getticout = ::GetTickCount();
                msgdata->bstart = true;
            }
            OutputLog2("车道%d 轴数%d 车型%s 速度%d 重量%d 方向%s %s行驶  置信度%d   =%s",
                       msgdata->laneno, msgdata->axiscount, msgdata->strcartype, msgdata->speed,
                       msgdata->weight, msgdata->strdirection, msgdata->strcrossroad, msgdata->credible, msgdata->strret);
        }
        m_QueueSect.Unlock();
    }

    //Send(ResponeValue+4,nLenthResponeValue);
    return 1;
}

int OverWidget::AnalysisCarResult(RESUTL *ret)
{
    int iLane = ret->iLine;
//    {
//        OutputLog2("before lane%d", iLane);
//        if(iLane < 2)
//            iLane = 2;
//        else
//            iLane = 1;
//        OutputLog2("after lane%d", iLane);
//    }
    //出队
    m_QueueSect.Lock();
    auto ifind = m_maplaneAndmsg.find(iLane);
    if (ifind != m_maplaneAndmsg.end())
    {
        STR_MSG *msgdata = ifind->second;
        msgdata->axiscount = ret->iAxleNum;
        msgdata->laneno = iLane;
        msgdata->speed = ret->iSpeed;
        msgdata->weight = ret->iVehicleTotalWeight;
        msgdata->Axis_weight[0] = ret->dAxleLMeanWeight[0] + ret->dAxleRMeanWeight[0];
        msgdata->Axis_weight[1] = ret->dAxleLMeanWeight[1] + ret->dAxleRMeanWeight[1];
        msgdata->Axis_weight[2] = ret->dAxleLMeanWeight[2] + ret->dAxleRMeanWeight[2];
        msgdata->Axis_weight[3] = ret->dAxleLMeanWeight[3] + ret->dAxleRMeanWeight[3];
        msgdata->Axis_weight[4] = ret->dAxleLMeanWeight[4] + ret->dAxleRMeanWeight[4];
        msgdata->Axis_weight[5] = ret->dAxleLMeanWeight[5] + ret->dAxleRMeanWeight[5];
        msgdata->Axis_weight[6] = ret->dAxleLMeanWeight[6] + ret->dAxleRMeanWeight[6];
        msgdata->Axis_weight[7] = 0;
        msgdata->Axis_space[0] = ret->iAxleDistance[0];
        msgdata->Axis_space[1] = ret->iAxleDistance[1];
        msgdata->Axis_space[2] = ret->iAxleDistance[2];
        msgdata->Axis_space[3] = ret->iAxleDistance[3];
        msgdata->Axis_space[4] = ret->iAxleDistance[4];
        msgdata->Axis_space[5] = ret->iAxleDistance[5];
        msgdata->Axis_space[6] = ret->iAxleDistance[6];
        char  szVal[50]={};
        GetPrivateProfileString("仪表1", "设备ID", "", szVal, 49, strConfigfile.c_str());
        strcpy(msgdata->deviceNo, szVal);

        int iType = ret->iDirrection;
        if (iType == 0)
            strcpy(msgdata->strdirection, "正向");
        else if (iType == 1)
            strcpy(msgdata->strdirection, "逆向");

        iType = ret->iVehicleType;
        if (iType == 0)
            strcpy(msgdata->strcartype, "小型客车");
        else if (iType == 1)
            strcpy(msgdata->strcartype, "小型货车");
        else if (iType == 2)
            strcpy(msgdata->strcartype, "中型货车");
        else if (iType == 3)
            strcpy(msgdata->strcartype, "大型客车");
        else if (iType == 4)
            strcpy(msgdata->strcartype, "大型货车");
        else if (iType == 5)
            strcpy(msgdata->strcartype, "特大货车");

        iType = ret->iCrossRoad;
        if (iType == 0)
            strcpy(msgdata->strcrossroad, "正常");
        else if (iType == 1)
            strcpy(msgdata->strcrossroad, "跨道");

        msgdata->bweight = true;

        if(0)
        {
            if (msgdata->weight > m_GBXZ)
            {
                strcpy(msgdata->strret, "不合格");
                msgdata->overWeight=msgdata->weight - m_GBXZ;
                msgdata->overPercent=(msgdata->overWeight/m_GBXZ)*100.0f;
            }
            else
                strcpy(msgdata->strret, "合格");
        }
        else
        {
            int GBxz = 18000;
            if(msgdata->axiscount == 6)
                GBxz = 49000;
            else if(msgdata->axiscount == 5)
                GBxz = 43000;
            else if(msgdata->axiscount == 4)
            {
                GBxz = 36000;
                if(msgdata->Axis_space[0]<215 && msgdata->Axis_space[2]<145)
                    GBxz = 31000;
            }
            else if(msgdata->axiscount == 3)
            {
                GBxz = 27000;
                if(msgdata->Axis_space[0]<215 || msgdata->Axis_space[1]<140)
                    GBxz = 25000;
            }
            msgdata->GBxz = GBxz;
            if (msgdata->weight > GBxz)
            {
                strcpy(msgdata->strret, "不合格");
                msgdata->overWeight=msgdata->weight - GBxz;
                msgdata->overPercent=(msgdata->overWeight/GBxz)*100.0f;
            }
            else
                strcpy(msgdata->strret, "合格");
        }

        msgdata->credible = ret->iCredible;
        msgdata->carlen = ret->iVehicleLen;
        msgdata->getticout = ::GetTickCount();
        msgdata->bstart = true;
        OutputLog2("车道%d 轴数%d 车型%s 速度%d 重量%d 方向%s %s行驶  置信度%d   =%s",
                   msgdata->laneno, msgdata->axiscount, msgdata->strcartype, msgdata->speed,
                   msgdata->weight, msgdata->strdirection, msgdata->strcrossroad, msgdata->credible, msgdata->strret);
    }
    else
        OutputLog2("车道%d 轴数%d 速度%d 重量%d",iLane, ret->iAxleNum, ret->iSpeed,ret->iVehicleTotalWeight);

    m_QueueSect.Unlock();

    return 1;
}

int OverWidget::Send(char *ResponeValue, int nLenthResponeValue)
{
    int value = (0xff << 22) | nLenthResponeValue;
    m_sWriteBuffer[3] = ((value >> 24) & 0xff);
    m_sWriteBuffer[2] = ((value >> 16) & 0xff);
    m_sWriteBuffer[1] = ((value >> 8) & 0xff);//高8位
    m_sWriteBuffer[0] = (value & 0xff);//低位
    for (int i = 0; i < nLenthResponeValue; i++)
        m_sWriteBuffer[i + 4] = ResponeValue[i];

    for (auto &it : m_clientGroup)
    {
        send(it->s, (char*)m_sWriteBuffer, nLenthResponeValue + 4, 0);
    }

    return 1;
}

void OverWidget::deleteSocketinfo(SOCKET socketid)
{
    m_csSend.Lock();
    std::list <ClientContext*>::iterator c1_Iter;
    for (c1_Iter = m_clientGroup.begin(); c1_Iter != m_clientGroup.end();)
    {
        if ((*c1_Iter)->s == socketid)
        {
            char  msg[250] = {};
            sprintf(msg, "%s断开连接", (*c1_Iter)->m_chIP);
            OutputLog2(msg);
            ::closeSocket((*c1_Iter)->s);
            delete (*c1_Iter);
            c1_Iter = m_clientGroup.erase(c1_Iter);
        }
        else
            c1_Iter++;
    }
    m_csSend.Unlock();
}

void OverWidget::InitOver()
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAutoFillBackground(true);
    this->setGeometry(QApplication::desktop()->availableGeometry());
//    this->setFixedSize(1024,768);

    //背景图
    QPalette palette;
    QPixmap pixmap(":/pnd/png/bk.png");
    palette.setBrush(QPalette::Window, QBrush(pixmap));
    setPalette(palette);

    //设置图标底色和背景色一样
    QPalette myPalette;
    QColor myColor(0, 0, 0);
    myColor.setAlphaF(0);
    myPalette.setBrush(backgroundRole(), myColor);

    QFile file(":/qss/widget.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    this->setStyleSheet(styleSheet);
    file.close();

    char szVal[100];
    std::string sPath = QCoreApplication::applicationDirPath().toStdString();
    strConfigfile = sPath + "/config.ini";
    GetPrivateProfileString("SYS", "GB", "", szVal, 100, strConfigfile.c_str());
    m_GBXZ = atoi(szVal);
    ::GetPrivateProfileString("SYS", "TimeOut", "", szVal,100,strConfigfile.c_str());
    m_Timeout = atoi(szVal);
    ::GetPrivateProfileString("SYS", "TimeHeart", "", szVal,100,strConfigfile.c_str());
    m_TimeHeart = atoi(szVal);
    ::GetPrivateProfileString("SYS", "屏幕复位", "", szVal, 100,strConfigfile.c_str());
    m_TimeFlush = atoi(szVal);
    ::GetPrivateProfileString("SYS", "车道数量","", szVal,100,strConfigfile.c_str());
    m_LaneNum = atoi(szVal);
    ::GetPrivateProfileString("SYS", "单车道传感器数量", "", szVal, 100,strConfigfile.c_str());
    m_SenorNum = atoi(szVal);
    ::GetPrivateProfileString("SYS", "站点ID", "", szVal,100, strConfigfile.c_str());
    m_stationId = atoi(szVal);
    ::GetPrivateProfileString("SYS", "前相机数量", "", szVal, 100,strConfigfile.c_str());
    m_PreNum = atoi(szVal);
    ::GetPrivateProfileString("SYS", "后相机数量", "", szVal,100, strConfigfile.c_str());
    m_AfrNum = atoi(szVal);
    ::GetPrivateProfileString("SYS", "联网方式", "0", szVal,100, strConfigfile.c_str());
    m_netType = atoi(szVal);
    ::GetPrivateProfileString("SYS", "GOV", "1", szVal,100, strConfigfile.c_str());
    m_govType = atoi(szVal);
    ::GetPrivateProfileString("SYS", "loglane", "1", szVal,100, strConfigfile.c_str());
    loglane = atoi(szVal);

    GetPrivateProfileString("WEB", "ServerIP", "127.0.0.1", m_szServerIp, sizeof(m_szServerIp), strConfigfile.c_str());
    ::GetPrivateProfileString("WEB", "ServerPort", "5650", szVal,100, strConfigfile.c_str());
    m_nPort = atoi(szVal);
    GetPrivateProfileString("WEB", "user", "111", m_webuser, sizeof(m_webuser), strConfigfile.c_str());
    GetPrivateProfileString("WEB", "xlnum", "222", m_webxlnum, sizeof(m_webxlnum), strConfigfile.c_str());
    GetPrivateProfileString("WEB", "ip", "127.0.0.1", m_webip, sizeof(m_webip), strConfigfile.c_str());
    ::GetPrivateProfileString("WEB", "port", "5555", szVal,100, strConfigfile.c_str());
    m_webport = atoi(szVal);
    GetPrivateProfileString("GOV", "indexCode", "111", m_indexCode, sizeof(m_indexCode), strConfigfile.c_str());
    GetPrivateProfileString("GOV", "checkId", "222", m_checkId, sizeof(m_checkId), strConfigfile.c_str());
    GetPrivateProfileString("GOV", "authorizeId", "333", m_authorizeId, sizeof(m_authorizeId), strConfigfile.c_str());
    //设置连接时间与重连时间
    NET_DVR_Init();
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(10000, true);
    //socket
    nSocketPort = 13999;
    m_sWriteBuffer = new unsigned char[1024];
    memset(m_sWriteBuffer, 0x00, sizeof(m_sWriteBuffer));
    //主界面
    initTitle();
    initReal();
    initHis();
    initLog();
    initState();
    initStatistics();
    initRecord();
    m_bottomlay = new QHBoxLayout;
    m_bottomlay->addWidget(m_realwid);
    m_bottomlay->addWidget(m_hiswid);
    m_bottomlay->addWidget(m_Statisticswid);
    m_bottomlay->addWidget(m_statewid);
    m_bottomlay->addWidget(m_logwid);
    m_bottomlay->addWidget(m_recordshowwid);
    m_bottomlay->setSpacing(0);
    m_hiswid->hide();
    m_Statisticswid->hide();
    m_statewid->hide();
    m_logwid->hide();
    m_recordshowwid->hide();
    m_realwid->show();
    m_mainlayout = new QVBoxLayout(this);
    m_mainlayout->addWidget(m_titlewid);
    m_mainlayout->addLayout(m_dhtoplay);
    m_mainlayout->addLayout(m_bottomlay);
    m_mainlayout->setSpacing(5);
    m_mainlayout->setMargin(10);
    QObject::connect(this, SIGNAL(FlushSig(QString)), this, SLOT(Flush(QString)));
    QObject::connect(this, SIGNAL(UpdateUISig(STR_MSG *)), this, SLOT(UpdateReal(STR_MSG *)));
    QObject::connect(this, SIGNAL(LedUpdate(int,char *, char *)), this, SLOT(UpdateLedSlot(int,char *, char *)));

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

    m_iLedType=m_iLedH=m_iLedW=0;
    GetPrivateProfileString("Welcome", "L1", "治超检测", m_szL1, sizeof(m_szL1), strConfigfile.c_str());
    GetPrivateProfileString("Welcome", "L2", " ", m_szL2, sizeof(m_szL2), strConfigfile.c_str());
    GetPrivateProfileString("Welcome", "L3", " ", m_szL3, sizeof(m_szL3), strConfigfile.c_str());
    ::GetPrivateProfileString("SYS", "showRet", "1", szVal,100, strConfigfile.c_str());
    m_showRet = atoi(szVal);

    m_proc = new QProcess(this);
    timer_pointer = new QTimer;
    timer_pointer->setSingleShot(true);
    timer_pointer->start(500);
    connect(timer_pointer, SIGNAL(timeout()), this, SLOT(slot_timeout_timer_pointer()));
}

void OverWidget::UpdateUI(STR_MSG *msg)
{
    emit UpdateUISig(msg);
}

bool OverWidget::ConnectAllDVR()
{
    DeviceNode* pTargetNode = NULL;
    bool ret = true;

    list<DeviceNode *>::iterator itr;//容器，用于链表遍历
    m_QueueStateSect.Lock();
    for(itr = lstDevice.begin();itr !=lstDevice.end();itr++)
    {
        pTargetNode = *itr;
        if(pTargetNode->Type == 2 || pTargetNode->Type == 3 || pTargetNode->Type == 4)
        {
            if ((pTargetNode->bIsOnline == UNCONNECT)&&((pTargetNode->LoginID == -1)))
            {
                if(pTargetNode->Type == 2)
                {
                    NET_DVR_DEVICEINFO_V30 struDeviceInfo = { 0 };
                    m_lHandleDVR = NET_DVR_Login_V30(pTargetNode->sIP, pTargetNode->iPort, pTargetNode->user, pTargetNode->pwd, &struDeviceInfo);
                    if (m_lHandleDVR < 0)
                    {
                        OutputLog2("DVR reLogin error, %d", NET_DVR_GetLastError());
                        ret = false;
                    }
                    else
                    {
                        pTargetNode->bIsOnline = NORMAL;
                        m_lStartChnDVR = struDeviceInfo.byStartDChan;
                        OutputLog2("DVR r起始通道 %d", m_lStartChnDVR);
                    }
                    pTargetNode->LoginID = m_lHandleDVR;
                }

                if(pTargetNode->Type == 3)
                {
                    NET_DVR_DEVICEINFO_V30 struDeviceInfo;
                    int iLoginID = NET_DVR_Login_V30(pTargetNode->sIP, pTargetNode->iPort, pTargetNode->user, pTargetNode->pwd, &struDeviceInfo);
                    if (iLoginID < 0)
                    {
                        OutputLog2("前相机%d rLogin error, %d", pTargetNode->ID, NET_DVR_GetLastError());
                        ret = false;
                    }
                    else
                    {
                        STR_Device *pDev = NULL;
                        auto ifind = m_maplaneAndDevice.find(pTargetNode->ilane);
                        if (ifind != m_maplaneAndDevice.end())
                        {
                            pDev = ifind->second;
                            pTargetNode->bIsOnline = NORMAL;
                            //设置报警回调函数
                            NET_DVR_SetDVRMessageCallBack_V30(MSesGCallback, this);
                            //启用布防
                            NET_DVR_SETUPALARM_PARAM struSetupParam = { 0 };
                            struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
                            struSetupParam.byAlarmInfoType = 1;
                            //struSetupParam.byLevel = 1;  //级别
                            pDev->lHandlePre = NET_DVR_SetupAlarmChan_V41(iLoginID, &struSetupParam);
                            if (pDev->lHandlePre < 0)
                            {
                                OutputLog2("前相机%d rNET_DVR_SetupAlarmChan_V41 error, %d", pTargetNode->ID, NET_DVR_GetLastError());
                            }
                            pTargetNode->LoginID = pDev->lUerIDPre = iLoginID;
                        }
                    }
                }

                if(pTargetNode->Type == 4)
                {
                    NET_DVR_DEVICEINFO_V30 struDeviceInfo;
                    int iLoginID = NET_DVR_Login_V30(pTargetNode->sIP, pTargetNode->iPort, pTargetNode->user, pTargetNode->pwd, &struDeviceInfo);
                    if (iLoginID < 0)
                    {
                        OutputLog2("后相机%d rLogin error, %d", pTargetNode->ID, NET_DVR_GetLastError());
                        ret = false;
                    }
                    else
                    {
                        STR_Device *pDev = NULL;
                        auto ifind = m_maplaneAndDevice.find(pTargetNode->ilane);
                        if (ifind != m_maplaneAndDevice.end())
                        {
                            pDev = ifind->second;
                            pTargetNode->bIsOnline = NORMAL;
                            //设置报警回调函数
                            NET_DVR_SetDVRMessageCallBack_V30(MSesGCallback, this);
                            //启用布防
                            NET_DVR_SETUPALARM_PARAM struSetupParam = { 0 };
                            struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
                            struSetupParam.byAlarmInfoType = 1;
                            //struSetupParam.byLevel = 1;  //级别
                            pDev->lHandleAfr = NET_DVR_SetupAlarmChan_V41(iLoginID, &struSetupParam);
                            if (pDev->lHandleAfr < 0)
                            {
                                OutputLog2("后相机%d rNET_DVR_SetupAlarmChan_V41 error, %d", pTargetNode->ID, NET_DVR_GetLastError());
                            }
                            pTargetNode->LoginID = pDev->lUerIDAfr = iLoginID;
                        }
                    }
                }
            }
        }
    }
    m_QueueStateSect.Unlock();
    return ret;
}

void OverWidget::DetectOnlineHK(void)
{
    DeviceNode* pTargetNode = NULL;
    BOOL ret = TRUE;
    NET_DVR_NETCFG_V30 pConfig;
    DWORD dwReturned = 0;

    list<DeviceNode *>::iterator itr;//容器，用于链表遍历
    try
    {
        m_QueueStateSect.Lock();
        for(itr = lstDevice.begin();itr !=lstDevice.end();itr++)
        {
            pTargetNode = *itr;
            if(pTargetNode->Type == 2 || pTargetNode->Type == 3 || pTargetNode->Type == 4)
            {
                if ((pTargetNode->bIsOnline == UNCONNECT)&&(pTargetNode->LoginID != -1))
                {
                    ret = NET_DVR_GetDVRConfig(pTargetNode->LoginID,NET_DVR_GET_NETCFG_V30,0,&pConfig,sizeof(NET_DVR_NETCFG_V30),&dwReturned);
                    if (ret)
                    {
                        pTargetNode->bIsOnline = NORMAL;
                        OutputLog2("%s %d重连成功!", pTargetNode->sIP,pTargetNode->iPort);
                        ///////////////////////////////////////////////////////////////
                        QString scal = QStringLiteral("重连成功");
                        QDateTime current_date_time = QDateTime::currentDateTime();
                        QString strdate = current_date_time.toString("yyyy-MM-dd hh:mm:ss"); //设置显示格式
                        //保存数据
                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                        QString strSqlText("INSERT INTO DEVSTATE (optiontime,name,state) VALUES (:optiontime,:name,:state)");
                        qsQuery.prepare(strSqlText);
                        qsQuery.bindValue(":optiontime", strdate);
                        qsQuery.bindValue(":name", QString::fromLocal8Bit(pTargetNode->sname));
                        qsQuery.bindValue(":state", scal);
                        bool bsuccess = qsQuery.exec();
                        if (!bsuccess)
                        {
                            QString querys = qsQuery.lastError().text();
                            OutputLog("状态保存失败%s",querys.toStdString().c_str());
                        }
                    }
                }
                else if ((pTargetNode->bIsOnline == NORMAL)&&!(NET_DVR_GetDVRConfig(pTargetNode->LoginID,NET_DVR_GET_NETCFG_V30,0,&pConfig,sizeof(NET_DVR_NETCFG_V30),&dwReturned)))
                {
                    if (pTargetNode->nOnlineFlag) //上次检测为掉线
                    {
                        OutputLog2("%s %d 断线! 错误码%d", pTargetNode->sIP,pTargetNode->iPort, NET_DVR_GetLastError());
                        pTargetNode->bIsOnline = UNCONNECT;
                        pTargetNode->nOnlineFlag = 0; //检测标志设置为正常
                        ///////////////////////////////////////////////////////////////
                        QString scal = QStringLiteral("断线");
                        QDateTime current_date_time = QDateTime::currentDateTime();
                        QString strdate = current_date_time.toString("yyyy-MM-dd hh:mm:ss"); //设置显示格式
                        //保存数据
                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                        QString strSqlText("INSERT INTO DEVSTATE (optiontime,name,state) VALUES (:optiontime,:name,:state)");
                        qsQuery.prepare(strSqlText);
                        qsQuery.bindValue(":optiontime", strdate);
                        qsQuery.bindValue(":name", QString::fromLocal8Bit(pTargetNode->sname));
                        qsQuery.bindValue(":state", scal);
                        bool bsuccess = qsQuery.exec();
                        if (!bsuccess)
                        {
                            QString querys = qsQuery.lastError().text();
                            OutputLog("状态保存失败%s",querys.toStdString().c_str());
                        }
                    }
                    else
                    {
                        pTargetNode->nOnlineFlag = 1; //此次检测为掉线
                    }
                }
                else
                {
                    pTargetNode->nOnlineFlag = 0; //检测标志设置为正常
                }
            }
        }
        m_QueueStateSect.Unlock();
    }
    catch (...)
    {
        OutputLog2("DetectOnlineHK数据处理异常");
    }
}

void OverWidget::initReal()
{
    m_realwid = new QWidget;
    //1
    m_realleftwid = new QWidget;
    m_realleftwid->setWindowFlags(Qt::FramelessWindowHint);
    m_realleftwid->setAutoFillBackground(true);
    QPalette myPalette;
    myPalette.setColor(QPalette::Background, QColor(0, 185, 245,40));
    m_realleftwid->setPalette(myPalette);

    m_devlab = new QLabel(QStringLiteral("【实时车辆信息】"));
    m_devlab->setObjectName("s1");
    m_lanelab = new QLabel(QStringLiteral("车道:"));
    m_cphmlab = new QLabel(QStringLiteral("车牌号码:"));
    m_axislab = new QLabel(QStringLiteral("车辆轴数:"));
    m_speedlab = new QLabel(QStringLiteral("车速:"));
    m_carlenlab = new QLabel(QStringLiteral("车长:"));
    m_weightlab = new QLabel(QStringLiteral("总重量:"));
    m_curtimelab = new QLabel(QStringLiteral("检测时间:"));
    m_curtimelab2 = new QLabel(QStringLiteral(""));
    m_lanelab2 = new QLabel(QStringLiteral(""));
    m_cphmlab2 = new QLabel(QStringLiteral(""));
    m_axislab2 = new QLabel(QStringLiteral(""));
    m_speedlab2 = new QLabel(QStringLiteral(""));
    m_carlenlab2 = new QLabel(QStringLiteral(""));
    m_weightlab2 = new QLabel(QStringLiteral(""));
    m_gridlay = new QGridLayout;
    m_gridlay->addWidget(m_devlab,0,0,1,2);
    m_gridlay->addWidget(m_curtimelab,0+1,0);
    m_gridlay->addWidget(m_curtimelab2,0+1,1);
    m_gridlay->addWidget(m_lanelab,3+1,0);
    m_gridlay->addWidget(m_lanelab2,3+1,1);
    m_gridlay->addWidget(m_cphmlab,1+1,0);
    m_gridlay->addWidget(m_cphmlab2,1+1,1);
    m_gridlay->addWidget(m_axislab,2+1,0);
    m_gridlay->addWidget(m_axislab2,2+1,1);
    m_gridlay->addWidget(m_speedlab,4+1,0);
    m_gridlay->addWidget(m_speedlab2,4+1,1);
    m_gridlay->addWidget(m_carlenlab,5+1,0);
    m_gridlay->addWidget(m_carlenlab2,5+1,1);
    m_gridlay->addWidget(m_weightlab,6+1,0);
    m_gridlay->addWidget(m_weightlab2,6+1,1);
    m_gridlay->setContentsMargins(15,10,15,10);
    m_realleftwid->setLayout(m_gridlay);
    m_realleftwid->setFixedWidth(this->width()/3);
    //2
    m_cusPre = new PicLabel;
    m_cusAfr = new PicLabel;
    m_cusCe = new PicLabel;
    m_cusAll = new PicLabel;
    m_topgridlay = new QGridLayout;
    m_topgridlay->addWidget(m_cusPre,0,0);
    m_topgridlay->addWidget(m_cusAfr,0,1);
    m_topgridlay->addWidget(m_cusCe,1,0);
    m_topgridlay->addWidget(m_cusAll,1,1);
    m_topgridlay->setSpacing(10);

    m_toplay = new QHBoxLayout;
    m_toplay->addWidget(m_realleftwid);
    m_toplay->addLayout(m_topgridlay);
    m_toplay->setSpacing(10);
    //3
    QFont font("Microsoft YaHei", 13); // 微软雅黑
    QFont font2("Microsoft YaHei", 11); // 微软雅黑
    m_view = new QTableView;
    m_view->setFixedHeight(200);
    m_view->verticalHeader()->hide();							// 隐藏垂直表头
    m_view->horizontalHeader()->setFont(font);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);	//根据内容自动调整大小
    m_view->horizontalHeader()->setStretchLastSection(true);		// 伸缩最后一列
    m_view->horizontalHeader()->setHighlightSections(false);		// 点击表时不对表头行光亮
    m_view->setShowGrid(false);										// 隐藏网格线
    m_view->setAlternatingRowColors(true);							// 开启隔行异色
    m_view->setFocusPolicy(Qt::NoFocus);							// 去除当前Cell周边虚线框
    m_view->horizontalHeader()->setMinimumSectionSize(120);
    m_view->setFont(font2);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);			//为整行选中
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);			//不可多选
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);				//只读属性，即不能编辑
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);		//滑动像素
    m_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);		//滑动像素
    m_view->setFrameShape(QFrame::NoFrame);

    m_mode = new QStandardItemModel(m_realwid);
    m_view->setModel(m_mode);
    m_mode->setColumnCount(16);
    m_mode->setHeaderData(0, Qt::Horizontal, QStringLiteral("序号"));
    m_mode->setHeaderData(1, Qt::Horizontal, QStringLiteral("检测时间"));
    m_mode->setHeaderData(2, Qt::Horizontal, QStringLiteral("车牌号码"));
    m_mode->setHeaderData(3, Qt::Horizontal, QStringLiteral("车道"));
    m_mode->setHeaderData(4, Qt::Horizontal, QStringLiteral("车速"));
    m_mode->setHeaderData(5, Qt::Horizontal, QStringLiteral("轴数"));
    m_mode->setHeaderData(6, Qt::Horizontal, QStringLiteral("总重量"));
    m_mode->setHeaderData(7, Qt::Horizontal, QStringLiteral("车牌颜色"));
    m_mode->setHeaderData(8, Qt::Horizontal, QStringLiteral("轴1重量"));
    m_mode->setHeaderData(9, Qt::Horizontal, QStringLiteral("轴2重量"));
    m_mode->setHeaderData(10, Qt::Horizontal, QStringLiteral("轴3重量"));
    m_mode->setHeaderData(11, Qt::Horizontal, QStringLiteral("轴4重量"));
    m_mode->setHeaderData(12, Qt::Horizontal, QStringLiteral("轴5重量"));
    m_mode->setHeaderData(13, Qt::Horizontal, QStringLiteral("轴6重量"));
    m_mode->setHeaderData(14, Qt::Horizontal, QStringLiteral("轴7重量"));
    m_mode->setHeaderData(15, Qt::Horizontal, QStringLiteral("轴8重量"));

    m_mainlay = new QVBoxLayout;
    m_mainlay->addLayout(m_toplay);
    m_mainlay->addWidget(m_view);
    m_mainlay->setSpacing(10);
    m_mainlay->setMargin(0);
    m_realwid->setLayout(m_mainlay);
}

void OverWidget::initHis()
{
    m_hiswid = new QWidget;
    m_histopwid = new QWidget;
    m_histopwid->setWindowFlags(Qt::FramelessWindowHint);
    m_histopwid->setAutoFillBackground(true);
    QPalette Palette;
    Palette.setColor(QPalette::Background, QColor(0, 185, 245,40));
    m_histopwid->setPalette(Palette);

    //设置图标底色和背景色一样
    QPalette myPalette;
    QColor myColor(0, 0, 0);
    myColor.setAlphaF(0);
    myPalette.setBrush(backgroundRole(), myColor);
    //1
    QFont font2("Microsoft YaHei", 13); // 微软雅黑
    QFont font("Microsoft YaHei", 11); // 微软雅黑
    m_timelab = new QLabel;
    m_timelab->setText(QStringLiteral("选择起止时间"));
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
    m_querybut = new QPushButton;
    m_querybut->setText(QStringLiteral(" 查 询 "));
    m_querybut->setFixedSize(80, 30);
    m_querybut->setFont(font);
    m_exportbut = new QPushButton;
    m_exportbut->setText(QStringLiteral(" 导 出 "));
    m_exportbut->setFixedSize(80, 30);
    m_exportbut->setFont(font);
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
    m_histopwid->setLayout(m_toplayhis);
    //2
    m_viewhis = new QTableView;
    m_modehis = new ResultTableModel;
    m_viewhis->setModel(m_modehis);
    m_viewhis->verticalHeader()->hide();							// 隐藏垂直表头
    m_viewhis->horizontalHeader()->setFont(font2);
    m_viewhis->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);	//根据内容自动调整大小
    m_viewhis->horizontalHeader()->setMinimumSectionSize(100);
    m_viewhis->horizontalHeader()->setStretchLastSection(true);		// 伸缩最后一列
    m_viewhis->horizontalHeader()->setHighlightSections(false);		// 点击表时不对表头行光亮
    m_viewhis->setShowGrid(false);										// 隐藏网格线
    m_viewhis->setAlternatingRowColors(true);							// 开启隔行异色
    m_viewhis->setFocusPolicy(Qt::NoFocus);							// 去除当前Cell周边虚线框
    m_viewhis->setFont(font);
    m_viewhis->setSelectionBehavior(QAbstractItemView::SelectRows);		//为整行选中
    m_viewhis->setSelectionMode(QAbstractItemView::SingleSelection);		//不可多选
    m_viewhis->setEditTriggers(QAbstractItemView::NoEditTriggers);			//只读属性，即不能编辑
    m_viewhis->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);	//滑动像素
    m_viewhis->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);	//滑动像
    m_viewhis->setFrameShape(QFrame::NoFrame);

    m_mainlayhis = new QVBoxLayout;
    m_mainlayhis->addWidget(m_histopwid);
    m_mainlayhis->addWidget(m_viewhis);
    m_mainlayhis->setSpacing(12);
    m_mainlayhis->setMargin(0);
    m_hiswid->setLayout(m_mainlayhis);

    QObject::connect(m_querybut, SIGNAL(clicked()), this, SLOT(query()));
    QObject::connect(m_exportbut, SIGNAL(clicked()), this, SLOT(exportto()));
    QObject::connect(m_stime, SIGNAL(clicked()), this, SLOT(showSTimeLabel()));
    QObject::connect(m_etime, SIGNAL(clicked()), this, SLOT(showETimeLabel()));
    QObject::connect(m_starttime, SIGNAL(sendTime(const QString&)), this, SLOT(recvTime(const QString&)));
    QObject::connect(m_endttime, SIGNAL(sendTime(const QString&)), this, SLOT(recvTime(const QString&)));
    QObject::connect(m_viewhis, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClickedToDetails(QModelIndex)));
}

void OverWidget::initLog()
{
    m_logwid = new QWidget();
    //1
    m_editlog = new QTextEdit;
    m_editlog->document()->setMaximumBlockCount(100);
    m_logbut = new QPushButton(QStringLiteral("记录"));
    m_mainlaylog = new QVBoxLayout;
    m_mainlaylog->addWidget(m_editlog);
    m_mainlaylog->addWidget(m_logbut,0,Qt::AlignCenter);
    m_mainlaylog->setMargin(0);
    m_logwid->setLayout(m_mainlaylog);
    QObject::connect(m_logbut, SIGNAL(clicked()), this, SLOT(logcy()));
}

void OverWidget::initRecord()
{
    m_preTimer = new QTimer;
    connect(m_preTimer, SIGNAL(timeout()), this, SLOT(slot_timeout_pre_pointer()));
    m_Timer = new QTimer;
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(slot_timeout_pointer()));

    m_recordshowwid = new QWidget;
    //1
    m_recordcusPre = new PicLabel;
    m_recordcusAfr = new PicLabel;
    m_recordcusSmall = new PicLabel;
    m_recordcusCe = new PicLabel;
    m_recordcusVideo = new PicLabel;
    m_recordcusPre->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusAfr->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusCe->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordcusVideo->setToolTip(QStringLiteral("可双击查看,按Esc返回"));
    m_recordtoplayout = new QHBoxLayout;
    m_recordtoplayout->addWidget(m_recordcusVideo);
    m_recordtoplayout->addWidget(m_recordcusPre);
    m_recordtoplayout->addWidget(m_recordcusAfr);
    m_recordtoplayout->addWidget(m_recordcusCe);
    m_recordtoplayout->addWidget(m_recordcusSmall);
    m_recordtoplayout->addSpacing(10);

    m_recordlanelab = new QLabel(QStringLiteral("车道:"));
    m_recordcphmlab = new QLabel(QStringLiteral("车牌号码:"));
    m_recordaxislab = new QLabel(QStringLiteral("轴数:"));
    m_recordspeedlab = new QLabel(QStringLiteral("车辆速度:"));
    m_recordcolorlab = new QLabel(QStringLiteral("车牌颜色:"));
    m_recordcurtimelab = new QLabel(QStringLiteral("检测时间:"));
    m_recordcartypelab = new QLabel(QStringLiteral("车辆类型:"));
    m_recordweightlab = new QLabel(QStringLiteral("总重量:"));
    m_recorddirectionlab = new QLabel(QStringLiteral("行车方向:"));
    m_recordcrossroadlab = new QLabel(QStringLiteral("是否跨道:"));
    m_recordcurtimelab2 = new QLabel(QStringLiteral(""));
    m_recordlanelab2 = new QLabel(QStringLiteral(""));
    m_recordcphmlab2 = new QLabel(QStringLiteral(""));
    m_recordaxislab2 = new QLabel(QStringLiteral(""));
    m_recordspeedlab2 = new QLabel(QStringLiteral(""));
    m_recordcolorlab2 = new QLabel(QStringLiteral(""));
    m_recordweightlab2 = new QLabel(QStringLiteral(""));
    m_recordcartypelab2 = new QLabel(QStringLiteral(""));
    m_recorddirectionlab2 = new QLabel(QStringLiteral(""));
    m_recordcrossroadlab2 = new QLabel(QStringLiteral(""));
    m_recordgridlay = new QGridLayout;
    m_recordgridlay->addWidget(m_recordcurtimelab,0,0);
    m_recordgridlay->addWidget(m_recordcurtimelab2,0,1);
    m_recordgridlay->addWidget(m_recordcphmlab,0,2);
    m_recordgridlay->addWidget(m_recordcphmlab2,0,3);
    m_recordgridlay->addWidget(m_recordspeedlab,0,4);
    m_recordgridlay->addWidget(m_recordspeedlab2,0,5);

    m_recordgridlay->addWidget(m_recordlanelab,1,0);
    m_recordgridlay->addWidget(m_recordlanelab2,1,1);
    m_recordgridlay->addWidget(m_recordcolorlab,1,2);
    m_recordgridlay->addWidget(m_recordcolorlab2,1,3);
    m_recordgridlay->addWidget(m_recordaxislab,1,4);
    m_recordgridlay->addWidget(m_recordaxislab2,1,5);

    m_recordgridlay->addWidget(m_recorddirectionlab,2,0);
    m_recordgridlay->addWidget(m_recorddirectionlab2,2,1);
    m_recordgridlay->addWidget(m_recordcrossroadlab,2,2);
    m_recordgridlay->addWidget(m_recordcrossroadlab2,2,3);
    m_recordgridlay->addWidget(m_recordweightlab,2,4);
    m_recordgridlay->addWidget(m_recordweightlab2,2,5);
    m_recordgridlay->setContentsMargins(35,20,35,20);

    //2
    m_recordshowbottomwid = new QWidget;
    m_recordshowbottomwid->setWindowFlags(Qt::FramelessWindowHint);
    m_recordshowbottomwid->setAutoFillBackground(true);
    QPalette myPalette;
    myPalette.setColor(QPalette::Background, QColor(0, 185, 245,40));
    m_recordshowbottomwid->setPalette(myPalette);

    m_recordweight1lab = new QLabel(QStringLiteral("轴1重:"));
    m_recordweight2lab = new QLabel(QStringLiteral("轴2重:"));
    m_recordweight3lab = new QLabel(QStringLiteral("轴3重:"));
    m_recordweight4lab = new QLabel(QStringLiteral("轴4重:"));
    m_recordweight5lab = new QLabel(QStringLiteral("轴5重:"));
    m_recordweight6lab = new QLabel(QStringLiteral("轴6重:"));
    m_recordweight7lab = new QLabel(QStringLiteral("轴7重:"));
    m_recordweight8lab = new QLabel(QStringLiteral("轴8重:"));
    m_recordweight1lab2 = new QLabel(QStringLiteral(""));
    m_recordweight2lab2 = new QLabel(QStringLiteral(""));
    m_recordweight3lab2 = new QLabel(QStringLiteral(""));
    m_recordweight4lab2 = new QLabel(QStringLiteral(""));
    m_recordweight5lab2 = new QLabel(QStringLiteral(""));
    m_recordweight6lab2 = new QLabel(QStringLiteral(""));
    m_recordweight7lab2 = new QLabel(QStringLiteral(""));
    m_recordweight8lab2 = new QLabel(QStringLiteral(""));
    m_recordspace1lab = new QLabel(QStringLiteral("轴1间距:"));
    m_recordspace2lab = new QLabel(QStringLiteral("轴2间距:"));
    m_recordspace3lab = new QLabel(QStringLiteral("轴3间距:"));
    m_recordspace4lab = new QLabel(QStringLiteral("轴4间距:"));
    m_recordspace5lab = new QLabel(QStringLiteral("轴5间距:"));
    m_recordspace6lab = new QLabel(QStringLiteral("轴6间距:"));
    m_recordspace7lab = new QLabel(QStringLiteral("轴7间距:"));
    m_recordspace1lab2 = new QLabel(QStringLiteral(""));
    m_recordspace2lab2 = new QLabel(QStringLiteral(""));
    m_recordspace3lab2 = new QLabel(QStringLiteral(""));
    m_recordspace4lab2 = new QLabel(QStringLiteral(""));
    m_recordspace5lab2 = new QLabel(QStringLiteral(""));
    m_recordspace6lab2 = new QLabel(QStringLiteral(""));
    m_recordspace7lab2 = new QLabel(QStringLiteral(""));

    m_recordgridlay2 = new QGridLayout;
    m_recordgridlay2->addWidget(m_recordweight1lab,0,0);
    m_recordgridlay2->addWidget(m_recordweight1lab2,0,1);
    m_recordgridlay2->addWidget(m_recordweight2lab,0,2);
    m_recordgridlay2->addWidget(m_recordweight2lab2,0,3);
    m_recordgridlay2->addWidget(m_recordweight3lab,0,4);
    m_recordgridlay2->addWidget(m_recordweight3lab2,0,5);
    m_recordgridlay2->addWidget(m_recordweight4lab,0,6);
    m_recordgridlay2->addWidget(m_recordweight4lab2,0,7);
    m_recordgridlay2->addWidget(m_recordweight5lab,1,0);
    m_recordgridlay2->addWidget(m_recordweight5lab2,1,1);
    m_recordgridlay2->addWidget(m_recordweight6lab,1,2);
    m_recordgridlay2->addWidget(m_recordweight6lab2,1,3);
    m_recordgridlay2->addWidget(m_recordweight7lab,1,4);
    m_recordgridlay2->addWidget(m_recordweight7lab2,1,5);
    m_recordgridlay2->addWidget(m_recordweight8lab,1,6);
    m_recordgridlay2->addWidget(m_recordweight8lab2,1,7);
    m_recordgridlay2->addWidget(m_recordspace1lab,2,0);
    m_recordgridlay2->addWidget(m_recordspace1lab2,2,1);
    m_recordgridlay2->addWidget(m_recordspace2lab,2,2);
    m_recordgridlay2->addWidget(m_recordspace2lab2,2,3);
    m_recordgridlay2->addWidget(m_recordspace3lab,2,4);
    m_recordgridlay2->addWidget(m_recordspace3lab2,2,5);
    m_recordgridlay2->addWidget(m_recordspace4lab,2,6);
    m_recordgridlay2->addWidget(m_recordspace4lab2,2,7);
    m_recordgridlay2->addWidget(m_recordspace5lab,3,0);
    m_recordgridlay2->addWidget(m_recordspace5lab2,3,1);
    m_recordgridlay2->addWidget(m_recordspace6lab,3,2);
    m_recordgridlay2->addWidget(m_recordspace6lab2,3,3);
    m_recordgridlay2->addWidget(m_recordspace7lab,3,4);
    m_recordgridlay2->addWidget(m_recordspace7lab2,3,5);
    m_recordgridlay2->setContentsMargins(25,20,25,20);
    m_recordshowbottomwid->setLayout(m_recordgridlay2);

    m_recordmainlay = new QVBoxLayout;
    m_recordmainlay->addLayout(m_recordtoplayout);
    m_recordmainlay->addLayout(m_recordgridlay);
    m_recordmainlay->addWidget(m_recordshowbottomwid);
    m_recordmainlay->setSpacing(10);
    m_recordmainlay->setMargin(10);
    m_recordshowwid->setLayout(m_recordmainlay);

    QObject::connect(m_recordcusVideo, SIGNAL(clicked()), this, SLOT(onPlayRecord()));
    QObject::connect(m_recordcusCe, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusVideo, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusPre, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
    QObject::connect(m_recordcusAfr, SIGNAL(dbclicked()), this, SLOT(onFullProc()));
}

void OverWidget::onFullProc()
{
    char szRatio[32];
    m_showwid->showFullScreen();
    if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusPre)
    {
        m_showpiclab->showpic(m_recordcusPre->m_bk);
    }
    else if (dynamic_cast<PicLabel*>(QObject::sender()) == m_recordcusAfr)
    {
        m_showpiclab->showpic(m_recordcusAfr->m_bk);
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

//#ifdef WIN32
//        m_vlc_media = libvlc_media_new_path(m_vlc_ins, m_strPre.toStdString().c_str());
//        // 把打开的媒体文件设置给播放器
//        libvlc_media_player_set_media(m_vlc_player, m_vlc_media);
//        //设置显示的窗口
//        libvlc_media_player_set_hwnd(m_vlc_player, (void *)m_showpiclab->winId());
//        sprintf_s(szRatio,"%d:%d",m_showpiclab->width(),m_showpiclab->height());
//        libvlc_video_set_aspect_ratio(m_vlc_player, szRatio);
//        libvlc_media_player_play(m_vlc_player);
//#endif

        m_Timer->start(500);
    }
}

void OverWidget::cdslot2()
{
    m_dtime2->setButtonBackground((":/pnd/png/d-press.png"));
    m_mtime2->setButtonBackground((":/pnd/png/m.png"));
    m_ytime2->setButtonBackground((":/pnd/png/y.png"));
    m_monthwid2->hide();
    m_yearwid2->hide();
    m_daywid2->show();
}

void OverWidget::cmslot2()
{
    m_dtime2->setButtonBackground((":/pnd/png/d.png"));
    m_mtime2->setButtonBackground((":/pnd/png/m-press.png"));
    m_ytime2->setButtonBackground((":/pnd/png/y.png"));
    m_daywid2->hide();
    m_yearwid2->hide();
    m_monthwid2->show();
}

void OverWidget::cyslot2()
{
    m_dtime2->setButtonBackground((":/pnd/png/d.png"));
    m_mtime2->setButtonBackground((":/pnd/png/m.png"));
    m_ytime2->setButtonBackground((":/pnd/png/y-press.png"));
    m_monthwid2->hide();
    m_daywid2->hide();
    m_yearwid2->show();
}

void OverWidget::UpdateLedSlot(int itype, char *CarNo, char *ret)
{
    if(itype == 0)
    {
        m_UP1->setText(QString::fromLocal8Bit(m_szL1));
        m_UP2->setText(QString::fromLocal8Bit(m_szL2));
        m_DOWN->setText(QString::fromLocal8Bit(m_szL3));
    }
    else
    {
        m_UP1->setText(QStringLiteral("治超检测"));
        m_UP2->setText(QString::fromLocal8Bit(CarNo));
        if(m_showRet == 1)
        {
            m_DOWN->setText(QString::fromLocal8Bit(ret));
            if(strstr(ret,"不")!=NULL)
                m_DOWN->setStyleSheet(QString("background-color:black; color:red;font-family:\"Microsoft YaHei\";font-size:%1px;").arg(m_iLedFont+2));
            else
                m_DOWN->setStyleSheet(QString("background-color:black; color:green;font-family:\"Microsoft YaHei\";font-size:%1px;").arg(m_iLedFont+2));
        }
        else
        {
            m_DOWN->setText("");
        }
        m_DOWN->update();
    }
}

void OverWidget::logcy()
{
    m_bStartGetSample = !m_bStartGetSample;
    m_logbut->setText(m_bStartGetSample ? QStringLiteral("停止") : QStringLiteral("开始"));
    if (m_bStartGetSample)
        LOG4CPLUS_DEBUG(pTestLogger, "log开始");
    else
        LOG4CPLUS_DEBUG(pTestLogger, "log停止");
}

void OverWidget::cdslot()
{
    m_dtime->setButtonBackground((":/pnd/png/d-press.png"));
    m_mtime->setButtonBackground((":/pnd/png/m.png"));
    m_ytime->setButtonBackground((":/pnd/png/y.png"));
    m_monthwid->hide();
    m_yearwid->hide();
    m_daywid->show();
}

void OverWidget::cmslot()
{
    m_dtime->setButtonBackground((":/pnd/png/d.png"));
    m_mtime->setButtonBackground((":/pnd/png/m-press.png"));
    m_ytime->setButtonBackground((":/pnd/png/y.png"));
    m_daywid->hide();
    m_yearwid->hide();
    m_monthwid->show();
}

void OverWidget::cyslot()
{
    m_dtime->setButtonBackground((":/pnd/png/d.png"));
    m_mtime->setButtonBackground((":/pnd/png/m.png"));
    m_ytime->setButtonBackground((":/pnd/png/y-press.png"));
    m_monthwid->hide();
    m_daywid->hide();
    m_yearwid->show();
}

void OverWidget::updateRecord(ResultRecord &recod)
{
    m_strPre = recod.qstrpicvideo;
    //m_strPre.replace(QString("/"),QString("\\"));
    m_recordcusPre->showpic(recod.qstrpicpre);
    m_recordcusAfr->showpic(recod.qstrpicafr);
    m_recordcusSmall->showpic(recod.qstrpicsmall,1);
    m_recordcusCe->showpic(recod.qstrpicce);
    m_recordcusVideo->showpic(":/pnd/png/play.png",1);
    m_recordlanelab2->setText(recod.qstrlane);
    m_recordcphmlab2->setText(recod.qstrcarno);
    m_recordcurtimelab2->setText(recod.qstrtime);
    m_recordaxislab2->setText(recod.qstraxis);
    m_recordcartypelab2->setText(recod.qstrcartype);
    m_recordspeedlab2->setText(recod.qstrspeed);
    m_recordweightlab2->setText(recod.qstrweight);
    m_recordcolorlab2->setText(recod.qstrcolor);
    m_recorddirectionlab2->setText(recod.qstrdirection);
    m_recordcrossroadlab2->setText(recod.qstrcrossroad);

    m_recordweight1lab2->setText(recod.qstrweight1);
    m_recordweight2lab2->setText(recod.qstrweight2);
    m_recordweight3lab2->setText(recod.qstrweight3);
    m_recordweight4lab2->setText(recod.qstrweight4);
    m_recordweight5lab2->setText(recod.qstrweight5);
    m_recordweight6lab2->setText(recod.qstrweight6);
    m_recordweight7lab2->setText(recod.qstrweight7);
    m_recordweight8lab2->setText(recod.qstrweight8);
    m_recordspace1lab2->setText(recod.qstrs1);
    m_recordspace2lab2->setText(recod.qstrs2);
    m_recordspace3lab2->setText(recod.qstrs3);
    m_recordspace4lab2->setText(recod.qstrs4);
    m_recordspace5lab2->setText(recod.qstrs5);
    m_recordspace6lab2->setText(recod.qstrs6);
    m_recordspace7lab2->setText(recod.qstrs7);

}

void OverWidget::onDoubleClickedToDetails(const QModelIndex &index)
{
    int nRow = index.row();
    int nColumn = index.column();
    ResultRecord record = m_modehis->getRecordList().at(nRow);
    m_backbut->show();
    m_hiswid->hide();
    m_recordshowwid->show();
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

//#ifdef WIN32
//    EnableWindow((HWND)m_recordcusVideo->winId(),false);
//#endif

//    m_Prevlc_media = libvlc_media_new_path(m_Prevlc_ins, m_strPre.toStdString().c_str());
//    // 把打开的媒体文件设置给播放器
//    libvlc_media_player_set_media(m_Prevlc_player, m_Prevlc_media);
//    //设置显示的窗口
//    libvlc_media_player_set_hwnd(m_Prevlc_player, (void *)m_recordcusVideo->winId());
//    sprintf_s(szRatio,"%d:%d",m_recordcusVideo->width(),m_recordcusVideo->height());
//    libvlc_video_set_aspect_ratio(m_Prevlc_player, szRatio);
//    libvlc_media_player_play(m_Prevlc_player);

//    m_preTimer->start(500);
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

//        m_Timer->stop();
////#ifdef WIN32
//        // 停止
//        libvlc_media_player_stop(m_vlc_player);
//        // 释放
//        libvlc_media_release(m_vlc_media);
////#endif
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

//        m_preTimer->stop();
////#ifdef WIN32
//        // 停止
//        libvlc_media_player_stop(m_Prevlc_player);
//        // 释放
//        libvlc_media_release(m_Prevlc_media);
////#endif
//        m_recordcusVideo->update();
    }
}

void OverWidget::createChartSplineSeriesY(int nXCount, QDate date)
{
    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    QFont font;
    font.setBold(false);
    font.setPixelSize(13);

    QPen pen;
    pen.setStyle(Qt::SolidLine);

    //设置X轴坐标
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("yyyy");
    axisX->setTitleFont(font);
    QDate startDate = date.addYears(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    axisX->setRange(QDateTime(startDate), QDateTime(date));
    axisX->setTickCount(nXCount);
    axisX->setGridLineVisible(true);
    axisX->setGridLinePen(pen);
    axisX->setGridLineColor(QColor(250,250,250));
    axisX->setLabelsColor(QColor(250,250,250));
    axisX->setLabelsFont(font);
    //设置Y轴坐标
    int nMaxY = -1;
    QLineSeries *series1 = createSeries1Y(nXCount, date,nMaxY);
    double decimal = double(nMaxY) / 1000.0;
    int nVal = qCeil(decimal); //向上取整
    QValueAxis *axisY = new QValueAxis();
//    axisY->setTitleText(QStringLiteral("辆"));
    axisY->setTitleFont(font);
    axisY->setRange(0, nVal * 1000);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(5);
    axisY->setGridLineVisible(true);
    axisY->setLabelsColor(QColor(250,250,250));
    axisY->setGridLinePen(pen);
    axisY->setGridLineColor(QColor(250,250,250));
    axisY->setLabelsFont(font);

    //添加线到图表,注意需要先设置x轴和y轴
    chart->addSeries(series1);
    chart->setAxisX(axisX, series1);
    chart->setAxisY(axisY, series1);
    chart->legend()->hide();

    m_yearwid = new QChartView(chart);
    m_yearwid->setRenderHint(QPainter::Antialiasing); //设置抗锯齿
}

QLineSeries *OverWidget::createSeries1Y(int nXCount, QDate date,int &YMax)
{
    QLineSeries *series = new QLineSeries();
    QPen pen;
    pen.setWidth(2);
    series->setPen(pen);
    series->setColor(QColor(0,255,170));

    QDate startDate = date.addYears(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    QList<QDate> listDate;
    for (int i = 0; i < nXCount; i++)
    {
        listDate.append(startDate.addYears(i));
    }

    for (int j = 0; j < listDate.count(); j++)
    {
        QDate dCurrDate = listDate.at(j);
        QString sCurrDate = dCurrDate.toString("yyyy");
        //
        int icc = 0;
        {
            char strSql[256] = {};
            sprintf(strSql, "SELECT count(*) FROM Task ");
            QString strTemp = strSql;
            sprintf(strSql, "WHERE DetectionTime >= '%s-01-01 00:00:00' and DetectionTime <= '%s-12-31 23:59:59' ", sCurrDate.toStdString().c_str(), sCurrDate.toStdString().c_str());
            QString strWhere = strSql;
            strTemp += strWhere;
            QSqlQuery query(g_DB);
            query.prepare(strTemp);
            if (!query.exec())
            {
                QString querys = query.lastError().text();
                OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
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
                    while (query.next())
                    {
                        icc = query.value(0).toInt();
                    }
                }
            }
            if (icc > YMax)
                YMax = icc;
            qDebug() << sCurrDate << "     " << icc;
        }
        series->append(QPointF(QDateTime(dCurrDate).toMSecsSinceEpoch(), icc));
    }

    QFont font;
    font.setBold(true);
    font.setPixelSize(12);

    series->setVisible(true);
    series->setPointsVisible(true);
    series->setPointLabelsFormat("    @yPoint");
    series->setPointLabelsFont(font);
    series->setPointLabelsColor(QColor(0,255,170));
    series->setPointLabelsClipping(false);
    series->setPointLabelsVisible(true);

    return series;
}

void OverWidget::createChartSplineSeriesM(int nXCount, QDate date)
{
    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    QFont font;
    font.setBold(false);
    font.setPixelSize(13);

    QPen pen;
    pen.setStyle(Qt::SolidLine);

    //设置X轴坐标
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("yyyy/MM");
    axisX->setTitleFont(font);
    QDate startDate = date.addMonths(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    axisX->setRange(QDateTime(startDate), QDateTime(date));
    axisX->setTickCount(nXCount/2);
    axisX->setGridLineVisible(true);
    axisX->setGridLinePen(pen);
    axisX->setGridLineColor(QColor(250,250,250));
    axisX->setLabelsColor(QColor(250,250,250));
    axisX->setLabelsFont(font);
    //设置Y轴坐标
    int nMaxY = -1;
    QLineSeries *series1 = createSeries1M(nXCount, date, nMaxY);
    double decimal = double(nMaxY) / 100.0;
    int nVal = qCeil(decimal); //向上取整
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleFont(font);
    axisY->setRange(0, nVal * 100);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(5);
    axisY->setGridLineVisible(true);
    axisY->setLabelsColor(QColor(250,250,250));
    axisY->setGridLinePen(pen);
    axisY->setGridLineColor(QColor(250,250,250));
    axisY->setLabelsFont(font);

    //添加线到图表,注意需要先设置x轴和y轴
    chart->addSeries(series1);
    chart->setAxisX(axisX, series1);
    chart->setAxisY(axisY, series1);
    chart->legend()->hide();

    m_monthwid = new QChartView(chart);
    m_monthwid->setRenderHint(QPainter::Antialiasing); //设置抗锯齿
}

QLineSeries *OverWidget::createSeries1M(int nXCount, QDate date,int &YMax)
{
    QLineSeries *series = new QLineSeries();
    QPen pen;
    pen.setWidth(2);
    series->setPen(pen);
    series->setColor(QColor(0,255,170));

    QDate startDate = date.addMonths(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    QList<QDate> listDate;
    for (int i = 0; i < nXCount; i++)
    {
        listDate.append(startDate.addMonths(i));
    }

    for (int j = 0; j < listDate.count(); j++)
    {
        QDate dCurrDate = listDate.at(j);
        QString sCurrDate = dCurrDate.toString("yyyy-MM");
        //
        int icc = 0;
        {
            char strSql[256] = {};
            sprintf(strSql, "SELECT count(*) FROM Task ");
            QString strTemp = strSql;
            sprintf(strSql, "WHERE DetectionTime >= '%s-01 00:00:00' and DetectionTime <= '%s-31 23:59:59' ", sCurrDate.toStdString().c_str(), sCurrDate.toStdString().c_str());
            QString strWhere = strSql;
            strTemp += strWhere;
            QSqlQuery query(g_DB);
            query.prepare(strTemp);
            if (!query.exec())
            {
                QString querys = query.lastError().text();
                OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
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
                    while (query.next())
                    {
                        icc = query.value(0).toInt();
                    }
                }
            }
            if (icc > YMax)
                YMax = icc;
            qDebug() << sCurrDate << "     " << icc;
        }
        series->append(QPointF(QDateTime(dCurrDate).toMSecsSinceEpoch(), icc));
    }

    QFont font;
    font.setBold(true);
    font.setPixelSize(12);

    series->setVisible(true);
    series->setPointsVisible(true);
    series->setPointLabelsFormat("    @yPoint");
    series->setPointLabelsFont(font);
    series->setPointLabelsColor(QColor(0,255,170));
    series->setPointLabelsClipping(false);
    series->setPointLabelsVisible(true);

    return series;
}

void OverWidget::createChartSplineSeries(int nXCount, QDate date)
{
    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    QFont font;
    font.setBold(false);
    font.setPixelSize(13);

    QPen pen;
    pen.setStyle(Qt::SolidLine);

    //设置X轴坐标
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("MM/dd");
    axisX->setTitleFont(font);
    QDate startDate = date.addDays(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    axisX->setRange(QDateTime(startDate), QDateTime(date));
    axisX->setTickCount(nXCount/5);
    axisX->setGridLineVisible(true);
    axisX->setGridLinePen(pen);
    axisX->setGridLineColor(QColor(250,250,250));
    axisX->setLabelsColor(QColor(250,250,250));
    axisX->setLabelsFont(font);
    //设置Y轴坐标
    int nMaxY = -1;
    QLineSeries *series1 = createSeries1(nXCount, date,nMaxY);
    double decimal = double(nMaxY) / 10.0;
    int nVal = qCeil(decimal); //向上取整
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleFont(font);
    axisY->setRange(0, nVal * 10);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(5);
    axisY->setGridLineVisible(true);
    axisY->setLabelsColor(QColor(250,250,250));
    axisY->setGridLinePen(pen);
    axisY->setGridLineColor(QColor(250,250,250));
    axisY->setLabelsFont(font);
    //添加线到图表,注意需要先设置x轴和y轴
    chart->addSeries(series1);
    chart->setAxisX(axisX, series1);
    chart->setAxisY(axisY, series1);
    chart->legend()->hide();
    m_daywid = new QChartView(chart);
    m_daywid->setRenderHint(QPainter::Antialiasing); //设置抗锯齿
}

QLineSeries *OverWidget::createSeries1(int nXCount, QDate date,int &YMax)
{
    QLineSeries *series = new QLineSeries();
    QPen pen;
    pen.setWidth(2);
    series->setPen(pen);
    series->setColor(QColor(0,255,170));

    QDate startDate = date.addDays(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    QList<QDate> listDate;
    for (int i = 0; i < nXCount; i++)
    {
        listDate.append(startDate.addDays(i));
    }

    for (int j = 0; j < listDate.count(); j++)
    {
        QDate dCurrDate = listDate.at(j);
        QString sCurrDate = dCurrDate.toString("yyyy-MM-dd");
        //
        int icc = 0;
        {
            char strSql[256] = {};
            sprintf(strSql, "SELECT count(*) FROM Task ");
            QString strTemp = strSql;
            sprintf(strSql, "WHERE DetectionTime >= '%s 00:00:00' and DetectionTime <= '%s 23:59:59' ", sCurrDate.toStdString().c_str(), sCurrDate.toStdString().c_str());
            QString strWhere = strSql;
            strTemp += strWhere;
            QSqlQuery query(g_DB);
            query.prepare(strTemp);
            if (!query.exec())
            {
                QString querys = query.lastError().text();
                OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
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
                    while (query.next())
                    {
                        icc = query.value(0).toInt();
                    }
                }
            }
            if (icc > YMax)
                YMax = icc;
            qDebug() << sCurrDate << "     " << icc;
        }
        series->append(QPointF(QDateTime(dCurrDate).toMSecsSinceEpoch(), icc));
    }

    QFont font;
    font.setBold(true);
    font.setPixelSize(10);

    series->setVisible(true);
    series->setPointsVisible(true);
    series->setPointLabelsFormat("    @yPoint");
    series->setPointLabelsFont(font);
    series->setPointLabelsColor(QColor(0,255,170));
    series->setPointLabelsClipping(false);
    series->setPointLabelsVisible(true);

    return series;
}

void OverWidget::createChartSplineSeriesY2(int nXCount, QDate date)
{
    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    QFont font;
    font.setBold(false);
    font.setPixelSize(13);

    QPen pen;
    pen.setStyle(Qt::SolidLine);

    //设置X轴坐标
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("yyyy");
    axisX->setTitleFont(font);
    QDate startDate = date.addYears(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    axisX->setRange(QDateTime(startDate), QDateTime(date));
    axisX->setTickCount(nXCount);
    axisX->setGridLineVisible(true);
    axisX->setGridLinePen(pen);
    axisX->setGridLineColor(QColor(250,250,250));
    axisX->setLabelsColor(QColor(250,250,250));
    axisX->setLabelsFont(font);
    //设置Y轴坐标
    int nMaxY = -1;
    QLineSeries *series1 = createSeries1Y2(nXCount, date,nMaxY);
    double decimal = double(nMaxY) / 100.0;
    int nVal = qCeil(decimal); //向上取整
    QValueAxis *axisY = new QValueAxis();
//    axisY->setTitleText(QStringLiteral("辆"));
    axisY->setTitleFont(font);
    axisY->setRange(0, nVal * 100);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(5);
    axisY->setGridLineVisible(true);
    axisY->setLabelsColor(QColor(250,250,250));
    axisY->setGridLinePen(pen);
    axisY->setGridLineColor(QColor(250,250,250));
    axisY->setLabelsFont(font);

    //添加线到图表,注意需要先设置x轴和y轴
    chart->addSeries(series1);
    chart->setAxisX(axisX, series1);
    chart->setAxisY(axisY, series1);
    chart->legend()->hide();

    m_yearwid2 = new QChartView(chart);
    m_yearwid2->setRenderHint(QPainter::Antialiasing); //设置抗锯齿
}

QLineSeries *OverWidget::createSeries1Y2(int nXCount, QDate date,int &YMax)
{
    QLineSeries *series = new QLineSeries();
    QPen pen;
    pen.setWidth(2);
    series->setPen(pen);
    series->setColor(QColor(0,255,170));

    QDate startDate = date.addYears(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    QList<QDate> listDate;
    for (int i = 0; i < nXCount; i++)
    {
        listDate.append(startDate.addYears(i));
    }

    for (int j = 0; j < listDate.count(); j++)
    {
        QDate dCurrDate = listDate.at(j);
        QString sCurrDate = dCurrDate.toString("yyyy");
        //
        int icc = 0;
        {
            char strSql[256] = {};
            sprintf(strSql, "SELECT count(*) FROM Task ");
            QString strTemp = strSql;
            sprintf(strSql, "WHERE DetectionTime >= '%s-01-01 00:00:00' and DetectionTime <= '%s-12-31 23:59:59' and DetectionResult = '不合格'", sCurrDate.toStdString().c_str(), sCurrDate.toStdString().c_str());
            QString strWhere = QString::fromLocal8Bit(strSql);
            strTemp += strWhere;
            QSqlQuery query(g_DB);
            query.prepare(strTemp);
            if (!query.exec())
            {
                QString querys = query.lastError().text();
                OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
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
                    while (query.next())
                    {
                        icc = query.value(0).toInt();
                    }
                }
            }
            if (icc > YMax)
                YMax = icc;
            qDebug() << sCurrDate << "     " << icc;
        }
        series->append(QPointF(QDateTime(dCurrDate).toMSecsSinceEpoch(), icc));
    }

    QFont font;
    font.setBold(true);
    font.setPixelSize(12);

    series->setVisible(true);
    series->setPointsVisible(true);
    series->setPointLabelsFormat("    @yPoint");
    series->setPointLabelsFont(font);
    series->setPointLabelsColor(QColor(0,255,170));
    series->setPointLabelsClipping(false);
    series->setPointLabelsVisible(true);

    return series;
}

void OverWidget::createChartSplineSeriesM2(int nXCount, QDate date)
{
    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    QFont font;
    font.setBold(false);
    font.setPixelSize(13);

    QPen pen;
    pen.setStyle(Qt::SolidLine);

    //设置X轴坐标
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("yyyy/MM");
    axisX->setTitleFont(font);
    QDate startDate = date.addMonths(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    axisX->setRange(QDateTime(startDate), QDateTime(date));
    axisX->setTickCount(nXCount/2);
    axisX->setGridLineVisible(true);
    axisX->setGridLinePen(pen);
    axisX->setGridLineColor(QColor(250,250,250));
    axisX->setLabelsColor(QColor(250,250,250));
    axisX->setLabelsFont(font);
    //设置Y轴坐标
    int nMaxY = -1;
    QLineSeries *series1 = createSeries1M2(nXCount, date, nMaxY);
    double decimal = double(nMaxY) / 100.0;
    int nVal = qCeil(decimal); //向上取整
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleFont(font);
    axisY->setRange(0, nVal * 100);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(5);
    axisY->setGridLineVisible(true);
    axisY->setLabelsColor(QColor(250,250,250));
    axisY->setGridLinePen(pen);
    axisY->setGridLineColor(QColor(250,250,250));
    axisY->setLabelsFont(font);

    //添加线到图表,注意需要先设置x轴和y轴
    chart->addSeries(series1);
    chart->setAxisX(axisX, series1);
    chart->setAxisY(axisY, series1);
    chart->legend()->hide();

    m_monthwid2 = new QChartView(chart);
    m_monthwid2->setRenderHint(QPainter::Antialiasing); //设置抗锯齿
}

QLineSeries *OverWidget::createSeries1M2(int nXCount, QDate date,int &YMax)
{
    QLineSeries *series = new QLineSeries();
    QPen pen;
    pen.setWidth(2);
    series->setPen(pen);
    series->setColor(QColor(0,255,170));

    QDate startDate = date.addMonths(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    QList<QDate> listDate;
    for (int i = 0; i < nXCount; i++)
    {
        listDate.append(startDate.addMonths(i));
    }

    for (int j = 0; j < listDate.count(); j++)
    {
        QDate dCurrDate = listDate.at(j);
        QString sCurrDate = dCurrDate.toString("yyyy-MM");
        //
        int icc = 0;
        {
            char strSql[256] = {};
            sprintf(strSql, "SELECT count(*) FROM Task ");
            QString strTemp = strSql;
            sprintf(strSql, "WHERE DetectionTime >= '%s-01 00:00:00' and DetectionTime <= '%s-31 23:59:59' and DetectionResult = '不合格'", sCurrDate.toStdString().c_str(), sCurrDate.toStdString().c_str());
            QString strWhere = QString::fromLocal8Bit(strSql);
            strTemp += strWhere;
            QSqlQuery query(g_DB);
            query.prepare(strTemp);
            if (!query.exec())
            {
                QString querys = query.lastError().text();
                OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
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
                    while (query.next())
                    {
                        icc = query.value(0).toInt();
                    }
                }
            }
            if (icc > YMax)
                YMax = icc;
            qDebug() << sCurrDate << "     " << icc;
        }
        series->append(QPointF(QDateTime(dCurrDate).toMSecsSinceEpoch(), icc));
    }

    QFont font;
    font.setBold(true);
    font.setPixelSize(12);

    series->setVisible(true);
    series->setPointsVisible(true);
    series->setPointLabelsFormat("    @yPoint");
    series->setPointLabelsFont(font);
    series->setPointLabelsColor(QColor(0,255,170));
    series->setPointLabelsClipping(false);
    series->setPointLabelsVisible(true);

    return series;
}

void OverWidget::createChartSplineSeries2(int nXCount, QDate date)
{
    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    QFont font;
    font.setBold(false);
    font.setPixelSize(13);

    QPen pen;
    pen.setStyle(Qt::SolidLine);

    //设置X轴坐标
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("MM/dd");
    axisX->setTitleFont(font);
    QDate startDate = date.addDays(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    axisX->setRange(QDateTime(startDate), QDateTime(date));
    axisX->setTickCount(nXCount/5);
    axisX->setGridLineVisible(true);
    axisX->setGridLinePen(pen);
    axisX->setGridLineColor(QColor(250,250,250));
    axisX->setLabelsColor(QColor(250,250,250));
    axisX->setLabelsFont(font);
    //设置Y轴坐标
    int nMaxY = -1;
    QLineSeries *series1 = createSeries12(nXCount, date,nMaxY);
    double decimal = double(nMaxY) / 10.0;
    int nVal = qCeil(decimal); //向上取整
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleFont(font);
    axisY->setRange(0, nVal * 10);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(5);
    axisY->setGridLineVisible(true);
    axisY->setLabelsColor(QColor(250,250,250));
    axisY->setGridLinePen(pen);
    axisY->setGridLineColor(QColor(250,250,250));
    axisY->setLabelsFont(font);
    //添加线到图表,注意需要先设置x轴和y轴
    chart->addSeries(series1);
    chart->setAxisX(axisX, series1);
    chart->setAxisY(axisY, series1);
    chart->legend()->hide();
    m_daywid2 = new QChartView(chart);
    m_daywid2->setRenderHint(QPainter::Antialiasing); //设置抗锯齿
}

QLineSeries *OverWidget::createSeries12(int nXCount, QDate date,int &YMax)
{
    QLineSeries *series = new QLineSeries();
    QPen pen;
    pen.setWidth(2);
    series->setPen(pen);
    series->setColor(QColor(0,255,170));

    QDate startDate = date.addDays(-(nXCount - 1)); //用当前日期减去想查看的天数,得到起始日期
    QList<QDate> listDate;
    for (int i = 0; i < nXCount; i++)
    {
        listDate.append(startDate.addDays(i));
    }

    for (int j = 0; j < listDate.count(); j++)
    {
        QDate dCurrDate = listDate.at(j);
        QString sCurrDate = dCurrDate.toString("yyyy-MM-dd");
        //
        int icc = 0;
        {
            char strSql[256] = {};
            sprintf(strSql, "SELECT count(*) FROM Task ");
            QString strTemp = strSql;
            sprintf(strSql, "WHERE DetectionTime >= '%s 00:00:00' and DetectionTime <= '%s 23:59:59' and DetectionResult = '不合格'", sCurrDate.toStdString().c_str(), sCurrDate.toStdString().c_str());
            QString strWhere = QString::fromLocal8Bit(strSql);
            strTemp += strWhere;
            QSqlQuery query(g_DB);
            query.prepare(strTemp);
            if (!query.exec())
            {
                QString querys = query.lastError().text();
                OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
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
                    while (query.next())
                    {
                        icc = query.value(0).toInt();
                    }
                }
            }
            if (icc > YMax)
                YMax = icc;
            qDebug() << sCurrDate << "     " << icc;
        }
        series->append(QPointF(QDateTime(dCurrDate).toMSecsSinceEpoch(), icc));
    }

    QFont font;
    font.setBold(true);
    font.setPixelSize(10);

    series->setVisible(true);
    series->setPointsVisible(true);
    series->setPointLabelsFormat("    @yPoint");
    series->setPointLabelsFont(font);
    series->setPointLabelsColor(QColor(0,255,170));
    series->setPointLabelsClipping(false);
    series->setPointLabelsVisible(true);

    return series;
}

void OverWidget::slot_timeout_pre_pointer()
{
    float nPos = 0;
    nPos = PlayM4_GetPlayPos(m_PrePort);
    OutputLog2("pre Be playing... %.2f", nPos);
    if(nPos > 0.98)
        onStop();
//#ifdef WIN32
//    float fpos = libvlc_media_player_get_position(m_Prevlc_player);
//    int istate = libvlc_media_player_get_state(m_Prevlc_player);
////    qDebug() << fpos << "  " << istate;
//    OutputLog2("   pre Be playing... %.2f  %d", fpos, istate);
//    if(fpos > 0.98 || istate == 6 || istate == libvlc_Error)
//        onStop();
//#endif

}

void OverWidget::slot_timeout_pointer()
{
    float nPos = 0;
    nPos = PlayM4_GetPlayPos(m_AfrPort);
    OutputLog2("afr Be playing... %.2f", nPos);
    if(nPos > 0.98)
        onStop(true);

//#ifdef WIN32
//    float fpos = libvlc_media_player_get_position(m_vlc_player);
//    int istate = libvlc_media_player_get_state(m_vlc_player);
////    qDebug() << fpos << "  " << istate;
//    OutputLog2("    Be playing... %.2f  %d", fpos, istate);
//    if(fpos > 0.98 || istate == 6 || istate == libvlc_Error)
//        onStop(true);
//#endif

}

void OverWidget::ProcessData(int iLineID, int iSensorID, int iSn, int iValue)
{
#ifdef WIN32
    SendData(iLineID,iSensorID,iSn,iValue);
#endif
}

void OverWidget::initState()
{
    m_statewid = new QWidget;
    //3
    QFont font("Microsoft YaHei", 13); // 微软雅黑
    QFont font2("Microsoft YaHei", 11); // 微软雅黑
    m_stateview = new QTableView;
    m_stateview->verticalHeader()->hide();							// 隐藏垂直表头
    m_stateview->horizontalHeader()->setFont(font);
    m_stateview->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);	//根据内容自动调整大小
    m_stateview->horizontalHeader()->setStretchLastSection(true);		// 伸缩最后一列
    m_stateview->horizontalHeader()->setHighlightSections(false);		// 点击表时不对表头行光亮
    m_stateview->setShowGrid(false);										// 隐藏网格线
    m_stateview->setAlternatingRowColors(true);							// 开启隔行异色
    m_stateview->setFocusPolicy(Qt::NoFocus);							// 去除当前Cell周边虚线框
    m_stateview->horizontalHeader()->setMinimumSectionSize(120);
    m_stateview->setFont(font2);
    m_stateview->setSelectionBehavior(QAbstractItemView::SelectRows);			//为整行选中
    m_stateview->setSelectionMode(QAbstractItemView::SingleSelection);			//不可多选
    m_stateview->setEditTriggers(QAbstractItemView::NoEditTriggers);				//只读属性，即不能编辑
    m_stateview->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);		//滑动像素
    m_stateview->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);		//滑动像素
    m_stateview->setFrameShape(QFrame::NoFrame);

    m_statemode = new QStandardItemModel(m_realwid);
    m_stateview->setModel(m_statemode);
    m_statemode->setColumnCount(4);
    m_statemode->setHeaderData(0, Qt::Horizontal, QStringLiteral("序号"));
    m_statemode->setHeaderData(1, Qt::Horizontal, QStringLiteral("时间"));
    m_statemode->setHeaderData(2, Qt::Horizontal, QStringLiteral("名称"));
    m_statemode->setHeaderData(3, Qt::Horizontal, QStringLiteral("状态"));

    m_statequerybut = new QPushButton;
    m_statequerybut->setText(QStringLiteral(" 查 询 "));
    m_statequerybut->setFixedSize(80, 30);
    m_statequerybut->setFont(font);
    QObject::connect(m_statequerybut, SIGNAL(clicked()), this, SLOT(querystate()));

    m_statedhlay1 = new QHBoxLayout;
    m_statedhlay1->addStretch();
    m_statedhlay1->addWidget(m_statequerybut);
    m_statedhlay1->setMargin(5);

    m_mainlaystate = new QVBoxLayout;
    m_mainlaystate->addLayout(m_statedhlay1);
    m_mainlaystate->addWidget(m_stateview);
    m_mainlaystate->setSpacing(10);
    m_mainlaystate->setMargin(0);
    m_statewid->setLayout(m_mainlaystate);
}

void OverWidget::initStatistics()
{
    //设置图标底色和背景色一样
    QPalette myPalette;
    QColor myColor(0, 0, 0);
    myColor.setAlphaF(0);
    myPalette.setBrush(backgroundRole(), myColor);

    m_Statisticswid = new QWidget;
    //1
    m_totalwid = new QWidget;
    m_totalwid->setWindowFlags(Qt::FramelessWindowHint);
    m_totalwid->setAutoFillBackground(true);
    QPalette Palette;
    Palette.setColor(QPalette::Background, QColor(0, 185, 245,40));
    m_totalwid->setPalette(Palette);
    m_numlab = new QLabel(QStringLiteral("过车数量/辆"));
    m_numlab->setObjectName("s1");
    m_numlcd = new QLCDNumber;
    m_numlcd->setSegmentStyle(QLCDNumber::Flat);
    m_numlcd->setStyleSheet("background-color:transparent;color:lightgreen");
    m_numlcd->display(0);
    m_numlcd->setFixedSize(200,50);
    m_totallay = new QHBoxLayout;
    m_totallay->addStretch();
    m_totallay->addWidget(m_numlab);
    m_totallay->addWidget(m_numlcd);
    m_totallay->addStretch();
    m_totallay->setSpacing(10);
    m_totallay->setMargin(10);
    m_totalwid->setLayout(m_totallay);
    //2
    initPieOK();
    //3
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString s = current_date_time.toString("yyyy-MM-dd");
    QDate dCurrDate = QDate::fromString(s, "yyyy-MM-dd");
    createChartSplineSeries(30, dCurrDate);
    createChartSplineSeriesM(12, dCurrDate);
    createChartSplineSeriesY(5, dCurrDate);

    m_checknumlab = new QLabel(QStringLiteral("检 测 量"));
    m_checknumlab->setObjectName("s2");
    m_dtime = new CustomPushButton;
    m_dtime->setButtonBackground((":/pnd/png/d-press.png"));
    m_mtime = new CustomPushButton;
    m_mtime->setButtonBackground((":/pnd/png/m.png"));
    m_ytime = new CustomPushButton;
    m_ytime->setButtonBackground((":/pnd/png/y.png"));
    m_dtime->setPalette(myPalette);
    m_mtime->setPalette(myPalette);
    m_ytime->setPalette(myPalette);
    QObject::connect(m_dtime, SIGNAL(clicked()), this, SLOT(cdslot()));
    QObject::connect(m_mtime, SIGNAL(clicked()), this, SLOT(cmslot()));
    QObject::connect(m_ytime, SIGNAL(clicked()), this, SLOT(cyslot()));
    m_topchecklay = new QHBoxLayout;
    m_topchecklay->addWidget(m_checknumlab);
    m_topchecklay->addStretch();
    m_topchecklay->addWidget(m_dtime);
    m_topchecklay->addWidget(m_mtime);
    m_topchecklay->addWidget(m_ytime);
    m_topchecklay->setMargin(0);
    m_topchecklay->setSpacing(2);
    m_midchecklay = new QHBoxLayout;
    m_midchecklay->addWidget(m_monthwid);
    m_midchecklay->addWidget(m_daywid);
    m_midchecklay->addWidget(m_yearwid);
    m_midchecklay->setMargin(0);
    m_midchecklay->setSpacing(0);
    m_monthwid->hide();
    m_yearwid->hide();
    m_daywid->show();
    m_checklayout = new QVBoxLayout;
    m_checklayout->addLayout(m_topchecklay);
    m_checklayout->addLayout(m_midchecklay);
    m_checklayout->setMargin(10);
    m_checklayout->setSpacing(10);
    m_checknumwid = new QWidget;
    m_checknumwid->setWindowFlags(Qt::FramelessWindowHint);
    m_checknumwid->setAutoFillBackground(true);
    m_checknumwid->setPalette(Palette);
    m_checknumwid->setLayout(m_checklayout);

    //4
    m_checkokwid = new QWidget;
    m_checkokwid->setWindowFlags(Qt::FramelessWindowHint);
    m_checkokwid->setAutoFillBackground(true);
    m_checkokwid->setPalette(Palette);
    createChartSplineSeries2(30, dCurrDate);
    createChartSplineSeriesM2(12, dCurrDate);
    createChartSplineSeriesY2(5, dCurrDate);

    m_checknumlab2 = new QLabel(QStringLiteral("超 限 量"));
    m_checknumlab2->setObjectName("s2");
    m_dtime2 = new CustomPushButton;
    m_dtime2->setButtonBackground((":/pnd/png/d-press.png"));
    m_mtime2 = new CustomPushButton;
    m_mtime2->setButtonBackground((":/pnd/png/m.png"));
    m_ytime2 = new CustomPushButton;
    m_ytime2->setButtonBackground((":/pnd/png/y.png"));
    m_dtime2->setPalette(myPalette);
    m_mtime2->setPalette(myPalette);
    m_ytime2->setPalette(myPalette);
    QObject::connect(m_dtime2, SIGNAL(clicked()), this, SLOT(cdslot2()));
    QObject::connect(m_mtime2, SIGNAL(clicked()), this, SLOT(cmslot2()));
    QObject::connect(m_ytime2, SIGNAL(clicked()), this, SLOT(cyslot2()));
    m_topchecklay2 = new QHBoxLayout;
    m_topchecklay2->addWidget(m_checknumlab2);
    m_topchecklay2->addStretch();
    m_topchecklay2->addWidget(m_dtime2);
    m_topchecklay2->addWidget(m_mtime2);
    m_topchecklay2->addWidget(m_ytime2);
    m_topchecklay2->setMargin(0);
    m_topchecklay2->setSpacing(2);
    m_midchecklay2 = new QHBoxLayout;
    m_midchecklay2->addWidget(m_monthwid2);
    m_midchecklay2->addWidget(m_daywid2);
    m_midchecklay2->addWidget(m_yearwid2);
    m_midchecklay2->setMargin(0);
    m_midchecklay2->setSpacing(0);
    m_monthwid2->hide();
    m_yearwid2->hide();
    m_daywid2->show();
    m_checklayout2 = new QVBoxLayout;
    m_checklayout2->addLayout(m_topchecklay2);
    m_checklayout2->addLayout(m_midchecklay2);
    m_checklayout2->setMargin(10);
    m_checklayout2->setSpacing(10);
    m_checkokwid->setLayout(m_checklayout2);

    m_Statisticsgridlay = new QGridLayout;
    m_Statisticsgridlay->addWidget(m_totalwid,0,0);
    m_Statisticsgridlay->addWidget(m_piewid2,0,1);
    m_Statisticsgridlay->addWidget(m_checknumwid,1,0);
    m_Statisticsgridlay->addWidget(m_checkokwid,1,1);
    m_Statisticsgridlay->setSpacing(10);
    m_Statisticsgridlay->setMargin(0);
    m_Statisticswid->setLayout(m_Statisticsgridlay);
}

void OverWidget::initPieCar()
{
    //绘制饼图
    m_series = new QPieSeries();
    //total
    int iTotalNum = 0;
    {
        QString strSelect = "SELECT count(*) FROM Task";
        QSqlQuery query(g_DB);
        query.prepare(strSelect);
        if (!query.exec())
        {
            QString querys = query.lastError().text();
            OutputLog2("查询CarType记录失败 %s",querys.toStdString().c_str());
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
                while (query.next())
                {
                    iTotalNum = query.value(0).toInt();
                }
            }
        }
    }
    OutputLog2("总数%d",iTotalNum);
    m_numlcd->display(iTotalNum);
    char strSql[256] = {};
    sprintf(strSql, "SELECT distinct(CarType) FROM Task");
    int rownum = 0;
    QSqlQuery query(g_DB);
    query.prepare(strSql);
    if (!query.exec())
    {
        QString querys = query.lastError().text();
        OutputLog("CarType记录查询失败%s",querys.toStdString().c_str());
        return;
    }
    else
    {
        int iCount = 0;
        if (query.last())
        {
            iCount = query.at() + 1;
            query.first();//重新定位指针到结果集首位
            query.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
            {
                while (query.next())
                {
                    QString strTmp = query.value(0).toString();
                    {
                        QString strSelect = "SELECT count(*) FROM Task where CarType = :CarType";
                        QSqlQuery query(g_DB);
                        query.prepare(strSelect);
                        query.bindValue(":CarType", strTmp);
                        if (!query.exec())
                        {
                            QString querys = query.lastError().text();
                            OutputLog2("查询CarType记录失败 %s",querys.toStdString().c_str());
                        }
                        else
                        {
                            int nCount = -1;
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
                                    nCount = query.value(0).toInt();
                                }
                                m_series->append(strTmp,nCount);
                                OutputLog2("%s %d",(const char*)strTmp.toLocal8Bit(), nCount);
                            }
                        }
                    }
                    rownum++;
                }
            }
        }
    }
    m_series->setHoleSize(0.2);//孔大小0.0-1.0
    m_series->setHorizontalPosition(0.5);//水平位置，默认0.5，0.0-1.0
    m_series->setLabelsPosition(QPieSlice::LabelOutside);
    m_series->setLabelsVisible(true);
    m_series->setPieSize(1.0);//饼图尺寸，默认0.7
    m_series->setPieStartAngle(0);//0度为12点钟方向
    m_series->setVerticalPosition(0.5);
    m_series->setVisible(true);

    QChart *chart = new QChart();
    chart->setAutoFillBackground(true);
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->addSeries(m_series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
//    chart->setTheme(QChart::ChartThemeBlueCerulean);//设置系统主题
    chart->setAnimationOptions(QChart::AllAnimations);//设置启用或禁用动画
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->setLocalizeNumbers(true);//数字是否本地化

    chart->legend()->setAlignment(Qt::AlignLeft);//对齐
    chart->legend()->setLabelColor(QColor(255,255,255));//设置标签颜色
    chart->legend()->setVisible(true);//设置是否可视
    QFont font = chart->legend()->font();
    font.setItalic(!font.italic());
    chart->legend()->setFont(font);//设置字体为斜体
    font.setPointSizeF(13);
    chart->legend()->setFont(font);//设置字体大小
    chart->legend()->setFont(QFont("微软雅黑"));//设置字体类型
    chart->legend()->setBorderColor(QColor(2,77,148));//设置边框颜色

    for(int i = 0; i< rownum; i++)
    {
        //操作单个切片
        QPieSlice *slice1 = m_series->slices().at(i);
        slice1->setLabelVisible(true);
        if(i==0)
        {
            slice1->setExploded();//切片是否与饼图分离
            slice1->setLabelColor(QColor(0,0,250));
            slice1->setColor(QColor(0,0,250));
        }
        else if(i==1)
        {
            slice1->setLabelColor(QColor(170,170,255));
            slice1->setColor(QColor(170,170,255));
        }
        else if(i==2)
        {
            slice1->setLabelColor(QColor(0,170,170));
            slice1->setColor(QColor(0,170,170));
        }
        else if(i==3)
        {
            slice1->setLabelColor(QColor(0,255,170));
            slice1->setColor(QColor(0,255,170));
        }
        else if(i==4)
        {
            slice1->setLabelColor(QColor(255,255,0));
            slice1->setColor(QColor(255,255,0));
        }
        else if(i==5)
        {
            slice1->setLabelColor(QColor(0,0,0));
            slice1->setColor(QColor(0,0,0));
        }
        else if(i==6)
        {
            slice1->setLabelColor(QColor(0,250,0));
            slice1->setColor(QColor(0,250,0));
        }
        else
        {
            slice1->setLabelColor(QColor(255,220,0));
            slice1->setColor(QColor(255,220,0));
        }
        QString str = slice1->label();
        QString hh=" ";
        str.append(hh);
        str = str + (QString::number(slice1->value()/iTotalNum*100,'f',2)) + "%";
        slice1->setLabel(str);
        slice1->setBorderColor(QColor(2,77,148));
        slice1->setLabelFont(QFont("微软雅黑"));
        slice1->setLabelVisible(false);
    }

    m_piewid = new QChartView(chart);
    m_piewid->setFixedSize(this->width()/2,(this->height()-60)/3);
//    m_piewid->setAutoFillBackground(true);
    m_piewid->setRenderHint(QPainter::Antialiasing);
}

void OverWidget::initPieOK()
{
    //total
    int iTotalNum = 0;
    {
        QString strSelect = "SELECT count(*) FROM Task";
        QSqlQuery query(g_DB);
        query.prepare(strSelect);
        if (!query.exec())
        {
            QString querys = query.lastError().text();
            OutputLog2("查询All记录失败 %s",querys.toStdString().c_str());
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
                while (query.next())
                {
                    iTotalNum = query.value(0).toInt();
                }
            }
        }
    }
    OutputLog2("总数%d",iTotalNum);
    m_numlcd->display(iTotalNum);
    int iTotalNumOk = 0;
    {
        QString strSelect = QString("SELECT count(*) FROM Task where DetectionResult = '%1'").arg(QStringLiteral("合格"));
        QSqlQuery query(g_DB);
        query.prepare(strSelect);
        if (!query.exec())
        {
            QString querys = query.lastError().text();
            OutputLog2("查询合格记录失败 %s",querys.toStdString().c_str());
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
                while (query.next())
                {
                    iTotalNumOk = query.value(0).toInt();
                }
            }
        }
    }
    int iTotalNumUnOk = iTotalNum - iTotalNumOk;
    //绘制饼图
    m_series2 = new QPieSeries();
    m_series2->append(QStringLiteral("合格"),iTotalNumOk);
    m_series2->append(QStringLiteral("不合格"),iTotalNumUnOk);
    m_series2->setHoleSize(0.2);//孔大小0.0-1.0
    m_series2->setHorizontalPosition(0.5);//水平位置，默认0.5，0.0-1.0
    m_series2->setLabelsPosition(QPieSlice::LabelOutside);
    m_series2->setLabelsVisible(true);
    m_series2->setPieSize(0.6);//饼图尺寸，默认0.7
    m_series2->setPieStartAngle(0);//0度为12点钟方向
    m_series2->setVerticalPosition(0.6);
    m_series2->setVisible(true);

    QChart *chart = new QChart();
    chart->setAutoFillBackground(true);
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundRoundness(0);				//设置背景区域无圆角
    chart->addSeries(m_series2);
    chart->setAnimationOptions(QChart::AllAnimations);//设置启用或禁用动画
    chart->setBackgroundBrush(QBrush(QColor(2,77,148)));//设置背景色,主题和背景二选一
    chart->setLocalizeNumbers(true);//数字是否本地化
    chart->legend()->setVisible(false);//设置是否可视

    //操作单个切片
    QPieSlice *slice1 = m_series2->slices().at(0);
    slice1->setLabel(slice1->label()+" "+(QString::number(slice1->value()/iTotalNum*100,'f',2)) + "%");
    slice1->setLabelColor(QColor(0,255,170));
    slice1->setColor(QColor(0,255,170));
    slice1->setBorderColor(QColor(2,77,148));
    slice1->setLabelFont(QFont("微软雅黑"));
    slice1->setLabelVisible(true);

    QPieSlice *slice2 = m_series2->slices().at(1);
    slice2->setExploded();//切片是否与饼图分离
    slice2->setLabel(slice2->label()+" "+(QString::number(slice2->value()/iTotalNum*100,'f',2)) + "%");
    slice2->setBorderColor(QColor(2,77,148));
    slice2->setLabelColor(QColor(255,170,255));
    slice2->setColor(QColor(255,170,255));
    slice2->setLabelFont(QFont("微软雅黑"));
    slice2->setLabelVisible(true);

    m_piewid2 = new QChartView(chart);
    m_piewid2->setFixedSize(this->width()/2,(this->height()-60)/3);
    m_piewid2->setRenderHint(QPainter::Antialiasing);
}

int OverWidget::StartLED()
{
    char strVal[100] = {};
    GetPrivateProfileString("LED", "Height", "100", strVal, 100, strConfigfile.c_str());
    m_iLedH = atoi(strVal);
    GetPrivateProfileString("LED", "Width", "200", strVal, 100, strConfigfile.c_str());
    m_iLedW = atoi(strVal);
    GetPrivateProfileString("LED", "Font", "18", strVal, 100, strConfigfile.c_str());
    m_iLedFont = atoi(strVal);
    GetPrivateProfileString("LED", "type", "0", strVal, 100, strConfigfile.c_str());
    m_iLedType = atoi(strVal);
    if(m_iLedType == 1)
    {
        DeviceNode *node = new DeviceNode;
        memset(node,0,sizeof(DeviceNode));
        node->Type = 0;
        GetPrivateProfileString("SYS", "LEDNO", "", strVal, 99, strConfigfile.c_str());
        strcpy(node->sDvrNo,strVal);

        m_Ledwid = new QWidget;
        m_Ledwid->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        m_Ledwid->setAutoFillBackground(true);
        m_Ledwid->setFixedSize(m_iLedW,m_iLedH);
        m_Ledwid->setGeometry(0, 0, m_iLedW,m_iLedH);
        m_UP1 = new QLabel(QString::fromLocal8Bit(m_szL1));
        m_UP1->setStyleSheet(QString("background-color:black; color:green;font-family:\"Microsoft YaHei\";font-size:%1px;").arg(m_iLedFont));
        m_UP2 = new QLabel(QString::fromLocal8Bit(m_szL2));
        m_UP2->setStyleSheet(QString("background-color:black; color:yellow;font-family:\"Microsoft YaHei\";font-size:%1px;").arg(m_iLedFont));
        m_DOWN = new QLabel(QString::fromLocal8Bit(m_szL3));
        m_DOWN->setStyleSheet(QString("background-color:black; color:green;font-family:\"Microsoft YaHei\";font-size:%1px;").arg(m_iLedFont+2));
        m_UP1->setFixedHeight(m_iLedH/3);
        m_UP2->setFixedHeight(m_iLedH/3);
        m_DOWN->setFixedHeight(m_iLedH/3+m_iLedH%3);
        m_UP1->setAlignment(Qt::AlignCenter);
        m_UP2->setAlignment(Qt::AlignCenter);
        m_DOWN->setAlignment(Qt::AlignCenter);

        m_ledlay = new QVBoxLayout;
        m_ledlay->addWidget(m_UP1);
        m_ledlay->addWidget(m_UP2);
        m_ledlay->addWidget(m_DOWN);
        m_ledlay->setSpacing(0);
        m_ledlay->setMargin(0);
        m_Ledwid->setLayout(m_ledlay);
        m_Ledwid->show();

        node->bIsOnline = NORMAL;
        lstDevice.push_back(node);
        pthread_t m_dwHeartID;//线程ID
        pthread_create(&m_dwHeartID, nullptr, &FlushThreadProc, this);
    }
    else
    {
#ifdef WIN32
        if(::GetPrivateProfileInt("SYS", "IsLED", 1, strConfigfile.c_str()) == 0)
            return 0;

        DeviceNode *node = new DeviceNode;
        memset(node,0,sizeof(DeviceNode));
        node->Type = 0;
        GetPrivateProfileString("SYS", "LEDNO", "", strVal, 99, strConfigfile.c_str());
        strcpy(node->sDvrNo,strVal);

        if (User_OpenScreen(m_iCardNum))
        {
            node->bIsOnline = NORMAL;
            OutputLog2("LED打开成功");
            if (!User_RealtimeScreenClear(m_iCardNum))
                OutputLog2("清空LED控制卡失败！");
            else
                OutputLog2("清空LED控制卡成功！");

            if (!User_RealtimeConnect(m_iCardNum))
                OutputLog2("LED建立连接失败！");
            else
                OutputLog2("LED建立连接成功！");

            SendLEDTextB(1, m_szL1);
            SendLEDTextB(2, m_szL2);
            SendLEDTextB(3, m_szL3);
            if (!User_RealtimeDisConnect(m_iCardNum))
                OutputLog2("LED断开连接失败！");
            else
                OutputLog2("LED断开连接成功！");

            //多发一次试试
            Sleep(100);
            if (!User_RealtimeConnect(m_iCardNum))
                OutputLog2("LED建立连接失败！");
            else
                OutputLog2("LED建立连接成功！");

            SendLEDTextB(1, m_szL1);
            SendLEDTextB(2, m_szL2);
            SendLEDTextB(3, m_szL3);
            if (!User_RealtimeDisConnect(m_iCardNum))
                OutputLog2("LED断开连接失败！");
            else
                OutputLog2("LED断开连接成功！");

            pthread_t m_dwHeartID;//线程ID
            pthread_create(&m_dwHeartID, nullptr, &FlushThreadProc, this);

        }
        else
        {
            node->bIsOnline = UNCONNECT;
            OutputLog2("LED打开失败");
        }
        lstDevice.push_back(node);
#endif
    }

    return 1;
}

void OverWidget::ResetLed()
{
    char szTemp[10] = "";
    if(m_iLedType == 1)
    {
        emit LedUpdate(0,szTemp,szTemp);
    }
    else
    {
#ifdef WIN32
        if(::GetPrivateProfileInt("SYS", "IsLED", 1, strConfigfile.c_str()) == 0)
            return;

        m_QueueSectLED.Lock();
        if (!User_RealtimeConnect(m_iCardNum))
            OutputLog2("LED建立连接失败！");
        else
            OutputLog2("LED建立连接成功！");

        SendLEDTextB(1, m_szL1);
        SendLEDTextB(2, m_szL2);
        SendLEDTextB(3, m_szL3);
        if (!User_RealtimeDisConnect(m_iCardNum))
            OutputLog2("LED断开连接失败！");
        else
            OutputLog2("LED断开连接成功！");
        m_QueueSectLED.Unlock();
#endif
    }
}

void OverWidget::UpdateLed(STR_MSG *msgdata)
{
    if(m_iLedType == 1)
    {
        emit LedUpdate(1,msgdata->strcarno,msgdata->strret);
    }
    else
    {
#ifdef WIN32
        if(::GetPrivateProfileInt("SYS", "IsLED", 1, strConfigfile.c_str()) == 1)
        {
            m_QueueSectLED.Lock();
            if(0) //园区测试
            {
                if (::GetPrivateProfileIntA("SYS", "遥感", 0, strConfigfile.c_str()) == 1)
                {
                    char sPath[256] = {};
                    GetPrivateProfileString("SYS", "picpath", "D:", sPath, 100, strConfigfile.c_str());
                    //获取遥感
                    char ssPath[256] = {};
                    sprintf(ssPath, "%s\\%s.ini", sPath, msgdata->strcarno);
                    OutputLog2(ssPath);
                    if (file_exists(ssPath))
                        OutputLog2("文件存在");
                    else
                    {
                        if (::GetTickCount() - msgdata->getticout > m_Timeout)
                        {
                            OutputLog2("3s等待尾气超时,本次无效 %s", msgdata->strcarno);
                            memset(msgdata, 0, sizeof(STR_MSG));
                        }
                        m_QueueSectLED.Unlock();
                        return;
                    }

                    char sVal[256] = {};
                    std::string shc, sno, sco2, sco, syd, sret;
                    GetPrivateProfileString("GAS", "co", "0.1", sVal, 29, ssPath);
                    sco = sVal;
                    GetPrivateProfileString("GAS", "co2", "12.0", sVal, 29, ssPath);
                    sco2 = sVal;
                    GetPrivateProfileString("GAS", "hc", "1200", sVal, 29, ssPath);
                    shc = sVal;
                    GetPrivateProfileString("GAS", "no", "450", sVal, 19, ssPath);
                    sno = sVal;
                    GetPrivateProfileString("GAS", "yd", "450", sVal, 19, ssPath);
                    syd = sVal;
                    GetPrivateProfileString("GAS", "ret", "合格", sVal, 19, ssPath);
                    sret = sVal;
                    DeleteFileA(ssPath);
                    //upload
                    OutputLog2("尾气准备完成 车道%d  %s", msgdata->laneno, msgdata->strcarno);
                    if (!User_RealtimeConnect(m_iCardNum))
                        OutputLog2("LED建立连接失败！");
                    else
                        OutputLog2("LED建立连接成功！");
                    char szTemp[200] = {};
                    SendLEDText5(1, msgdata->strcarno);
                    sprintf(szTemp, "总重量%dkg", msgdata->weight);
                    SendLEDText5(2, szTemp);
                    sprintf(szTemp, "HC:%s CO2:%s", shc.c_str(), sco2.c_str());
                    SendLEDText5(3, szTemp);
                    sprintf(szTemp, "CO:%s NOX:%s", sco.c_str(), sno.c_str());
                    SendLEDText5(4, szTemp);
                    sprintf(szTemp, "重量%s 尾气%s", msgdata->strret, sret.c_str());
                    SendLEDText5(5, szTemp);
                    SendLEDText5(6, "本数据仅作为内部测试使用");
                    if (!User_RealtimeDisConnect(m_iCardNum))
                        OutputLog2("LED断开连接失败！");
                    else
                        OutputLog2("LED断开连接成功！");
                }
                else
                {
                    if (!User_RealtimeConnect(m_iCardNum))
                        OutputLog2("LED建立连接失败！");
                    else
                        OutputLog2("LED建立连接成功！");
                    //园区测试
                    if (msgdata->axiscount < 3)
                    {
                        char szTemp[200] = {};
                        SendLEDText4(1, msgdata->strcarno);
                        sprintf(szTemp, "总重量%dkg", msgdata->weight);
                        SendLEDText4(2, szTemp);
                        SendLEDText4(3, msgdata->strret);
                        string str;
                        for (int i = 1; i <= msgdata->axiscount; i++)
                        {
                            sprintf(szTemp, "%d轴%d ", i, msgdata->Axis_weight[i - 1]);
                            str += szTemp;
                        }
                        SendLEDText4(4, str.c_str());
                        SendLEDText4(5, "本数据仅作为内部测试使用");
                    }
                    else
                    {
                        char szTemp[200] = {};
                        SendLEDText5(1, msgdata->strcarno);
                        sprintf(szTemp, "总重量%dkg", msgdata->weight);
                        SendLEDText5(2, szTemp);
                        SendLEDText5(3, msgdata->strret);
                        string str;
                        for (int i = 1; i <= 2; i++)
                        {
                            sprintf(szTemp, "%d轴%d ", i, msgdata->Axis_weight[i - 1]);
                            str += szTemp;
                        }
                        SendLEDText5(4, str.c_str());
                        str = "";
                        int axis = msgdata->axiscount > 4 ? 4 : msgdata->axiscount;
                        OutputLog2("axis %d", axis);
                        for (int i = 3; i <= axis; i++)
                        {
                            sprintf(szTemp, "%d轴%d ", i, msgdata->Axis_weight[i - 1]);
                            str += szTemp;
                        }
                        SendLEDText5(5, str.c_str());
                        SendLEDText5(6, "本数据仅作为内部测试使用");
                    }
                    //园区测试end
                    if (!User_RealtimeDisConnect(m_iCardNum))
                        OutputLog2("LED断开连接失败！");
                    else
                        OutputLog2("LED断开连接成功！");
                }
            }
            else
            {
                if (!User_RealtimeConnect(m_iCardNum))
                    OutputLog2("LED建立连接失败！");
                else
                    OutputLog2("LED建立连接成功！");
                SendLEDText(1, "治超检测");
                SendLEDText(2, msgdata->strcarno);
                if(m_showRet == 1)
                    SendLEDText(3, msgdata->strret);
                else
                    SendLEDText(3, " ");

                if (!User_RealtimeDisConnect(m_iCardNum))
                    OutputLog2("LED断开连接失败！");
                else
                    OutputLog2("LED断开连接成功！");
            }
            m_QueueSectLED.Unlock();
        }
#endif
    }
}

int OverWidget::SendLEDTextB(int nPosition, const char *str, bool bret)
{
#ifdef WIN32
    char szFont[10] = {};
    sprintf(szFont,"宋体");
    int iScreenX = 0;
    int iScreenY = 64;
    int iScreenWidth = 288;
    int iScreemHeight = 64;
    User_FontSet FontInfo;
    FontInfo.bFontBold = FALSE;
    FontInfo.bFontItaic = FALSE;
    FontInfo.bFontUnderline = FALSE;
    FontInfo.colorFont = 0x00FF;   //2 HONG
    FontInfo.iFontSize = 24;
    FontInfo.strFontName = szFont;
    FontInfo.iAlignStyle = 1;
    FontInfo.iVAlignerStyle = 1;
    FontInfo.iRowSpace = 0;
    if (nPosition == 3)
    {
        iScreenX = 0;
        iScreenY = 128;
        iScreenWidth = 288;
        iScreemHeight = 64;
    }
    else if (nPosition == 1)
    {
        iScreenX = 0;
        iScreenY = 0;
        iScreenWidth = 288;
        iScreemHeight = 64;
    }

    if (!User_RealtimeSendText(m_iCardNum, iScreenX, iScreenY, iScreenWidth, iScreemHeight, (char*)str, &FontInfo))
        OutputLog2("发送LED文本失败！");
    else
        OutputLog2("发送LED文本成功！");
#endif

    return 1;
}

int OverWidget::SendLEDText(int nPosition, const char *str, bool bret)
{
#ifdef WIN32
    char szFont[10] = {};
    sprintf(szFont,"宋体");
    int iScreenX = 0;
    int iScreenY = 64;
    int iScreenWidth = 288;
    int iScreemHeight = 64;
    User_FontSet FontInfo;
    FontInfo.bFontBold = FALSE;
    FontInfo.bFontItaic = FALSE;
    FontInfo.bFontUnderline = FALSE;
    FontInfo.colorFont = 0xFFFF;   //2 黄色
    FontInfo.iFontSize = 24;
    FontInfo.strFontName = szFont;
    FontInfo.iAlignStyle = 1;
    FontInfo.iVAlignerStyle = 1;
    FontInfo.iRowSpace = 0;
    if (nPosition == 3)
    {
        iScreenX = 0;
        iScreenY = 128;
        iScreenWidth = 288;
        iScreemHeight = 64;
        if(strstr(str,"不")!=NULL)
            FontInfo.colorFont = 0x00FF;
        else
            FontInfo.colorFont = 0xFF00;
    }
    else if (nPosition == 1)
    {
        iScreenX = 0;
        iScreenY = 0;
        iScreenWidth = 288;
        iScreemHeight = 64;
        FontInfo.colorFont = 0xFF00;   //1 绿色
    }

    if (!User_RealtimeSendText(m_iCardNum, iScreenX, iScreenY, iScreenWidth, iScreemHeight, (char*)str, &FontInfo))
        OutputLog2("发送LED文本失败！");
    else
        OutputLog2("发送LED文本成功！");
#endif

    return 1;
}

int OverWidget::SendLEDText4(int nPosition, const char *str, bool bret)
{
#ifdef WIN32
    char szFont[10] = {};
    sprintf(szFont,"宋体");
    int iScreenX = 0;
    int iScreenY = 42;
    int iScreenWidth = 288;
    int iScreemHeight = 42;
    User_FontSet FontInfo;
    FontInfo.bFontBold = FALSE;
    FontInfo.bFontItaic = FALSE;
    FontInfo.bFontUnderline = FALSE;
    FontInfo.colorFont = 0xFFFF;   //2 黄色
    FontInfo.iFontSize = 23;
    FontInfo.strFontName = szFont;
    FontInfo.iAlignStyle = 1;
    FontInfo.iVAlignerStyle = 1;
    FontInfo.iRowSpace = 0;
    if (nPosition == 3)
    {
        iScreenX = 0;
        iScreenY = 42 + 42;
        iScreenWidth = 288;
        iScreemHeight = 42;
        if(strstr(str,"不")!=NULL)
            FontInfo.colorFont = 0x00FF;
        else
            FontInfo.colorFont = 0xFF00;
    }
    else if (nPosition == 1)
    {
        iScreenX = 0;
        iScreenY = 0;
        iScreenWidth = 288;
        iScreemHeight = 42;
        FontInfo.colorFont = 0xFF00;   //1 绿色
    }
    else if (nPosition == 4)
    {
        iScreenX = 0;
        iScreenY = 42 + 42 + 42;
        iScreenWidth = 288;
        iScreemHeight = 42;
        FontInfo.colorFont = 0xFF00;   //1 绿色
    }
    else if (nPosition == 5)
    {
        iScreenX = 0;
        iScreenY = 42 + 42 + 42 + 42;
        iScreenWidth = 288;
        iScreemHeight = 24;
        FontInfo.colorFont = 0x00FF;   //1 绿色
        FontInfo.iFontSize = 10;
    }

    if (!User_RealtimeSendText(m_iCardNum, iScreenX, iScreenY, iScreenWidth, iScreemHeight, (char*)str, &FontInfo))
        OutputLog2("发送LED文本失败！");
    else
        OutputLog2("发送LED文本成功！");
#endif

    return 1;
}

int OverWidget::SendLEDText5(int nPosition, const char *str, bool bret)
{

#ifdef WIN32
    char szFont[10] = {};
    sprintf(szFont,"宋体");
    int iScreenX = 0;
    int iScreenY = 33;
    int iScreenWidth = 288;
    int iScreemHeight = 33;
    User_FontSet FontInfo;
    FontInfo.bFontBold = FALSE;
    FontInfo.bFontItaic = FALSE;
    FontInfo.bFontUnderline = FALSE;
    FontInfo.colorFont = 0xFFFF;   //2 黄色
    FontInfo.iFontSize = 22;
    FontInfo.strFontName = szFont;
    FontInfo.iAlignStyle = 1;
    FontInfo.iVAlignerStyle = 1;
    FontInfo.iRowSpace = 0;
    if (nPosition == 3)
    {
        iScreenX = 0;
        iScreenY = 33+33;
        iScreenWidth = 288;
        iScreemHeight = 33;
        if(strstr(str,"不")!=NULL)
            FontInfo.colorFont = 0x00FF;
        else
            FontInfo.colorFont = 0xFF00;
    }
    else if (nPosition == 1)
    {
        iScreenX = 0;
        iScreenY = 0;
        iScreenWidth = 288;
        iScreemHeight = 33;
        FontInfo.colorFont = 0xFF00;   //1 绿色
    }
    else if (nPosition == 4)
    {
        iScreenX = 0;
        iScreenY = 33+33+33;
        iScreenWidth = 288;
        iScreemHeight = 33;
        FontInfo.colorFont = 0xFF00;   //1 绿色
    }
    else if (nPosition == 5)
    {
        iScreenX = 0;
        iScreenY = 33+33+33+33;
        iScreenWidth = 288;
        iScreemHeight = 33;
        FontInfo.colorFont = 0xFF00;   //1 绿色
    }
    else if (nPosition == 6)
    {
        iScreenX = 0;
        iScreenY = 33 + 33 + 33 + 33+33;
        iScreenWidth = 288;
        iScreemHeight = 26;
        FontInfo.colorFont = 0x00FF;   //1 绿色
        FontInfo.iFontSize = 10;
    }

    if (!User_RealtimeSendText(m_iCardNum, iScreenX, iScreenY, iScreenWidth, iScreemHeight, (char*)str, &FontInfo))
        OutputLog2("发送LED文本失败！");
    else
        OutputLog2("发送LED文本成功！");
#endif

    return 1;
}

void* ThreadDetectHK(LPVOID param)
{
    std::string sPath = QCoreApplication::applicationDirPath().toStdString();
    std::string sfilePath = sPath + "/active.ini";
    int iCout = 0;
    OverWidget* pThis = reinterpret_cast<OverWidget*>(param);
    while(pThis->bStart)
    {
        Sleep(5000);
        pThis->DetectOnlineHK();
        WritePrivateProfileString("SYS", "Count", QString::number(iCout).toStdString().c_str(), sfilePath.c_str());
        iCout++;
        if(iCout > 100000)
            iCout=0;
    }

    return 0;
}

void* ReConnectDVR(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    //定时检测链表中未连接的DVR，直到所有的DVR都已连接成功过一次
    while(pThis->bStart)
    {
        Sleep(3000);
        if(pThis->ConnectAllDVR())
            break;
    }

    pThis->OutputLog2("ReConnectDVR exit...");
    return 0;
}

void* FlushThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    pThis->OutputLog2("刷新线程开启...");
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
    int rownum = m_mode->rowCount();
    //插入新记录
    m_mode->setItem(rownum, 0, new QStandardItem(QString::number(msg->dataId)));
    m_mode->setItem(rownum, 1, new QStandardItem(msg->strtime));
    m_mode->setItem(rownum, 2, new QStandardItem(QString::fromLocal8Bit(msg->strcarno)));
    m_mode->setItem(rownum, 3, new QStandardItem(QString::number(msg->laneno)));
    m_mode->setItem(rownum, 4, new QStandardItem(QString::number(msg->speed)));
    m_mode->setItem(rownum, 5, new QStandardItem(QString::number(msg->axiscount)));
    m_mode->setItem(rownum, 6, new QStandardItem(QString::number(msg->weight)));
    m_mode->setItem(rownum, 7, new QStandardItem(QString::fromLocal8Bit(msg->strplatecolor)));
    m_mode->setItem(rownum, 8, new QStandardItem(QString::number(msg->Axis_weight[0])));
    m_mode->setItem(rownum, 9, new QStandardItem(QString::number(msg->Axis_weight[1])));
    m_mode->setItem(rownum, 10, new QStandardItem(QString::number(msg->Axis_weight[2])));
    m_mode->setItem(rownum, 11, new QStandardItem(QString::number(msg->Axis_weight[3])));
    m_mode->setItem(rownum, 12, new QStandardItem(QString::number(msg->Axis_weight[4])));
    m_mode->setItem(rownum, 13, new QStandardItem(QString::number(msg->Axis_weight[5])));
    m_mode->setItem(rownum, 14, new QStandardItem(QString::number(msg->Axis_weight[6])));
    m_mode->setItem(rownum, 15, new QStandardItem(QString::number(msg->Axis_weight[7])));
    m_mode->item(rownum,0)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,1)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,2)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,3)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,4)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,5)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,6)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,7)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,8)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,9)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,10)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,11)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,12)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,13)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,14)->setTextAlignment(Qt::AlignCenter);
    m_mode->item(rownum,15)->setTextAlignment(Qt::AlignCenter);
    QModelIndex mdidx = m_mode->index(rownum, 1); //获得需要编辑的单元格的位置
    m_view->setCurrentIndex(mdidx);	//设定需要编辑的单元格

    m_cusPre->showpic(QString::fromLocal8Bit(msg->strprepic));
    m_cusAfr->showpic(QString::fromLocal8Bit(msg->strafrpic));
    m_cusCe->showpic(QString::fromLocal8Bit(msg->strsmallpic/*strcepic*/),1);
    m_cusAll->showpic(QString::fromLocal8Bit(msg->strcepic));
    m_lanelab2->setText(QString::number(msg->laneno));
    m_cphmlab2->setText(QString::fromLocal8Bit(msg->strcarno));
    m_curtimelab2->setText(QString::fromLocal8Bit(msg->strtime));
    m_axislab2->setText(QString::number(msg->axiscount));
    m_carlenlab2->setText(QString::number(msg->carlen));
    m_speedlab2->setText(QString::number(msg->speed));
    m_weightlab2->setText(QString::number(msg->weight));
}

void OverWidget::Flush(QString str)
{
    m_editlog->append(str);
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
    QString strTemp,strWhere;
    m_modehis->clearRecordList();
    QString S = m_startedit->text().left(10);
    QString E = m_endedit->text().left(10);

    char strSql[256] = {};
    sprintf(strSql, "SELECT * FROM Task ");
    strTemp = strSql;
    sprintf(strSql, "WHERE DetectionTime >= '%s 00:00:00' and DetectionTime <= '%s 23:59:59' ", S.toStdString().c_str(), E.toStdString().c_str());
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
    sprintf(strSql, " order by id desc");
    strWhere += strSql;

    strTemp += strWhere;
    QSqlQuery query(g_DB);
    query.prepare(strTemp);
    if (!query.exec())
    {
        QString querys = query.lastError().text();
        OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
        return;
    }
    else
    {
        int iCount = 0;
        if (query.last())
        {
            iCount = query.at() + 1;
            query.first();//重新定位指针到结果集首位
            query.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
            {
                while (query.next())
                {
                    ResultRecord record;
                    record.qstrfid = query.value(0).toString();
                    record.qstrtime = query.value(1).toString();
                    record.qstrcarno = query.value(2).toString();
                    record.qstrlane = query.value(3).toString();
                    record.qstraxis = query.value(4).toString();
                    record.qstrcartype = query.value(5).toString();
                    record.qstrspeed = query.value(6).toString();
                    record.qstrweight = query.value(7).toString();
                    record.qstrcolor = query.value(8).toString();
                    record.qstrweight1 = query.value(15).toString();
                    record.qstrweight2 = query.value(16).toString();
                    record.qstrweight3 = query.value(17).toString();
                    record.qstrweight4 = query.value(18).toString();
                    record.qstrweight5 = query.value(19).toString();
                    record.qstrweight6 = query.value(20).toString();
                    record.qstrweight7 = query.value(21).toString();
                    record.qstrweight8 = query.value(22).toString();

                    record.qstrpicvideo = query.value("vedioBaseFront").toString();
                    record.qstrpicpre = query.value("ImageBaseFront").toString();
                    record.qstrpicafr = query.value("ImageBaseBack").toString();
                    record.qstrpicce = query.value("ImageBaseSide").toString();
                    record.qstrpicsmall = query.value("ImageBaseCarNO").toString();
                    record.qstrs1 = query.value("AxlesSpace1").toString();
                    record.qstrs2 = query.value("AxlesSpace2").toString();
                    record.qstrs3 = query.value("AxlesSpace3").toString();
                    record.qstrs4 = query.value("AxlesSpace4").toString();
                    record.qstrs5 = query.value("AxlesSpace5").toString();
                    record.qstrs6 = query.value("AxlesSpace6").toString();
                    record.qstrs7 = query.value("AxlesSpace7").toString();
                    record.qstrcrossroad = query.value("crossroad").toString();
                    record.qstrdirection = query.value("direction").toString();
                    m_modehis->updateData(record);
                }
            }
            QMessageBox::information(NULL,QStringLiteral("总数"),QString::number(iCount));
        }
        else
        {
            m_viewhis->reset();
        }       
    }
}

void OverWidget::querystate()
{
    char strSql[256] = {};
    sprintf(strSql, "SELECT * FROM DEVSTATE ");

    QSqlQuery query(g_DB);
    query.prepare(strSql);
    if (!query.exec())
    {
        QString querys = query.lastError().text();
        OutputLog("状态记录查询失败%s",querys.toStdString().c_str());
        return;
    }
    else
    {
        int iCount = 0;
        if (query.last())
        {
            iCount = query.at() + 1;
            query.first();//重新定位指针到结果集首位
            query.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
            {
                int rownum = 0;
                while (query.next())
                {
                    //插入新记录
                    m_statemode->setItem(rownum, 0, new QStandardItem(query.value(0).toString()));
                    m_statemode->setItem(rownum, 1, new QStandardItem(query.value(1).toString()));
                    m_statemode->setItem(rownum, 2, new QStandardItem(query.value(2).toString()));
                    m_statemode->setItem(rownum, 3, new QStandardItem(query.value(3).toString()));
                    m_statemode->item(rownum,0)->setTextAlignment(Qt::AlignCenter);
                    m_statemode->item(rownum,1)->setTextAlignment(Qt::AlignCenter);
                    m_statemode->item(rownum,2)->setTextAlignment(Qt::AlignCenter);
                    m_statemode->item(rownum,3)->setTextAlignment(Qt::AlignCenter);
                    rownum++;
                }
            }
        }
        else
        {
            m_stateview->reset();
        }
    }
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
    QFile file(filePath) ;
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        return;
    }

    QTextStream out(&file);
    out<<QStringLiteral("检测时间:,")<<S.toStdString().c_str()<<QStringLiteral("至," )<< E.toStdString().c_str()<<",\n" ;
    QString Stmp;
    QModelIndex index = m_viewhis->currentIndex();
    for (int i=0;i<m_modehis->columnCount(index);i++)
    {
        Stmp= m_modehis->headerData(i,Qt::Horizontal,Qt::DisplayRole).toString()+",";
        out<<Stmp;
    }
    out<<",\n";
    int iNum = m_modehis->getRecordList().count();
    QVariant data;
    QString strVal;
    //循环显示数据
    for (int i = 0; i < iNum; i++)
    {
        for(int k = 0;k<m_modehis->columnCount(index);k++)
        {
            index = m_modehis->index(i, k);
            data = m_modehis->data(index, Qt::DisplayRole);
            strVal = data.value<QString>()+",";
            out<<strVal;
        }
        out<<",\n";
    }
    file.close();
}

void OverWidget::initTitle()
{
    m_titlewid = new QWidget;
    m_titlewid->setFixedHeight(60);

    //设置图标底色和背景色一样
    QPalette myPalette;
    QColor myColor(0, 0, 0);
    myColor.setAlphaF(0);
    myPalette.setBrush(backgroundRole(), myColor);

    m_toplogo = new CustomPushButton;
    m_toplogo->setButtonBackground((":/pnd/png/logo.png"));
    m_closebut = new CustomPushButton;
    m_closebut->setButtonBackground((":/pnd/png/X.png"));
    m_labtitle = new CustomPushButton;
    m_labtitle->setButtonBackground((":/pnd/png/title.png"));
    m_toplogo->setPalette(myPalette);
    m_closebut->setPalette(myPalette);
    m_labtitle->setPalette(myPalette);
    QObject::connect(m_closebut, SIGNAL(clicked()), this, SLOT(close()));

    m_backbut = new CustomPushButton;
    m_backbut->setButtonBackground((":/pnd/png/back.png"));
    m_backbut->hide();

    m_toplayout = new QHBoxLayout;
    m_toplayout->addWidget(m_toplogo);
    m_toplayout->addWidget(m_labtitle);
    m_toplayout->addStretch();
    m_toplayout->addWidget(m_closebut,0,Qt::AlignTop);
    m_toplayout->setSpacing(20);
    m_toplayout->setMargin(0);
    m_titlewid->setLayout(m_toplayout);

    m_dhtoplay = new QHBoxLayout;
    m_cReal = new CustomPushButton;
    m_cReal->setButtonBackground(":/pnd/png/real-press.png");
    m_cHis = new CustomPushButton;
    m_cHis->setButtonBackground(":/pnd/png/his.png");
    m_cLog = new CustomPushButton;
    m_cLog->setButtonBackground(":/pnd/png/log.png");
    m_cTj = new CustomPushButton;
    m_cTj->setButtonBackground(":/pnd/png/tj.png");
    m_cSelf = new CustomPushButton;
    m_cSelf->setButtonBackground(":/pnd/png/self.png");
    m_cReal->setPalette(myPalette);
    m_cHis->setPalette(myPalette);
    m_cLog->setPalette(myPalette);
    m_cSelf->setPalette(myPalette);
    m_cTj->setPalette(myPalette);
    m_dhtoplay->addStretch();
    m_dhtoplay->addWidget(m_cReal);
    m_dhtoplay->addWidget(m_cHis);
    m_dhtoplay->addWidget(m_cSelf);
    m_dhtoplay->addWidget(m_cTj);
    m_dhtoplay->addWidget(m_cLog);
    m_dhtoplay->addStretch();
    m_dhtoplay->addWidget(m_backbut);
    m_dhtoplay->setSpacing(15);
    m_dhtoplay->setMargin(0);
    QObject::connect(m_cReal, SIGNAL(clicked()), this, SLOT(cRealslot()));
    QObject::connect(m_cHis, SIGNAL(clicked()), this, SLOT(cHisslot()));
    QObject::connect(m_cSelf, SIGNAL(clicked()), this, SLOT(cSelfslot()));
    QObject::connect(m_cTj, SIGNAL(clicked()), this, SLOT(cTjslot()));
    QObject::connect(m_cLog, SIGNAL(clicked()), this, SLOT(cLogslot()));
    QObject::connect(m_backbut, SIGNAL(clicked()), this, SLOT(backslot()));
}

void OverWidget::cRealslot()
{
    if(m_backbut->isVisible())
    {
        QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("请先返回"), QMessageBox::Ok);
        return;
    }
    m_cReal->setButtonBackground(":/pnd/png/real-press.png");
    m_cHis->setButtonBackground(":/pnd/png/his.png");
    m_cLog->setButtonBackground(":/pnd/png/log.png");
    m_cTj->setButtonBackground(":/pnd/png/tj.png");
    m_cSelf->setButtonBackground(":/pnd/png/self.png");
    m_hiswid->hide();
    m_Statisticswid->hide();
    m_statewid->hide();
    m_logwid->hide();
    m_realwid->show();
}

void OverWidget::cHisslot()
{
    if(m_backbut->isVisible())
    {
        QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("请先返回"), QMessageBox::Ok);
        return;
    }
    m_cReal->setButtonBackground(":/pnd/png/real.png");
    m_cHis->setButtonBackground(":/pnd/png/his-press.png");
    m_cLog->setButtonBackground(":/pnd/png/log.png");
    m_cTj->setButtonBackground(":/pnd/png/tj.png");
    m_cSelf->setButtonBackground(":/pnd/png/self.png");
    m_realwid->hide();
    m_Statisticswid->hide();
    m_statewid->hide();
    m_logwid->hide();
    m_hiswid->show();
}

void OverWidget::cSelfslot()
{
    if(m_backbut->isVisible())
    {
        QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("请先返回"), QMessageBox::Ok);
        return;
    }
    m_cReal->setButtonBackground(":/pnd/png/real.png");
    m_cHis->setButtonBackground(":/pnd/png/his.png");
    m_cLog->setButtonBackground(":/pnd/png/log.png");
    m_cTj->setButtonBackground(":/pnd/png/tj.png");
    m_cSelf->setButtonBackground(":/pnd/png/self-press.png");
    m_realwid->hide();
    m_Statisticswid->hide();
    m_hiswid->hide();
    m_logwid->hide();
    m_statewid->show();
}

void OverWidget::cTjslot()
{
    if(m_backbut->isVisible())
    {
        QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("请先返回"), QMessageBox::Ok);
        return;
    }
    m_cReal->setButtonBackground(":/pnd/png/real.png");
    m_cHis->setButtonBackground(":/pnd/png/his.png");
    m_cLog->setButtonBackground(":/pnd/png/log.png");
    m_cTj->setButtonBackground(":/pnd/png/tj-press.png");
    m_cSelf->setButtonBackground(":/pnd/png/self.png");
    m_realwid->hide();
    m_hiswid->hide();
    m_statewid->hide();
    m_logwid->hide();
    m_Statisticswid->show();
}

void OverWidget::cLogslot()
{
    if(m_backbut->isVisible())
    {
        QMessageBox::warning(nullptr, QStringLiteral("警告"), QStringLiteral("请先返回"), QMessageBox::Ok);
        return;
    }
    m_cReal->setButtonBackground(":/pnd/png/real.png");
    m_cHis->setButtonBackground(":/pnd/png/his.png");
    m_cLog->setButtonBackground(":/pnd/png/log-press.png");
    m_cTj->setButtonBackground(":/pnd/png/tj.png");
    m_cSelf->setButtonBackground(":/pnd/png/self.png");
    m_realwid->hide();
    m_Statisticswid->hide();
    m_statewid->hide();
    m_hiswid->hide();
    m_logwid->show();
}

void OverWidget::backslot()
{
    if(m_preTimer->isActive())
        onStop();

    m_backbut->hide();
    m_recordshowwid->hide();
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


void* UploadVideoThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    pThis->OutputLog2("video线程开启...");
    char strPath[100] = {};
    GetPrivateProfileString("SYS", "upload", "1", strPath, 100, pThis->strConfigfile.c_str());
    if (atoi(strPath) == 0)
        return 0;

    if(pThis->m_govType == 1)  //广东省厅
    {
        GetPrivateProfileString("SYS", "webserviceGov", "http://10.17.64.82:8079", pThis->m_strURLGov, sizeof(pThis->m_strURLGov), pThis->strConfigfile.c_str());
        pThis->OutputLog2("webserviceGov=%s", pThis->m_strURLGov);
        while(pThis->bStart)
        {
            Sleep(1000);
            QString strQuery = "SELECT * FROM Task WHERE bUploadvedio = 2 order by id desc limit 50;";
            pThis->m_videoQuery.prepare(strQuery);
            if (!pThis->m_videoQuery.exec())
            {
                QString querys = pThis->m_videoQuery.lastError().text();
                pThis->OutputLog2("车辆待传省厅记录查询失败 %s",querys.toStdString().c_str());
                return 0;
            }
            else
            {
                int iCount = 0;
                if (pThis->m_videoQuery.last())
                {
                    iCount = pThis->m_videoQuery.at() + 1;
                    pThis->m_videoQuery.first();//重新定位指针到结果集首位
                    pThis->m_videoQuery.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
                    if(iCount>0)
                    {
                        pThis->OutputLog2("车辆待传省厅记录%d条",iCount);
                        while (pThis->m_videoQuery.next())
                        {
                            try
                            {
                                int nFID = pThis->m_videoQuery.value("id").toInt();
                                QNetworkAccessManager pManager;
                                QNetworkRequest requt;
                                requt.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded")); //设置数据类型

                                char url[1024] = {0};
                                sprintf(url, POST_VehicleInfo_URL, pThis->m_strURLGov);
                                requt.setUrl(QUrl(url));
                                //json
                                QJsonObject pageObject;
                                pageObject.insert("dataId", QStringLiteral("123"));
                                pageObject.insert("dataType", 1);
                                pageObject.insert("indexCode", pThis->m_indexCode);
                                pageObject.insert("checkId", pThis->m_checkId);
                                pageObject.insert("plateNo", pThis->m_videoQuery.value("CarNO").toString());
                                QString stmp = pThis->m_videoQuery.value("PlateColor").toString();
                                if(stmp.indexOf(QStringLiteral("蓝")) != -1)
                                    pageObject.insert("plateColor", 1);
                                else if(stmp.indexOf(QStringLiteral("黄")) != -1)
                                    pageObject.insert("plateColor", 2);
                                else if(stmp.indexOf(QStringLiteral("白")) != -1)
                                    pageObject.insert("plateColor", 4);
                                else if(stmp.indexOf(QStringLiteral("黑")) != -1)
                                    pageObject.insert("plateColor", 3);
                                else if(stmp.indexOf(QStringLiteral("绿")) != -1)
                                    pageObject.insert("plateColor", 5);
                                else
                                    pageObject.insert("plateColor", 9);

                                pageObject.insert("plateType", 1);
                                pageObject.insert("vehicleColor", "J");
                                pageObject.insert("vehicleType", "K30");
                                pageObject.insert("vehicleSpeed", pThis->m_videoQuery.value("Speed").toInt());
                                pageObject.insert("laneNo", pThis->m_videoQuery.value("laneno").toInt());
                                stmp = pThis->m_videoQuery.value("DetectionResult").toString();
                                if(stmp.indexOf(QStringLiteral("不")) != -1)
                                    pageObject.insert("isOverWeight", 1);
                                else
                                    pageObject.insert("isOverWeight", 0);
                                pageObject.insert("axleNum", pThis->m_videoQuery.value("Axles").toInt());
                                stmp = pThis->m_videoQuery.value("direction").toString();
                                if(stmp.indexOf(QStringLiteral("逆")) != -1)
                                    pageObject.insert("isDirect", 1);
                                else
                                    pageObject.insert("isDirect", 0);
                                pageObject.insert("overWeight", pThis->m_videoQuery.value("overWeight").toInt());
                                pageObject.insert("vehicleWeight", pThis->m_videoQuery.value("Weight").toInt());
                                pageObject.insert("limitWeight", pThis->m_videoQuery.value("XZ").toInt());
                                pageObject.insert("checkTime", pThis->m_videoQuery.value("DetectionTime").toString());
                                pageObject.insert("passCode", 0);
                                int alexnum = pThis->m_videoQuery.value("Axles").toInt();
                                {
                                    QJsonArray versionArray;
                                    for(int i = 1; i <= alexnum; i++)
                                    {
                                        QJsonObject Object;
                                        Object.insert("AxleNo", i);
                                        stmp = QString("AxlesWeight%1").arg(i+1);
                                        Object.insert("AxleInfo", pThis->m_videoQuery.value(stmp).toInt());
                                        Object.insert("axleSpeed ", pThis->m_videoQuery.value("Speed").toInt());
                                        stmp = QString("AxlesSpace1%1").arg(i+1);
                                        Object.insert("axleDistance", pThis->m_videoQuery.value(stmp).toInt());

                                        versionArray.append(Object);
                                    }
                                    pageObject.insert("axleArray", QJsonValue(versionArray));
                                }
                                pageObject.insert("dataIsCredible", 0);
                                pageObject.insert("tyreNums", alexnum*2);
                                pageObject.insert("loadRate", pThis->m_videoQuery.value("overPercent").toString());
                                QJsonDocument document;
                                document.setObject(pageObject);
                                QByteArray byteArray = document.toJson(QJsonDocument::Compact);
                                QString strlog(byteArray);
                                pThis->OutputLog2("上传省厅数据= %s", (const char*)strlog.toLocal8Bit());
                                {
                                    QJsonArray versionArray;
                                    QString fileName = pThis->m_msgQuery.value("ImageBaseFront").toString();
                                    if(fileName.length() > 1)
                                    {
                                        QJsonObject Object;
                                        Object.insert("imageName", QStringLiteral("前照片"));
                                        Object.insert("imageUrl", "");
                                        Object.insert("imageFeature", "D");
                                        Object.insert("imageBase64", GetPicBase64Data(fileName));
                                        versionArray.append(Object);
                                    }
                                    fileName = pThis->m_msgQuery.value("ImageBaseBack").toString();
                                    if(fileName.length() > 1)
                                    {
                                        QJsonObject Object;
                                        Object.insert("imageName", QStringLiteral("后照片"));
                                        Object.insert("imageUrl", "");
                                        Object.insert("imageFeature", "WD");
                                        Object.insert("imageBase64", GetPicBase64Data(fileName));
                                        versionArray.append(Object);
                                    }
                                    fileName = pThis->m_msgQuery.value("ImageBaseSide").toString();
                                    if(fileName.length() > 1)
                                    {
                                        QJsonObject Object;
                                        Object.insert("imageName", QStringLiteral("侧照片"));
                                        Object.insert("imageUrl", "");
                                        Object.insert("imageFeature", "C");
                                        Object.insert("imageBase64", GetPicBase64Data(fileName));
                                        versionArray.append(Object);
                                    }
                                    fileName = pThis->m_msgQuery.value("ImageBaseCarNO").toString();
                                    if(fileName.length() > 1)
                                    {
                                        QJsonObject Object;
                                        Object.insert("imageName", QStringLiteral("小照片"));
                                        Object.insert("imageUrl", "");
                                        Object.insert("imageFeature", "X");
                                        Object.insert("imageBase64", GetPicBase64Data(fileName));
                                        versionArray.append(Object);
                                    }
                                    pageObject.insert("imageArray", QJsonValue(versionArray));
                                }
                                {
                                    QJsonArray versionArray;
                                    QString fileName = pThis->m_msgQuery.value("vedioBaseFront").toString();
                                    if(fileName.length() > 1)
                                    {
                                        QJsonObject Object;
                                        Object.insert("videoName", QStringLiteral("前录像"));
                                        Object.insert("videoURL", "");
                                        Object.insert("videoBase64", GetPicBase64Data(fileName));
                                        versionArray.append(Object);
                                    }
                                    pageObject.insert("videoArray", QJsonValue(versionArray));
                                }

                                QJsonDocument document1;
                                document1.setObject(pageObject);
                                QByteArray byteStrData = document1.toJson(QJsonDocument::Compact);
                                QString StrData64 = Encode(byteStrData.data(), byteStrData.length());
                                QString str = QString("authorizeId=%1&strData=%2").arg(pThis->m_authorizeId).arg(StrData64);

                                QNetworkReply *reply = pManager.post(requt, str.toUtf8());
                                QEventLoop eventLoop;
                                QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
                                eventLoop.exec();
                                if (reply->error() == QNetworkReply::NoError)
                                {
                                    QByteArray bytes = reply->readAll();
                                    QString strRet(bytes);
                                    pThis->OutputLog2(strRet.toLocal8Bit());

                                    QJsonParseError parseJsonErr;
                                    QJsonDocument document = QJsonDocument::fromJson(bytes,&parseJsonErr);
                                    if(parseJsonErr.error == QJsonParseError::NoError)
                                    {
                                        QJsonObject jsonObject = document.object();
                                        int bRet = jsonObject["rscode"].toInt();
                                        if(bRet == 200)
                                        {
                                            QSqlQuery qsQuery = QSqlQuery(g_DB);
                                            QString strSqlText = QString("UPDATE Task SET bUploadvedio=1 WHERE id = %1;").arg(nFID);
                                            qsQuery.prepare(strSqlText);
                                            if (!qsQuery.exec())
                                            {
                                                QString querys = qsQuery.lastError().text();
                                                pThis->OutputLog2("%d bUpload=1 webId error %s",nFID,querys.toStdString().c_str());
                                            }
                                        }
                                        else
                                        {
                                            //接口报错
                                            QSqlQuery qsQuery = QSqlQuery(g_DB);
                                            QString strSqlText = QString("UPDATE Task SET bUploadvedio=6 WHERE id = %1;").arg(nFID);
                                            qsQuery.prepare(strSqlText);
                                            if (!qsQuery.exec())
                                            {
                                                QString querys = qsQuery.lastError().text();
                                                pThis->OutputLog2("%d bUpload=6 webId error %s",nFID,querys.toStdString().c_str());
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                                    //statusCodeV是HTTP服务器的相应码，reply->error()是Qt定义的错误码，可以参考QT的文档
                                    pThis->OutputLog2("found error ....code: %d %d", statusCodeV.toInt(), (int)reply->error());
                                    pThis->OutputLog2(reply->errorString().toStdString().c_str());
                                    QSqlQuery qsQuery = QSqlQuery(g_DB);
                                    QString strSqlText = QString("UPDATE Task SET bUploadvedio=6 WHERE id = %1;").arg(nFID);
                                    qsQuery.prepare(strSqlText);
                                    if (!qsQuery.exec())
                                    {
                                        QString querys = qsQuery.lastError().text();
                                        pThis->OutputLog2("%d bUpload=6 webId error %s",nFID,querys.toStdString().c_str());
                                    }
                                }
                                reply->deleteLater();
                            }
                            catch (...)
                            {
                                pThis->OutputLog2("省厅线程数据处理异常");
                            }
                        }
                    }
                }
            }
        }
    }

    GetPrivateProfileString("SYS", "webservicenew", "http://27.223.76.86:8888", pThis->m_strURLnew, sizeof(pThis->m_strURLnew), pThis->strConfigfile.c_str());
    pThis->OutputLog2("webservicenew=%s", pThis->m_strURLnew);
    if(pThis->m_netType == 1)
    {
        while(pThis->bStart)
        {
            Sleep(1000);
            QString strQuery = "SELECT * FROM Task WHERE bUploadvedio = 2 and webno > 0 order by id desc;";
            pThis->m_videoQuery.prepare(strQuery);
            if (!pThis->m_videoQuery.exec())
            {
                QString querys = pThis->m_videoQuery.lastError().text();
                pThis->OutputLog2("车辆待传video记录查询失败 %s",querys.toStdString().c_str());
                return 0;
            }
            else
            {
                int iCount = 0;
                if (pThis->m_videoQuery.last())
                {
                    iCount = pThis->m_videoQuery.at() + 1;
                    pThis->m_videoQuery.first();//重新定位指针到结果集首位
                    pThis->m_videoQuery.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
                    if(iCount>0)
                    {
                        pThis->OutputLog2("车辆待传video记录%d条",iCount);
                        while (pThis->m_videoQuery.next())
                        {
                            try
                            {
                                int nID = pThis->m_videoQuery.value("id").toInt();
                                qint64 nFID = pThis->m_videoQuery.value("webno").toLongLong();
                                OutputLog("记录%d  %lld",nID, nFID);
                                QNetworkRequest request;    //(注意这个地方不能设置head)
                                char url[1024] = {0};
                                sprintf(url, POST_DataOverVideo_URL, pThis->m_strURLnew);
                                request.setUrl(QUrl(url));

                                QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

                                QHttpPart image_part;
                                //image_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                image_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"detecVedio\";filename=\"%1\";").arg(pThis->m_videoQuery.value("vedioBaseFront").toString())));
                                QString fileName = pThis->m_videoQuery.value("vedioBaseFront").toString();
                                QFile *file = new QFile(fileName);      //由于是异步，所以记得一定要new
                                file->open(QIODevice::ReadOnly);
                                image_part.setBodyDevice(file);          //方便后续内存释放
                                file->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                multiPart->append(image_part);

                                QHttpPart textPart;
                                textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"id\""));
                                textPart.setBody(QString::number(nFID).toUtf8());
                                multiPart->append(textPart);

                                QNetworkAccessManager pManger;
                                QNetworkReply *reply = pManger.post(request, multiPart);
                                multiPart->setParent(reply);
                                QEventLoop eventLoop;
                                QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
                                eventLoop.exec();
                                if (reply->error() == QNetworkReply::NoError)
                                {
                                    QByteArray bytes = reply->readAll();
                                    QString strRet(bytes);
                                    pThis->OutputLog2(strRet.toLocal8Bit());
                                    QJsonParseError parseJsonErr;
                                    QJsonDocument document = QJsonDocument::fromJson(bytes,&parseJsonErr);
                                    if(parseJsonErr.error == QJsonParseError::NoError)
                                    {
                                        QJsonObject jsonObject = document.object();
                                        int bRet = jsonObject["code"].toInt();
                                        if(bRet == 200)
                                        {
                                            QSqlQuery qsQuery = QSqlQuery(g_DB);
                                            QString strSqlText = QString("UPDATE Task SET bUploadvedio=1 WHERE id = %1;").arg(nID);
                                            qsQuery.prepare(strSqlText);
                                            if (!qsQuery.exec())
                                            {
                                                QString querys = qsQuery.lastError().text();
                                                pThis->OutputLog2("%d bUploadvedio=1 error %s",nID,querys.toStdString().c_str());
                                            }
                                        }
                                        else
                                        {
                                            //接口报错
                                            Sleep(2000);
                                            pThis->OutputLog2("code != 200");
                                        }
                                    }
                                }
                                else
                                {
                                    QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                                    //statusCodeV是HTTP服务器的相应码，reply->error()是Qt定义的错误码，可以参考QT的文档
                                    pThis->OutputLog2("found error ....code: %d %d", statusCodeV.toInt(), (int)reply->error());
                                    pThis->OutputLog2(reply->errorString().toStdString().c_str());
                                    Sleep(2000);
                                }
                                reply->deleteLater();
                            }
                            catch (...)
                            {
                                pThis->OutputLog2("video线程数据处理异常");
                            }
                        }
                    }
                }
    //            query.clear();
    //            query.finish();
            }
        }
    }

    return 0;
}

void* UploadDevStateThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    char strPath[100] = {};
    GetPrivateProfileString("SYS", "upload", "1", strPath, 100, pThis->strConfigfile.c_str());
    if (atoi(strPath) == 0)
        return 0;

    GetPrivateProfileString("SYS", "webserviceGov", "http://10.17.64.82:8079", pThis->m_strURLGov, sizeof(pThis->m_strURLGov), pThis->strConfigfile.c_str());
    pThis->OutputLog2("webserviceGov=%s", pThis->m_strURLGov);
    while(pThis->bStart)
    {
        try
        {
            if(pThis->m_govType == 1)  //广东省厅
            {
                QNetworkAccessManager pManager;
                QNetworkRequest requt;
                requt.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded")); //设置数据类型

                char url[1024] = {0};
                sprintf(url, POST_monitorHeartbeat_URL, pThis->m_strURLGov);
                requt.setUrl(QUrl(url));

                //json
                QJsonObject pageObject;
                pageObject.insert("dataId", QStringLiteral("123"));
                pageObject.insert("dataType", 1);
                QJsonDocument document1;
                document1.setObject(pageObject);
                QByteArray byteStrData = document1.toJson(QJsonDocument::Compact);
                QString StrData64 = Encode(byteStrData.data(), byteStrData.length());
                QString str = QString("authorizeId=%1&strData=%2").arg(pThis->m_authorizeId).arg(StrData64);

                QNetworkReply *reply = pManager.post(requt, str.toUtf8());
                QEventLoop eventLoop;
                QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
                eventLoop.exec();
                if (reply->error() == QNetworkReply::NoError)
                {
                    QByteArray bytes = reply->readAll();
                    QString strRet(bytes);
                    pThis->OutputLog2(strRet.toLocal8Bit());

                    QJsonParseError parseJsonErr;
                    QJsonDocument document = QJsonDocument::fromJson(bytes,&parseJsonErr);
                    if(parseJsonErr.error == QJsonParseError::NoError)
                    {
                        QJsonObject jsonObject = document.object();
                        int bRet = jsonObject["rscode"].toInt();
                        if(bRet == 200)
                        {
                            pThis->OutputLog2("心跳ok");
                        }
                        else
                        {
                           pThis->OutputLog2("心跳error");
                        }
                    }
                }
                else
                {
                    QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                    //statusCodeV是HTTP服务器的相应码，reply->error()是Qt定义的错误码，可以参考QT的文档
                    pThis->OutputLog2("found error ....code: %d %d", statusCodeV.toInt(), (int)reply->error());
                    pThis->OutputLog2(reply->errorString().toStdString().c_str());
                }
                reply->deleteLater();
            }
        }
        catch (...)
        {
            pThis->OutputLog2("UploadDevStateThreadProc数据处理异常");
        }
        Sleep(pThis->m_TimeHeart);

        try
        {
            DeviceNode* pTargetNode = NULL;
            // 构建 Json 对象
            QJsonObject json;
            QJsonArray versionArray;
            list<DeviceNode *>::iterator itr;//容器，用于链表遍历
            for(itr = pThis->lstDevice.begin();itr !=pThis->lstDevice.end();itr++)
            {
                pTargetNode = *itr;
                QJsonObject pageObject;
                pageObject.insert("deviceNo", pTargetNode->sDvrNo);
                pageObject.insert("deviceStatus", QString::number(pTargetNode->bIsOnline));
                if(pTargetNode->bIsOnline == NORMAL)
                    pageObject.insert("offlineReason", QStringLiteral(""));
                else
                    pageObject.insert("offlineReason", QStringLiteral("设备异常,无法连接"));

                versionArray.append(pageObject);
            }
            json.insert("devices", QJsonValue(versionArray));
            //构建 Json 文档
            QJsonDocument document;
            document.setObject(json);
            QByteArray byteArray = document.toJson(QJsonDocument::Compact);
            QString strlog(byteArray);
            pThis->OutputLog2("上传状态数据= %s", (const char*)strlog.toLocal8Bit());

            char url[1024] = {0};
            sprintf(url, POST_DataOverState_URL, pThis->m_strURLnew);
            //http
            QNetworkRequest request;
            request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json")); //设置数据类型
            request.setUrl(QUrl(url));
            QNetworkAccessManager manager;
            QNetworkReply *reply = manager.post(request,byteArray);
            QEventLoop eventLoop;
            QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            if (reply->error() == QNetworkReply::NoError)
            {
                QByteArray bytes = reply->readAll();
                QString strRet(bytes);
                pThis->OutputLog2(strRet.toLocal8Bit());
            }
            else
            {
                QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                //statusCodeV是HTTP服务器的相应码，reply->error()是Qt定义的错误码，可以参考QT的文档
                pThis->OutputLog2("found error ....code: %d %d", statusCodeV.toInt(), (int)reply->error());
                pThis->OutputLog2(reply->errorString().toStdString().c_str());
            }
            reply->deleteLater();
        }
        catch (...)
        {
            pThis->OutputLog2("UploadDevStateThreadProc数据处理异常");
        }
    }

    return 0;
}

void* UploadMsgThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    pThis->OutputLog2("上传线程开启...");
    char strPath[100] = {};
    GetPrivateProfileString("SYS", "upload", "1", strPath, 100, pThis->strConfigfile.c_str());
    if (atoi(strPath) == 0)
        return 0;

    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, pThis->strConfigfile.c_str());
    QString dir_strCar = QString(strPath)+PLATPATH+"carinfo";

    int nLength=5*1024*1024;
    char *pack=(char *)malloc(nLength);
    if (pack==NULL)
    {
        pThis->OutputLog2("Alloc 5M Memory Error...");
    }

    GetPrivateProfileString("SYS", "webservicenew", "http://27.223.76.86:8888", pThis->m_strURLnew, sizeof(pThis->m_strURLnew), pThis->strConfigfile.c_str());
    pThis->OutputLog2("webservicenew=%s", pThis->m_strURLnew);
    while(pThis->bStart)
    {
        Sleep(1000);
        QString strQuery = "SELECT * FROM Task WHERE bUploadpic = 0 order by id desc limit 30;";
        pThis->m_msgQuery.prepare(strQuery);
        if (!pThis->m_msgQuery.exec())
        {
            QString querys = pThis->m_msgQuery.lastError().text();
            pThis->OutputLog2("车辆待传记录查询失败 %s",querys.toStdString().c_str());
            return 0;
        }
        else
        {
            int iCount = 0;
            if (pThis->m_msgQuery.last())
            {
                iCount = pThis->m_msgQuery.at() + 1;
                pThis->m_msgQuery.first();//重新定位指针到结果集首位
                pThis->m_msgQuery.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
                if(iCount > 0)
                {
                    pThis->OutputLog2("车辆待传msg记录%d条",iCount);
                    while (pThis->m_msgQuery.next())
                    {
                        int nFID = pThis->m_msgQuery.value("id").toInt();
                        //图片水印
                        STR_MSG msg = {0};
                        strcpy(msg.strtime,pThis->m_msgQuery.value("DetectionTime").toString().toLocal8Bit());
                        strcpy(msg.strcarno,pThis->m_msgQuery.value("CarNO").toString().toLocal8Bit());
                        strcpy(msg.strdirection,pThis->m_msgQuery.value("direction").toString().toLocal8Bit());
                        strcpy(msg.strcrossroad,pThis->m_msgQuery.value("crossroad").toString().toLocal8Bit());
                        strcpy(msg.strret,pThis->m_msgQuery.value("DetectionResult").toString().toLocal8Bit());
                        strcpy(msg.strprepic,pThis->m_msgQuery.value("ImageBaseFront").toString().toLocal8Bit());
                        strcpy(msg.strafrpic,pThis->m_msgQuery.value("ImageBaseBack").toString().toLocal8Bit());
                        strcpy(msg.strallpic,pThis->m_msgQuery.value("ImageBasePanorama").toString().toLocal8Bit());
                        strcpy(msg.strcepic,pThis->m_msgQuery.value("ImageBaseSide").toString().toLocal8Bit());
                        strcpy(msg.strsmallpic,pThis->m_msgQuery.value("ImageBaseCarNO").toString().toLocal8Bit());
                        msg.speed = pThis->m_msgQuery.value("Speed").toString().toInt();
                        msg.axiscount = pThis->m_msgQuery.value("Axles").toString().toInt();
                        msg.weight = pThis->m_msgQuery.value("Weight").toString().toInt();
                        msg.GBxz = pThis->m_msgQuery.value("XZ").toString().toInt();
                        pThis->DrawPicSY(msg);
                        pThis->OutputLog2("DrawPicSY...");
                        if(pThis->m_netType == 0)
                        {
                            char filecar[256] = {};
                            sprintf(filecar, "%s%s%s.ini", dir_strCar.toStdString().c_str(),PLATPATH,msg.strcarno);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byPlateType = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byColor = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byBright = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byLicenseLen = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byEntireBelieve = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byVehicleType = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byColorDepth = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byColorCar = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.wLength = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byDir = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byVehicleLogoRecog = atoi(strPath);
                            GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                            msg.byVehicleType2 = atoi(strPath);
                            try
                            {
                                int n=128;  //报文头固定128
                                memset(pack,0,nLength);
                                //结果信息
                                memcpy(&pack[n],&nFID,sizeof(int));
                                n=n+sizeof(int);
                                pack[n] = 2;         //检测方式
                                n=n+1;
                                pack[n] = 0;         //卡口
                                n=n+1;
                                int dwChannel = 1;
                                {
                                    char  szVal[200], strSection[20];
                                    sprintf(strSection, "车道%d", pThis->m_msgQuery.value("laneno").toString().toInt());
                                    GetPrivateProfileString(strSection, "前相机", "", szVal, 19, pThis->strConfigfile.c_str());
                                    int iPreID = atoi(szVal);
                                    auto ifind = pThis->m_maplaneAndDevice.find(iPreID);
                                    if (ifind != pThis->m_maplaneAndDevice.end())
                                    {
                                        dwChannel = ifind->second->lChanlNumPre;
                                    }
                                }
                                pack[n] = (unsigned char)dwChannel;   //通道号
                                n=n+1;
                                int lane = pThis->m_msgQuery.value("laneno").toString().toInt();
                                pack[n] = (unsigned char)lane;  //车道
                                n=n+1;
                                QString stime = pThis->m_msgQuery.value("DetectionTime").toString();
                                stime.replace(" ","-");
                                stime.replace(":","-");
                                stime = stime + "-123";
                                memcpy(&pack[n],stime.toStdString().c_str(),32);
                                n=n+32;
                                n=n+48;
                                pack[n] = 0;         //是否携带消息
                                n=n+4;
                                pack[n] = 0;         //附件消息长
                                n=n+4;
                                pack[n] = 3;         //图片数量
                                n=n+4;
                                memcpy(&pack[n],&msg.weight,sizeof(int));
                                n=n+4;
                                memcpy(&pack[n],&msg.GBxz,sizeof(int));
                                n=n+4;
                                pack[n] = 0;         //轴型
                                n=n+4;
                                pack[n] = (unsigned char)msg.axiscount;  //轴数
                                n=n+1;
                                n=n+3;
                                //车牌信息
                                pack[n] = (unsigned char)msg.byPlateType;         //车牌类型
                                n=n+1;
                                pack[n] = (unsigned char)msg.byColor;         //车牌颜色
                                n=n+1;
                                pack[n] = (unsigned char)msg.byBright;         //车牌亮度
                                n=n+1;
                                pack[n] = (unsigned char)msg.byLicenseLen;         //车牌字符个数
                                n=n+1;
                                memcpy(&pack[n],msg.strcarno,16);
                                n=n+16;
                                pack[n] = (unsigned short)msg.byEntireBelieve; //置信
                                n=n+2;
                                n=n+10;
                                //车辆信息定义
                                pack[n] = (unsigned char)msg.byVehicleType;         //车辆类型
                                n=n+1;
                                pack[n] = (unsigned char)msg.byColorDepth;         //车身颜色深浅
                                n=n+1;
                                pack[n] = (unsigned char)msg.byColorCar;         //车身颜色
                                n=n+1;
                                pack[n] = (unsigned char)msg.byDir;         //方向
                                n=n+1;
                                pack[n] = (unsigned short)msg.speed;
                                n=n+2;
                                pack[n] = (unsigned short)msg.wLength;  //车身长度
                                n=n+2;
                                pack[n] = 0;         //
                                n=n+4;
                                pack[n] = 0;         //
                                n=n+4;
                                pack[n] = (unsigned short)msg.byVehicleLogoRecog;  //车标
                                n=n+2;
                                pack[n] = msg.byVehicleType2;         //车辆分类
                                n=n+1;
                                pack[n] = 0;         //附加
                                n=n+1;
                                n=n+16;
                                //图片组
                                QString sfile = pThis->m_msgQuery.value("ImageBaseFront").toString();
                                QFile file1(sfile);
                                int ipic1=file1.size();
                                sfile = pThis->m_msgQuery.value("ImageBaseBack").toString();
                                QFile file2(sfile);
                                int ipic2=file2.size();
                                sfile = pThis->m_msgQuery.value("ImageBaseCarNO").toString();
                                QFile file3(sfile);
                                int ipic3=file3.size();
                                int iTotal = ipic1+ipic2+ipic3;
                                memcpy(&pack[n],&iTotal,sizeof(int)); //图片总长度
                                n=n+sizeof(int);
                                //1
                                memcpy(&pack[n],&ipic1,sizeof(int)); //图片长度
                                n=n+sizeof(int);
                                memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                n=n+32;
                                pack[n] = (unsigned short)2;   //图片类型
                                n=n+2;
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                n=n+4;  //帧号
                                int w=0,h=0;
                                QPixmap pix = QPixmap(QString::fromLocal8Bit(msg.strprepic));
                                if(!pix.isNull())
                                {
                                    QImage image = pix.toImage();
                                    w = image.width();
                                    h = image.height();
                                }
                                pack[n] = (unsigned short)w;   //w
                                n=n+2;
                                pack[n] = (unsigned short)h;   //h
                                n=n+2;
                                n=n+8;
                                //2
                                memcpy(&pack[n],&ipic2,sizeof(int)); //图片长度
                                n=n+sizeof(int);
                                memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                n=n+32;
                                pack[n] = (unsigned short)2;   //图片类型
                                n=n+2;
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                n=n+4;  //帧号
                                w=0;
                                h=0;
                                pix = QPixmap(QString::fromLocal8Bit(msg.strafrpic));
                                if(!pix.isNull())
                                {
                                    QImage image = pix.toImage();
                                    w = image.width();
                                    h = image.height();
                                }
                                pack[n] = (unsigned short)w;   //w
                                n=n+2;
                                pack[n] = (unsigned short)h;   //h
                                n=n+2;
                                n=n+8;
                                //3
                                memcpy(&pack[n],&ipic3,sizeof(int)); //图片长度
                                n=n+sizeof(int);
                                memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                n=n+32;
                                pack[n] = (unsigned short)0;   //图片类型
                                n=n+2;
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                n=n+4;  //帧号
                                w=0;
                                h=0;
                                pix = QPixmap(QString::fromLocal8Bit(msg.strsmallpic));
                                if(!pix.isNull())
                                {
                                    QImage image = pix.toImage();
                                    w = image.width();
                                    h = image.height();
                                }
                                pack[n] = (unsigned short)w;   //w
                                n=n+2;
                                pack[n] = (unsigned short)h;   //h
                                n=n+2;
                                n=n+8;
                                //4
                                int ipic4 = 0;
                                memcpy(&pack[n],&ipic4,sizeof(int));    //图片长度
                                n=n+sizeof(int);
                                memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                n=n+32;
                                pack[n] = (unsigned short)0;   //图片类型
                                n=n+2;
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                n=n+4;  //帧号
                                w=0;
                                h=0;
                                pack[n] = (unsigned short)w;   //w
                                n=n+2;
                                pack[n] = (unsigned short)h;   //h
                                n=n+2;
                                n=n+8;
                                //5
                                int ipic5 = 0;
                                memcpy(&pack[n],&ipic5,sizeof(int));    //图片长度
                                n=n+sizeof(int);
                                memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                n=n+32;
                                pack[n] = (unsigned short)0;   //图片类型
                                n=n+2;
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                n=n+4;  //帧号
                                w=0;
                                h=0;
                                pack[n] = (unsigned short)w;   //w
                                n=n+2;
                                pack[n] = (unsigned short)h;   //h
                                n=n+2;
                                n=n+8;
                                //6
                                int ipic6 = 0;
                                memcpy(&pack[n],&ipic6,sizeof(int));    //图片长度
                                n=n+sizeof(int);
                                memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                n=n+32;
                                pack[n] = (unsigned short)0;   //图片类型
                                n=n+2;
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y
                                n=n+2;
                                pack[n] = (unsigned short)0;   //x len
                                n=n+2;
                                pack[n] = (unsigned short)0;   //y len
                                n=n+2;
                                n=n+4;  //帧号
                                w=0;
                                h=0;
                                pack[n] = (unsigned short)w;   //w
                                n=n+2;
                                pack[n] = (unsigned short)h;   //h
                                n=n+2;
                                n=n+8;
                                //memcpy1~6
                                {
                                    FILE* config = NULL;
                                    char *buffer = NULL;
                                    buffer = (char *)malloc(ipic1);
                                    if (buffer!=NULL)
                                    {
                                        config = fopen(msg.strprepic,"rb"); //读二进制文件和可写，如没有权限写或者读为NULL
                                        if(config != NULL)
                                        {
                                            fread(buffer,1,ipic1,config);
                                            fclose(config);
                                        }
                                    }
                                    memcpy(&pack[n],buffer,ipic1);
                                    n=n+ipic1;
                                    free(buffer);
                                }
                                {
                                    FILE* config = NULL;
                                    char *buffer = NULL;
                                    buffer = (char *)malloc(ipic2);
                                    if (buffer!=NULL)
                                    {
                                        config = fopen(msg.strafrpic,"rb"); //读二进制文件和可写，如没有权限写或者读为NULL
                                        if(config != NULL)
                                        {
                                            fread(buffer,1,ipic2,config);
                                            fclose(config);
                                        }
                                    }
                                    memcpy(&pack[n],buffer,ipic2);
                                    n=n+ipic2;
                                    free(buffer);
                                }
                                {
                                    FILE* config = NULL;
                                    char *buffer = NULL;
                                    buffer = (char *)malloc(ipic3);
                                    if (buffer!=NULL)
                                    {
                                        config = fopen(msg.strsmallpic,"rb"); //读二进制文件和可写，如没有权限写或者读为NULL
                                        if(config != NULL)
                                        {
                                            fread(buffer,1,ipic3,config);
                                            fclose(config);
                                        }
                                    }
                                    memcpy(&pack[n],buffer,ipic3);
                                    n=n+ipic3;
                                    free(buffer);
                                }
                                //总
                                int m = 0;
                                int iAll = n;
                                memcpy(&pack[m],&iAll,sizeof(int)); //图片长度
                                m=m+sizeof(int);
                                char version[8] = "V2.0";
                                memcpy(&pack[m],version,8); //图片长度
                                m=m+8;
                                pack[m] = (unsigned int)170;  //识别结果上传 170
                                m=m+4;
                                memcpy(&pack[m],pThis->m_webuser,32); //图片长度
                                m=m+32;
                                memcpy(&pack[m],pThis->m_webxlnum,48); //图片长度
                                m=m+48;
                                memcpy(&pack[m],pThis->m_webip,24); //图片长度
                                m=m+24;
                                pack[m] = (unsigned int)pThis->m_webport;
                                m=m+2;
                                pack[m] = 1;
                                m=m+1;
                                m=m+5;
                                //////////////////////////////////////////////////////////////////////////
                                SOCKET m_socketClient = socket(AF_INET, SOCK_STREAM, 0);
                                if (m_socketClient != INVALID_SOCKET)
                                {
                                    struct sockaddr_in ServerAddr = {};
                                    ServerAddr.sin_addr.S_un.S_addr = inet_addr(pThis->m_szServerIp);
                                    ServerAddr.sin_family = AF_INET;
                                    ServerAddr.sin_port = htons(pThis->m_nPort);
                                    if (connect(m_socketClient, (PSOCKADDR)&ServerAddr, sizeof(ServerAddr)) != SOCKET_ERROR)
                                    {
                                        int timeout = 5000; //5s
                                        setsockopt(m_socketClient, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
                                        send(m_socketClient, pack, iAll, 0);
                                        Sleep(100);
                                        char szRet[1024];
                                        memset(&szRet, 0x00, sizeof(szRet));
                                        int nLen = recv(m_socketClient, szRet, 1023, 0);
                                        shutdown(m_socketClient, SD_BOTH);
                                        closesocket(m_socketClient);
                                        if(nLen == 128)
                                        {
                                            QSqlQuery qsQuery = QSqlQuery(g_DB);
                                            QString strSqlText = QString("UPDATE Task SET bUploadpic=1 WHERE id = %1;").arg(nFID);
                                            qsQuery.prepare(strSqlText);
                                            if (!qsQuery.exec())
                                            {
                                                QString querys = qsQuery.lastError().text();
                                                pThis->OutputLog2("%d bUpload=1 webId error %s",nFID,querys.toStdString().c_str());
                                            }
                                        }
                                        else
                                        {
                                            QSqlQuery qsQuery = QSqlQuery(g_DB);
                                            QString strSqlText = QString("UPDATE Task SET bUploadpic=6 WHERE id = %1;").arg(nFID);
                                            qsQuery.prepare(strSqlText);
                                            if (!qsQuery.exec())
                                            {
                                                QString querys = qsQuery.lastError().text();
                                                pThis->OutputLog2("%d bUpload=6 webId error %s",nFID,querys.toStdString().c_str());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        OutputLog("Server连接失败 !! %d", WSAGetLastError());
                                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                                        QString strSqlText = QString("UPDATE Task SET bUploadpic=6 WHERE id = %1;").arg(nFID);
                                        qsQuery.prepare(strSqlText);
                                        if (!qsQuery.exec())
                                        {
                                            QString querys = qsQuery.lastError().text();
                                            pThis->OutputLog2("%d bUpload=6 webId error %s",nFID,querys.toStdString().c_str());
                                        }
                                    }

                                }
                                else
                                {
                                    OutputLog("创建socket失败 !! %d", WSAGetLastError());
                                    QSqlQuery qsQuery = QSqlQuery(g_DB);
                                    QString strSqlText = QString("UPDATE Task SET bUploadpic=6 WHERE id = %1;").arg(nFID);
                                    qsQuery.prepare(strSqlText);
                                    if (!qsQuery.exec())
                                    {
                                        QString querys = qsQuery.lastError().text();
                                        pThis->OutputLog2("%d bUpload=6 webId error %s",nFID,querys.toStdString().c_str());
                                    }
                                }
                                //////////////////////////////////////////////////////////////////////////
                            }
                            catch (...)
                            {
                                printf("Send Err");
                            }
                        }
                        if(pThis->m_netType == 1)
                        {
                            QNetworkRequest request;    //(注意这个地方不能设置head)
                            char url[1024] = {0};
                            sprintf(url, POST_DataOverMsg_URL, pThis->m_strURLnew);
                            request.setUrl(QUrl(url));

                            QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
                            QString fileName = pThis->m_msgQuery.value("ImageBaseFront").toString();
                            if(fileName.length() > 1)
                            {
                                QHttpPart image_part;
                                image_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                image_part.setHeader(QNetworkRequest::ContentDispositionHeader,QVariant(QString("form-data; name=\"frontPhoto\";filename=\"%1\";").arg(fileName)));
                                QFile *file = new QFile(fileName);      //由于是异步，所以记得一定要new
                                file->open(QIODevice::ReadOnly);
                                image_part.setBodyDevice(file);          //方便后续内存释放
                                file->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                multiPart->append(image_part);
                            }

                            QString fileName2 = pThis->m_msgQuery.value("ImageBaseBack").toString();
                            if(fileName2.length() > 1)
                            {
                                QHttpPart image_part2;
                                image_part2.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                image_part2.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"blackPhoto\";filename=\"%1\";").arg(fileName2)));
                                QFile *file2 = new QFile(fileName2);      //由于是异步，所以记得一定要new
                                file2->open(QIODevice::ReadOnly);
                                image_part2.setBodyDevice(file2);          //方便后续内存释放
                                file2->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                multiPart->append(image_part2);
                            }

                            QString fileName3 = pThis->m_msgQuery.value("ImageBaseSide").toString();  //
                            if(fileName3.length() > 1)
                            {
                                QHttpPart image_part3;
                                image_part3.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                image_part3.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"sidePhoto\";filename=\"%1\";").arg(fileName3)));
                                QFile *file3 = new QFile(fileName3);      //由于是异步，所以记得一定要new
                                file3->open(QIODevice::ReadOnly);
                                image_part3.setBodyDevice(file3);          //方便后续内存释放
                                file3->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                multiPart->append(image_part3);
                            }

                            QString fileName4 = pThis->m_msgQuery.value("ImageBaseCarNO").toString();
                            if(fileName4.length() > 1)
                            {
                                QHttpPart image_part4;
                                image_part4.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                image_part4.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"platePhoto\";filename=\"%1\";").arg(fileName4)));
                                QFile *file4 = new QFile(fileName4);      //由于是异步，所以记得一定要new
                                file4->open(QIODevice::ReadOnly);
                                image_part4.setBodyDevice(file4);          //方便后续内存释放
                                file4->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                multiPart->append(image_part4);
                            }

                            QString strJson = QString("{'stationId':%1, 'deviceNo':'%2', 'detectionTime':'%3', 'carPlate':'%4', 'carModel':'%5', 'wheelNumber':'%6', 'wheelBase':'%7,%8,%9,%10,%11,%12,%13', 'carLength':'%14', \
                                                          'overLength':'0', 'carWidth':'0', 'overWidth':'0', 'height':'0', 'overHeight':'0', 'wheelWeight':'%15,%16,%17,%18,%19,%20,%21,%22', 'totalWeight':'%23', 'limitWeight':'%24', 'overWeight':'%25', \
                                                          'overPercent':'%26', 'lineNo':'%27', 'speed':'%28', 'detectConclusion':'%29', 'detectType':'1'}")
                            .arg(pThis->m_stationId).arg(pThis->m_msgQuery.value("deviceNo").toString()).arg(pThis->m_msgQuery.value("DetectionTime").toString())
                                    .arg(pThis->m_msgQuery.value("CarNO").toString()).arg(pThis->m_msgQuery.value("CarType").toString()).arg(pThis->m_msgQuery.value("Axles").toString())
                                    .arg(pThis->m_msgQuery.value("AxlesSpace1").toString()).arg(pThis->m_msgQuery.value("AxlesSpace2").toString()).arg(pThis->m_msgQuery.value("AxlesSpace3").toString())
                                    .arg(pThis->m_msgQuery.value("AxlesSpace4").toString()).arg(pThis->m_msgQuery.value("AxlesSpace5").toString()).arg(pThis->m_msgQuery.value("AxlesSpace6").toString())
                                    .arg(pThis->m_msgQuery.value("AxlesSpace7").toString()).arg(pThis->m_msgQuery.value("carLength").toString()).arg(pThis->m_msgQuery.value("AxlesWeight1").toString())
                                    .arg(pThis->m_msgQuery.value("AxlesWeight2").toString()).arg(pThis->m_msgQuery.value("AxlesWeight3").toString()).arg(pThis->m_msgQuery.value("AxlesWeight4").toString())
                                    .arg(pThis->m_msgQuery.value("AxlesWeight5").toString()).arg(pThis->m_msgQuery.value("AxlesWeight6").toString()).arg(pThis->m_msgQuery.value("AxlesWeight7").toString())
                                    .arg(pThis->m_msgQuery.value("AxlesWeight8").toString()).arg(pThis->m_msgQuery.value("Weight").toString()).arg(pThis->m_GBXZ).arg(pThis->m_msgQuery.value("overWeight").toString())
                                    .arg(pThis->m_msgQuery.value("overPercent").toString()).arg(pThis->m_msgQuery.value("laneno").toString()).arg(pThis->m_msgQuery.value("Speed").toString()).arg(pThis->m_msgQuery.value("DetectionResult").toString());

                            pThis->OutputLog2(strJson.toLocal8Bit());
                            QHttpPart textPart;
                            textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"recordVo\""));
                            textPart.setBody(strJson.toUtf8());
                            multiPart->append(textPart);

                            QNetworkAccessManager pManger;
                            QNetworkReply *reply = pManger.post(request, multiPart);
                            multiPart->setParent(reply);
                            QEventLoop eventLoop;
                            QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
                            eventLoop.exec();
                            if (reply->error() == QNetworkReply::NoError)
                            {
                                QByteArray bytes = reply->readAll();
                                QString strRet(bytes);
                                pThis->OutputLog2(strRet.toLocal8Bit());
                                QJsonParseError parseJsonErr;
                                QJsonDocument document = QJsonDocument::fromJson(bytes,&parseJsonErr);
                                if(parseJsonErr.error == QJsonParseError::NoError)
                                {
                                    QJsonObject jsonObject = document.object();
                                    int bRet = jsonObject["code"].toInt();
                                    if(bRet == 200)
                                    {
                                        OutputLog("平台记录id= %s", jsonObject["data"].toString().toStdString().c_str());
                                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                                        QString strSqlText = QString("UPDATE Task SET bUploadpic=1,webno=:webno WHERE id = %1;").arg(nFID);
                                        qsQuery.prepare(strSqlText);
                                        qsQuery.bindValue(":webno", jsonObject["data"].toString());
                                        if (!qsQuery.exec())
                                        {
                                            QString querys = qsQuery.lastError().text();
                                            pThis->OutputLog2("%d bUpload=1 webId error %s",nFID,querys.toStdString().c_str());
                                        }
                                    }
                                    else
                                    {
                                        //接口报错
                                        pThis->OutputLog2("bRet!=200");
                                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                                        QString strSqlText = QString("UPDATE Task SET bUploadpic=6 WHERE id = %1;").arg(nFID);
                                        qsQuery.prepare(strSqlText);
                                        if (!qsQuery.exec())
                                        {
                                            QString querys = qsQuery.lastError().text();
                                            pThis->OutputLog2("%d bUpload=6 webId error %s",nFID,querys.toStdString().c_str());
                                        }
                                    }
                                }
                            }
                            else
                            {
                                QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                                //statusCodeV是HTTP服务器的相应码，reply->error()是Qt定义的错误码，可以参考QT的文档
                                pThis->OutputLog2("found error ....code: %d %d", statusCodeV.toInt(), (int)reply->error());
                                pThis->OutputLog2(reply->errorString().toStdString().c_str());
                                QSqlQuery qsQuery = QSqlQuery(g_DB);
                                QString strSqlText = QString("UPDATE Task SET bUploadpic=6 WHERE id = %1;").arg(nFID);
                                qsQuery.prepare(strSqlText);
                                if (!qsQuery.exec())
                                {
                                    QString querys = qsQuery.lastError().text();
                                    pThis->OutputLog2("%d bUpload=6 webId error %s",nFID,querys.toStdString().c_str());
                                }
                            }
                            reply->deleteLater();
                        }
                    }
                }
            }
        }
    }

    free(pack);

    return 0;
}   

void* ReUploadMsgThreadProc(LPVOID pParam)
{
        OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
        pThis->OutputLog2("重传线程开启...");
        char strPath[100] = {};
        GetPrivateProfileString("SYS", "upload", "1", strPath, 100, pThis->strConfigfile.c_str());
        if (atoi(strPath) == 0)
            return 0;

        GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, pThis->strConfigfile.c_str());
        QString dir_strCar = QString(strPath)+PLATPATH+"carinfo";

        int nLength=5*1024*1024;
        char *pack=(char *)malloc(nLength);
        if (pack==NULL)
        {
            pThis->OutputLog2("Alloc 5M Memory Error...");
        }

        GetPrivateProfileString("SYS", "webservicenew", "http://27.223.76.86:8888", pThis->m_strURLnew, sizeof(pThis->m_strURLnew), pThis->strConfigfile.c_str());
        pThis->OutputLog2("webservicenew=%s", pThis->m_strURLnew);
        while(pThis->bStart)
        {
            Sleep(1000);
            QString strQuery = "SELECT * FROM Task WHERE bUploadpic = 6 order by id desc limit 100;";
            pThis->m_msgQueryRe.prepare(strQuery);
            if (!pThis->m_msgQueryRe.exec())
            {
                QString querys = pThis->m_msgQueryRe.lastError().text();
                pThis->OutputLog2("车辆重传记录查询失败 %s",querys.toStdString().c_str());
                return 0;
            }
            else
            {
                int iCount = 0;
                if (pThis->m_msgQueryRe.last())
                {
                    iCount = pThis->m_msgQueryRe.at() + 1;
                    pThis->m_msgQueryRe.first();//重新定位指针到结果集首位
                    pThis->m_msgQueryRe.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
                    if(iCount > 0)
                    {
                        pThis->OutputLog2("车辆重传msg记录%d条",iCount);
                        while (pThis->m_msgQueryRe.next())
                        {
                            int nFID = pThis->m_msgQueryRe.value("id").toInt();
                            //图片水印
                            STR_MSG msg = {0};
                            strcpy(msg.strtime,pThis->m_msgQueryRe.value("DetectionTime").toString().toLocal8Bit());
                            strcpy(msg.strcarno,pThis->m_msgQueryRe.value("CarNO").toString().toLocal8Bit());
                            strcpy(msg.strdirection,pThis->m_msgQueryRe.value("direction").toString().toLocal8Bit());
                            strcpy(msg.strcrossroad,pThis->m_msgQueryRe.value("crossroad").toString().toLocal8Bit());
                            strcpy(msg.strret,pThis->m_msgQueryRe.value("DetectionResult").toString().toLocal8Bit());
                            strcpy(msg.strprepic,pThis->m_msgQueryRe.value("ImageBaseFront").toString().toLocal8Bit());
                            strcpy(msg.strafrpic,pThis->m_msgQueryRe.value("ImageBaseBack").toString().toLocal8Bit());
                            strcpy(msg.strallpic,pThis->m_msgQueryRe.value("ImageBasePanorama").toString().toLocal8Bit());
                            strcpy(msg.strcepic,pThis->m_msgQueryRe.value("ImageBaseSide").toString().toLocal8Bit());
                            strcpy(msg.strsmallpic,pThis->m_msgQueryRe.value("ImageBaseCarNO").toString().toLocal8Bit());
                            msg.speed = pThis->m_msgQueryRe.value("Speed").toString().toInt();
                            msg.axiscount = pThis->m_msgQueryRe.value("Axles").toString().toInt();
                            msg.weight = pThis->m_msgQueryRe.value("Weight").toString().toInt();
                            msg.GBxz = pThis->m_msgQueryRe.value("XZ").toString().toInt();
                            pThis->DrawPicSY(msg);
                            pThis->OutputLog2("重传DrawPicSY...");
                            if(pThis->m_netType == 0)
                            {
                                char filecar[256] = {};
                                sprintf(filecar, "%s%s%s.ini", dir_strCar.toStdString().c_str(),PLATPATH,msg.strcarno);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byPlateType = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byColor = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byBright = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byLicenseLen = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byEntireBelieve = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byVehicleType = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byColorDepth = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byColorCar = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.wLength = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byDir = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byVehicleLogoRecog = atoi(strPath);
                                GetPrivateProfileString("Car", "byPlateType", "0", strPath, 100, filecar);
                                msg.byVehicleType2 = atoi(strPath);
                                try
                                {
                                    int n=128;  //报文头固定128
                                    memset(pack,0,nLength);
                                    //结果信息
                                    memcpy(&pack[n],&nFID,sizeof(int));
                                    n=n+sizeof(int);
                                    pack[n] = 2;         //检测方式
                                    n=n+1;
                                    pack[n] = 0;         //卡口
                                    n=n+1;
                                    int dwChannel = 1;
                                    {
                                        char  szVal[200], strSection[20];
                                        sprintf(strSection, "车道%d", pThis->m_msgQuery.value("laneno").toString().toInt());
                                        GetPrivateProfileString(strSection, "前相机", "", szVal, 19, pThis->strConfigfile.c_str());
                                        int iPreID = atoi(szVal);
                                        auto ifind = pThis->m_maplaneAndDevice.find(iPreID);
                                        if (ifind != pThis->m_maplaneAndDevice.end())
                                        {
                                            dwChannel = ifind->second->lChanlNumPre;
                                        }
                                    }
                                    pack[n] = (unsigned char)dwChannel;   //通道号
                                    n=n+1;
                                    int lane = pThis->m_msgQueryRe.value("laneno").toString().toInt();
                                    pack[n] = (unsigned char)lane;  //车道
                                    n=n+1;
                                    QString stime = pThis->m_msgQueryRe.value("DetectionTime").toString();
                                    stime.replace(" ","-");
                                    stime.replace(":","-");
                                    stime = stime + "-123";
                                    memcpy(&pack[n],stime.toStdString().c_str(),32);
                                    n=n+32;
                                    n=n+48;
                                    pack[n] = 0;         //是否携带消息
                                    n=n+4;
                                    pack[n] = 0;         //附件消息长
                                    n=n+4;
                                    pack[n] = 3;         //图片数量
                                    n=n+4;
                                    memcpy(&pack[n],&msg.weight,sizeof(int));
                                    n=n+4;
                                    memcpy(&pack[n],&msg.GBxz,sizeof(int));
                                    n=n+4;
                                    pack[n] = 0;         //轴型
                                    n=n+4;
                                    pack[n] = (unsigned char)msg.axiscount;  //轴数
                                    n=n+1;
                                    n=n+3;
                                    //车牌信息
                                    pack[n] = (unsigned char)msg.byPlateType;         //车牌类型
                                    n=n+1;
                                    pack[n] = (unsigned char)msg.byColor;         //车牌颜色
                                    n=n+1;
                                    pack[n] = (unsigned char)msg.byBright;         //车牌亮度
                                    n=n+1;
                                    pack[n] = (unsigned char)msg.byLicenseLen;         //车牌字符个数
                                    n=n+1;
                                    memcpy(&pack[n],msg.strcarno,16);
                                    n=n+16;
                                    pack[n] = (unsigned short)msg.byEntireBelieve; //置信
                                    n=n+2;
                                    n=n+10;
                                    //车辆信息定义
                                    pack[n] = (unsigned char)msg.byVehicleType;         //车辆类型
                                    n=n+1;
                                    pack[n] = (unsigned char)msg.byColorDepth;         //车身颜色深浅
                                    n=n+1;
                                    pack[n] = (unsigned char)msg.byColorCar;         //车身颜色
                                    n=n+1;
                                    pack[n] = (unsigned char)msg.byDir;         //方向
                                    n=n+1;
                                    pack[n] = (unsigned short)msg.speed;
                                    n=n+2;
                                    pack[n] = (unsigned short)msg.wLength;  //车身长度
                                    n=n+2;
                                    pack[n] = 0;         //
                                    n=n+4;
                                    pack[n] = 0;         //
                                    n=n+4;
                                    pack[n] = (unsigned short)msg.byVehicleLogoRecog;  //车标
                                    n=n+2;
                                    pack[n] = msg.byVehicleType2;         //车辆分类
                                    n=n+1;
                                    pack[n] = 0;         //附加
                                    n=n+1;
                                    n=n+16;
                                    //图片组
                                    QString sfile = pThis->m_msgQueryRe.value("ImageBaseFront").toString();
                                    QFile file1(sfile);
                                    int ipic1=file1.size();
                                    sfile = pThis->m_msgQueryRe.value("ImageBaseBack").toString();
                                    QFile file2(sfile);
                                    int ipic2=file2.size();
                                    sfile = pThis->m_msgQueryRe.value("ImageBaseCarNO").toString();
                                    QFile file3(sfile);
                                    int ipic3=file3.size();
                                    int iTotal = ipic1+ipic2+ipic3;
                                    memcpy(&pack[n],&iTotal,sizeof(int)); //图片总长度
                                    n=n+sizeof(int);
                                    //1
                                    memcpy(&pack[n],&ipic1,sizeof(int)); //图片长度
                                    n=n+sizeof(int);
                                    memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                    n=n+32;
                                    pack[n] = (unsigned short)2;   //图片类型
                                    n=n+2;
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    n=n+4;  //帧号
                                    int w=0,h=0;
                                    QPixmap pix = QPixmap(QString::fromLocal8Bit(msg.strprepic));
                                    if(!pix.isNull())
                                    {
                                        QImage image = pix.toImage();
                                        w = image.width();
                                        h = image.height();
                                    }
                                    pack[n] = (unsigned short)w;   //w
                                    n=n+2;
                                    pack[n] = (unsigned short)h;   //h
                                    n=n+2;
                                    n=n+8;
                                    //2
                                    memcpy(&pack[n],&ipic2,sizeof(int)); //图片长度
                                    n=n+sizeof(int);
                                    memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                    n=n+32;
                                    pack[n] = (unsigned short)2;   //图片类型
                                    n=n+2;
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    n=n+4;  //帧号
                                    w=0;
                                    h=0;
                                    pix = QPixmap(QString::fromLocal8Bit(msg.strafrpic));
                                    if(!pix.isNull())
                                    {
                                        QImage image = pix.toImage();
                                        w = image.width();
                                        h = image.height();
                                    }
                                    pack[n] = (unsigned short)w;   //w
                                    n=n+2;
                                    pack[n] = (unsigned short)h;   //h
                                    n=n+2;
                                    n=n+8;
                                    //3
                                    memcpy(&pack[n],&ipic3,sizeof(int)); //图片长度
                                    n=n+sizeof(int);
                                    memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                    n=n+32;
                                    pack[n] = (unsigned short)0;   //图片类型
                                    n=n+2;
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    n=n+4;  //帧号
                                    w=0;
                                    h=0;
                                    pix = QPixmap(QString::fromLocal8Bit(msg.strsmallpic));
                                    if(!pix.isNull())
                                    {
                                        QImage image = pix.toImage();
                                        w = image.width();
                                        h = image.height();
                                    }
                                    pack[n] = (unsigned short)w;   //w
                                    n=n+2;
                                    pack[n] = (unsigned short)h;   //h
                                    n=n+2;
                                    n=n+8;
                                    //4
                                    int ipic4 = 0;
                                    memcpy(&pack[n],&ipic4,sizeof(int));    //图片长度
                                    n=n+sizeof(int);
                                    memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                    n=n+32;
                                    pack[n] = (unsigned short)0;   //图片类型
                                    n=n+2;
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    n=n+4;  //帧号
                                    w=0;
                                    h=0;
                                    pack[n] = (unsigned short)w;   //w
                                    n=n+2;
                                    pack[n] = (unsigned short)h;   //h
                                    n=n+2;
                                    n=n+8;
                                    //5
                                    int ipic5 = 0;
                                    memcpy(&pack[n],&ipic5,sizeof(int));    //图片长度
                                    n=n+sizeof(int);
                                    memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                    n=n+32;
                                    pack[n] = (unsigned short)0;   //图片类型
                                    n=n+2;
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    n=n+4;  //帧号
                                    w=0;
                                    h=0;
                                    pack[n] = (unsigned short)w;   //w
                                    n=n+2;
                                    pack[n] = (unsigned short)h;   //h
                                    n=n+2;
                                    n=n+8;
                                    //6
                                    int ipic6 = 0;
                                    memcpy(&pack[n],&ipic6,sizeof(int));    //图片长度
                                    n=n+sizeof(int);
                                    memcpy(&pack[n],stime.toStdString().c_str(),32);  //sj
                                    n=n+32;
                                    pack[n] = (unsigned short)0;   //图片类型
                                    n=n+2;
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //x len
                                    n=n+2;
                                    pack[n] = (unsigned short)0;   //y len
                                    n=n+2;
                                    n=n+4;  //帧号
                                    w=0;
                                    h=0;
                                    pack[n] = (unsigned short)w;   //w
                                    n=n+2;
                                    pack[n] = (unsigned short)h;   //h
                                    n=n+2;
                                    n=n+8;
                                    //memcpy1~6
                                    {
                                        FILE* config = NULL;
                                        char *buffer = NULL;
                                        buffer = (char *)malloc(ipic1);
                                        if (buffer!=NULL)
                                        {
                                            config = fopen(msg.strprepic,"rb"); //读二进制文件和可写，如没有权限写或者读为NULL
                                            if(config != NULL)
                                            {
                                                fread(buffer,1,ipic1,config);
                                                fclose(config);
                                            }
                                        }
                                        memcpy(&pack[n],buffer,ipic1);
                                        n=n+ipic1;
                                        free(buffer);
                                    }
                                    {
                                        FILE* config = NULL;
                                        char *buffer = NULL;
                                        buffer = (char *)malloc(ipic2);
                                        if (buffer!=NULL)
                                        {
                                            config = fopen(msg.strafrpic,"rb"); //读二进制文件和可写，如没有权限写或者读为NULL
                                            if(config != NULL)
                                            {
                                                fread(buffer,1,ipic2,config);
                                                fclose(config);
                                            }
                                        }
                                        memcpy(&pack[n],buffer,ipic2);
                                        n=n+ipic2;
                                        free(buffer);
                                    }
                                    {
                                        FILE* config = NULL;
                                        char *buffer = NULL;
                                        buffer = (char *)malloc(ipic3);
                                        if (buffer!=NULL)
                                        {
                                            config = fopen(msg.strsmallpic,"rb"); //读二进制文件和可写，如没有权限写或者读为NULL
                                            if(config != NULL)
                                            {
                                                fread(buffer,1,ipic3,config);
                                                fclose(config);
                                            }
                                        }
                                        memcpy(&pack[n],buffer,ipic3);
                                        n=n+ipic3;
                                        free(buffer);
                                    }
                                    //总
                                    int m = 0;
                                    int iAll = n;
                                    memcpy(&pack[m],&iAll,sizeof(int)); //图片长度
                                    m=m+sizeof(int);
                                    char version[8] = "V2.0";
                                    memcpy(&pack[m],version,8); //图片长度
                                    m=m+8;
                                    pack[m] = (unsigned int)170;  //识别结果上传 170
                                    m=m+4;
                                    memcpy(&pack[m],pThis->m_webuser,32); //图片长度
                                    m=m+32;
                                    memcpy(&pack[m],pThis->m_webxlnum,48); //图片长度
                                    m=m+48;
                                    memcpy(&pack[m],pThis->m_webip,24); //图片长度
                                    m=m+24;
                                    pack[m] = (unsigned int)pThis->m_webport;
                                    m=m+2;
                                    pack[m] = 1;
                                    m=m+1;
                                    m=m+5;
                                    //////////////////////////////////////////////////////////////////////////
                                    SOCKET m_socketClient = socket(AF_INET, SOCK_STREAM, 0);
                                    if (m_socketClient != INVALID_SOCKET)
                                    {
                                        struct sockaddr_in ServerAddr = {};
                                        ServerAddr.sin_addr.S_un.S_addr = inet_addr(pThis->m_szServerIp);
                                        ServerAddr.sin_family = AF_INET;
                                        ServerAddr.sin_port = htons(pThis->m_nPort);
                                        if (connect(m_socketClient, (PSOCKADDR)&ServerAddr, sizeof(ServerAddr)) != SOCKET_ERROR)
                                        {
                                            int timeout = 5000; //5s
                                            setsockopt(m_socketClient, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
                                            send(m_socketClient, pack, iAll, 0);
                                            Sleep(100);
                                            char szRet[1024];
                                            memset(&szRet, 0x00, sizeof(szRet));
                                            int nLen = recv(m_socketClient, szRet, 1023, 0);
                                            shutdown(m_socketClient, SD_BOTH);
                                            closesocket(m_socketClient);
                                            if(nLen == 128)
                                            {
                                                QSqlQuery qsQuery = QSqlQuery(g_DB);
                                                QString strSqlText = QString("UPDATE Task SET bUploadpic=1 WHERE id = %1;").arg(nFID);
                                                qsQuery.prepare(strSqlText);
                                                if (!qsQuery.exec())
                                                {
                                                    QString querys = qsQuery.lastError().text();
                                                    pThis->OutputLog2("%d bUpload=1 webId error %s",nFID,querys.toStdString().c_str());
                                                }
                                            }
                                        }
                                        else
                                            OutputLog("重传Server连接失败 !! %d", WSAGetLastError());
                                    }
                                    else
                                        OutputLog("重传socket失败 !! %d", WSAGetLastError());
                                    //////////////////////////////////////////////////////////////////////////
                                }
                                catch (...)
                                {
                                    printf("Send Err");
                                }
                            }
                            if(pThis->m_netType == 1)
                            {
                                QNetworkRequest request;    //(注意这个地方不能设置head)
                                char url[1024] = {0};
                                sprintf(url, POST_DataOverMsg_URL, pThis->m_strURLnew);
                                request.setUrl(QUrl(url));

                                QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
                                QString fileName = pThis->m_msgQueryRe.value("ImageBaseFront").toString();
                                if(fileName.length() > 1)
                                {
                                    QHttpPart image_part;
                                    image_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                    image_part.setHeader(QNetworkRequest::ContentDispositionHeader,QVariant(QString("form-data; name=\"frontPhoto\";filename=\"%1\";").arg(fileName)));
                                    QFile *file = new QFile(fileName);      //由于是异步，所以记得一定要new
                                    file->open(QIODevice::ReadOnly);
                                    image_part.setBodyDevice(file);          //方便后续内存释放
                                    file->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                    multiPart->append(image_part);
                                }

                                QString fileName2 = pThis->m_msgQueryRe.value("ImageBaseBack").toString();
                                if(fileName2.length() > 1)
                                {
                                    QHttpPart image_part2;
                                    image_part2.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                    image_part2.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"blackPhoto\";filename=\"%1\";").arg(fileName2)));
                                    QFile *file2 = new QFile(fileName2);      //由于是异步，所以记得一定要new
                                    file2->open(QIODevice::ReadOnly);
                                    image_part2.setBodyDevice(file2);          //方便后续内存释放
                                    file2->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                    multiPart->append(image_part2);
                                }

                                QString fileName3 = pThis->m_msgQueryRe.value("ImageBaseSide").toString();
                                if(fileName3.length() > 1)
                                {
                                    QHttpPart image_part3;
                                    image_part3.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                    image_part3.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"sidePhoto\";filename=\"%1\";").arg(fileName3)));
                                    QFile *file3 = new QFile(fileName3);      //由于是异步，所以记得一定要new
                                    file3->open(QIODevice::ReadOnly);
                                    image_part3.setBodyDevice(file3);          //方便后续内存释放
                                    file3->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                    multiPart->append(image_part3);
                                }

                                QString fileName4 = pThis->m_msgQueryRe.value("ImageBaseCarNO").toString();
                                if(fileName4.length() > 1)
                                {
                                    QHttpPart image_part4;
                                    image_part4.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpg"));
                                    image_part4.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"platePhoto\";filename=\"%1\";").arg(fileName4)));
                                    QFile *file4 = new QFile(fileName4);      //由于是异步，所以记得一定要new
                                    file4->open(QIODevice::ReadOnly);
                                    image_part4.setBodyDevice(file4);          //方便后续内存释放
                                    file4->setParent(multiPart);                 // we cannot delete the file now, so delete it with the multiPart
                                    multiPart->append(image_part4);
                                }

                                QString strJson = QString("{'stationId':%1, 'deviceNo':'%2', 'detectionTime':'%3', 'carPlate':'%4', 'carModel':'%5', 'wheelNumber':'%6', 'wheelBase':'%7,%8,%9,%10,%11,%12,%13', 'carLength':'%14', \
                                                          'overLength':'0', 'carWidth':'0', 'overWidth':'0', 'height':'0', 'overHeight':'0', 'wheelWeight':'%15,%16,%17,%18,%19,%20,%21,%22', 'totalWeight':'%23', 'limitWeight':'%24', 'overWeight':'%25', \
                                                          'overPercent':'%26', 'lineNo':'%27', 'speed':'%28', 'detectConclusion':'%29', 'detectType':'1'}")
                            .arg(pThis->m_stationId).arg(pThis->m_msgQueryRe.value("deviceNo").toString()).arg(pThis->m_msgQueryRe.value("DetectionTime").toString())
                                    .arg(pThis->m_msgQueryRe.value("CarNO").toString()).arg(pThis->m_msgQueryRe.value("CarType").toString()).arg(pThis->m_msgQueryRe.value("Axles").toString())
                                    .arg(pThis->m_msgQueryRe.value("AxlesSpace1").toString()).arg(pThis->m_msgQueryRe.value("AxlesSpace2").toString()).arg(pThis->m_msgQueryRe.value("AxlesSpace3").toString())
                                    .arg(pThis->m_msgQueryRe.value("AxlesSpace4").toString()).arg(pThis->m_msgQueryRe.value("AxlesSpace5").toString()).arg(pThis->m_msgQueryRe.value("AxlesSpace6").toString())
                                    .arg(pThis->m_msgQueryRe.value("AxlesSpace7").toString()).arg(pThis->m_msgQueryRe.value("carLength").toString()).arg(pThis->m_msgQueryRe.value("AxlesWeight1").toString())
                                    .arg(pThis->m_msgQueryRe.value("AxlesWeight2").toString()).arg(pThis->m_msgQueryRe.value("AxlesWeight3").toString()).arg(pThis->m_msgQueryRe.value("AxlesWeight4").toString())
                                    .arg(pThis->m_msgQueryRe.value("AxlesWeight5").toString()).arg(pThis->m_msgQueryRe.value("AxlesWeight6").toString()).arg(pThis->m_msgQueryRe.value("AxlesWeight7").toString())
                                    .arg(pThis->m_msgQueryRe.value("AxlesWeight8").toString()).arg(pThis->m_msgQueryRe.value("Weight").toString()).arg(pThis->m_GBXZ).arg(pThis->m_msgQueryRe.value("overWeight").toString())
                                    .arg(pThis->m_msgQueryRe.value("overPercent").toString()).arg(pThis->m_msgQueryRe.value("laneno").toString()).arg(pThis->m_msgQueryRe.value("Speed").toString()).arg(pThis->m_msgQueryRe.value("DetectionResult").toString());

                            pThis->OutputLog2(strJson.toLocal8Bit());
                            QHttpPart textPart;
                            textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"recordVo\""));
                            textPart.setBody(strJson.toUtf8());
                            multiPart->append(textPart);

                            QNetworkAccessManager pManger;
                            QNetworkReply *reply = pManger.post(request, multiPart);
                            multiPart->setParent(reply);
                            QEventLoop eventLoop;
                            QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
                            eventLoop.exec();
                            if (reply->error() == QNetworkReply::NoError)
                            {
                                QByteArray bytes = reply->readAll();
                                QString strRet(bytes);
                                pThis->OutputLog2(strRet.toLocal8Bit());
                                QJsonParseError parseJsonErr;
                                QJsonDocument document = QJsonDocument::fromJson(bytes,&parseJsonErr);
                                if(parseJsonErr.error == QJsonParseError::NoError)
                                {
                                    QJsonObject jsonObject = document.object();
                                    int bRet = jsonObject["code"].toInt();
                                    if(bRet == 200)
                                    {
                                        OutputLog("重传平台记录id= %s", jsonObject["data"].toString().toStdString().c_str());
                                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                                        QString strSqlText = QString("UPDATE Task SET bUploadpic=1,webno=:webno WHERE id = %1;").arg(nFID);
                                        qsQuery.prepare(strSqlText);
                                        qsQuery.bindValue(":webno", jsonObject["data"].toString());
                                        if (!qsQuery.exec())
                                        {
                                            QString querys = qsQuery.lastError().text();
                                            pThis->OutputLog2("%d bUpload=1 webId error %s",nFID,querys.toStdString().c_str());
                                        }
                                    }
                                    else
                                    {
                                        //接口报错
                                        pThis->OutputLog2("重传bRet!=200");
                                    }
                                }
                            }
                            else
                            {
                                QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                                //statusCodeV是HTTP服务器的相应码，reply->error()是Qt定义的错误码，可以参考QT的文档
                                pThis->OutputLog2("重传found error ....code: %d %d", statusCodeV.toInt(), (int)reply->error());
                                pThis->OutputLog2(reply->errorString().toStdString().c_str());
                            }
                            reply->deleteLater();
                        }
                    }
                }
            }
        }
        Sleep(20*60*1000);
    }

    free(pack);

    return 0;
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
                    char strSql[256] = {};
                    sprintf(strSql, "SELECT * FROM Task order by id asc limit 100;");
                    QString strTemp = strSql;
                    QSqlQuery query(g_DB);
                    query.prepare(strTemp);
                    if (!query.exec())
                    {
                        QString querys = query.lastError().text();
                        OutputLog("车辆记录查询失败%s",querys.toStdString().c_str());
                        return 0;
                    }
                    else
                    {
                        int iCount = 0;
                        if (query.last())
                        {
                            iCount = query.at() + 1;
                        }
                        else
                        {
                            continue;
                        }
                        query.first();//重新定位指针到结果集首位
                        query.previous();//如果执行query.next来获取结果集中数据，要将指针移动到首位的上一位。
                        {
                            while (query.next())
                            {
                                ResultRecord record;
                                record.qstrfid = query.value(0).toString();
                                record.qstrpicvideo = query.value("vedioBaseFront").toString();
                                record.qstrpicpre = query.value("ImageBaseFront").toString();
                                record.qstrpicafr = query.value("ImageBaseBack").toString();
                                record.qstrpicce = query.value("ImageBasePanorama").toString();
                                record.qstrpicsmall = query.value("ImageBaseCarNO").toString();
                                DeleteFileA((const char*)record.qstrpicvideo.toLocal8Bit());
                                DeleteFileA((const char*)record.qstrpicpre.toLocal8Bit());
                                DeleteFileA((const char*)record.qstrpicafr.toLocal8Bit());
                                DeleteFileA((const char*)record.qstrpicce.toLocal8Bit());
                                DeleteFileA((const char*)record.qstrpicsmall.toLocal8Bit());
                                //
                                QSqlQuery qsQuery = QSqlQuery(g_DB);
                                QString strSqlText = QString("delete from Task  WHERE id = %1;").arg(record.qstrfid);
                                qsQuery.prepare(strSqlText);
                                if (!qsQuery.exec())
                                {
                                    QString querys = qsQuery.lastError().text();
                                    pThis->OutputLog2("%d delete Task error %s", record.qstrfid.toInt(), querys.toStdString().c_str());
                                }
                            }
                        }
                    }
                }
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
                    bool bUpOver = false;
                    if(bCamera == 1)
                    {
                        if (it.second->bpic && it.second->bpicpre && it.second->bweight)
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
                        if (strlen(it.second->strcarno)<1)
                        {
                            pThis->OutputLog2("无车牌号");
                            strcpy(it.second->strplatecolor, "无");
                            strcpy(it.second->strcarno, "未识别");
                        }

                        pThis->OutputLog2("数据准备完成 车道%d  %s", it.second->laneno, it.second->strcarno);
                        //无效处理
                        strcpy(it.second->strreason, " ");
                        int iRet = 0;
                        if(it.second->speed > 180)
                        {
                            strcpy(it.second->strreason, "速度异常");
                            iRet = 7;
                        }
                        if(it.second->weight < 500)
                        {
                            strcpy(it.second->strreason, "总重量异常");
                            iRet = 7;
                        }
                        switch (it.second->axiscount) {
                        case 1:
                            iRet = 7;
                            strcpy(it.second->strreason, "轴数异常");
                            break;
                        case 2:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 3:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 4:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200 || it.second->Axis_weight[3]<200)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 5:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200
                                    || it.second->Axis_weight[3]<200 || it.second->Axis_weight[4]<200)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 6:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200
                                    || it.second->Axis_weight[3]<200 || it.second->Axis_weight[4]<200 || it.second->Axis_weight[5]<200)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 7:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200
                                    || it.second->Axis_weight[3]<200 || it.second->Axis_weight[4]<200 || it.second->Axis_weight[5]<200
                                    || it.second->Axis_weight[6]<200)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        case 8:
                            if(it.second->Axis_weight[0]<200 || it.second->Axis_weight[1]<200 || it.second->Axis_weight[2]<200
                                    || it.second->Axis_weight[3]<200 || it.second->Axis_weight[4]<200 || it.second->Axis_weight[5]<200
                                    || it.second->Axis_weight[6]<200 || it.second->Axis_weight[7]<200)
                            {
                                strcpy(it.second->strreason, "单轴重异常");
                                iRet = 7;
                            }
                            break;
                        default:
                            break;
                        }
                        //
                        //保存数据
                        QSqlQuery qsQuery = QSqlQuery(g_DB);
                        pThis->OutputLog2("保存分析任务 %s", it.second->strcarno);
                        QString strInsert = "INSERT INTO Task(DetectionTime,CarNO,laneno,Axles,CarType,deviceNo,carLength,Speed,Weight,PlateColor,DetectionResult,ImageBaseFront,ImageBaseBack,ImageBaseSide,\
                                ImageBasePanorama,ImageBaseCarNO,AxlesWeight1,AxlesWeight2,AxlesWeight3,AxlesWeight4,AxlesWeight5,AxlesWeight6,AxlesWeight7,AxlesWeight8,AxlesSpace1,AxlesSpace2,AxlesSpace3,AxlesSpace4,AxlesSpace5,AxlesSpace6,AxlesSpace7,\
                                bUploadpic,bUploadvedio,webId,overWeight,overPercent,direction,crossroad,XZ,reason) VALUES(:DetectionTime,:CarNO,:laneno,:Axles,:CarType,:deviceNo,:carLength,:Speed,:Weight,:PlateColor,:DetectionResult,:ImageBaseFront,:ImageBaseBack,:ImageBaseSide,\
                                                                :ImageBasePanorama,:ImageBaseCarNO,:AxlesWeight1,:AxlesWeight2,:AxlesWeight3,:AxlesWeight4,:AxlesWeight5,:AxlesWeight6,:AxlesWeight7,:AxlesWeight8,\
                                                                :AxlesSpace1,:AxlesSpace2,:AxlesSpace3,:AxlesSpace4,:AxlesSpace5,:AxlesSpace6,:AxlesSpace7,:bUploadpic,:bUploadvedio,:webId,:overWeight,:overPercent,:direction,:crossroad,:XZ,:reason); ";
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
                        qsQuery.bindValue(":ImageBaseBack", QString::fromLocal8Bit(it.second->strafrpic));
                        qsQuery.bindValue(":ImageBaseSide", QString::fromLocal8Bit(it.second->strcepic));
                        qsQuery.bindValue(":ImageBasePanorama", QString::fromLocal8Bit(it.second->strallpic));
                        qsQuery.bindValue(":ImageBaseCarNO", QString::fromLocal8Bit(it.second->strsmallpic));
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
                        qsQuery.bindValue(":bUploadvedio", 0);
                        qsQuery.bindValue(":direction", QString::fromLocal8Bit(it.second->strdirection));
                        qsQuery.bindValue(":crossroad", QString::fromLocal8Bit(it.second->strcrossroad));
                        qsQuery.bindValue(":webId", 0);
                        qsQuery.bindValue(":overWeight", it.second->overWeight);
                        qsQuery.bindValue(":overPercent", QString::number(it.second->overPercent,'f',2));
                        qsQuery.bindValue(":XZ", it.second->GBxz);
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
                            pThis->UpdateUI(&msg);
                            pThis->UpdateLed(&msg);
                            pThis->m_bNeedFlush = true;
                            pThis->m_LastTic = ::GetTickCount();
                            pThis->m_QueueSectVedio.Lock();
                            pThis->m_MsgVedioQueue.push(msg);
                            pThis->m_QueueSectVedio.Unlock();
                            pThis->OutputLog2("启动录像 %s, real size=%d", it.second->strcarno, pThis->m_MsgVedioQueue.size());
                        }
                        //下个过车
                        memset(it.second, 0, sizeof(STR_MSG));
                    }
                    else
                    {
                        if (::GetTickCount() - it.second->getticout > pThis->m_Timeout)
                        {
                            if ((it.second->bpic || it.second->bpicpre) && it.second->bweight)
                            {
                                pThis->OutputLog2("超时已有bpic  %d  %d  %d", it.second->bpicpre, it.second->bpic,it.second->bweight);
                                if(it.second->bpic == false)
                                    strcpy(it.second->strafrpic, it.second->strprepic);
                                if(it.second->bpicpre == false)
                                    strcpy(it.second->strprepic, it.second->strafrpic);
                                it.second->bpic = it.second->bpicpre = it.second->bweight = true;
                            }
                            else
                            {

                                pThis->OutputLog2("3s超时无效,车道%d  %s", it.second->laneno, it.second->strcarno);
                                DeleteFileA(it.second->strprepic);
                                DeleteFileA(it.second->strafrpic);
                                DeleteFileA(it.second->strsmallpic);
                                DeleteFileA(it.second->strcepic);
                                memset(it.second, 0, sizeof(STR_MSG));
                            }
                        }
                    }
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

void* ParseDataThreadProc(LPVOID pParam)
{
#ifdef WIN32
    int port = (int)pParam;
#else
    int port = (intptr_t)pParam;
#endif

    //通道使用数量
    char strSection[20],temp[6];
    sprintf(strSection, "仪表%d", port+1);
    ::GetPrivateProfileString(strSection, "通道使用数量", "0", temp, 5, g_Over->strConfigfile.c_str());
    int iNum = atoi(temp);
    //设备id
    QString strCom;
    static int oldsn = 0;
    auto iifind = g_Over->m_mapComAndID.find(port);
    if (iifind == g_Over->m_mapComAndID.end())
        return 0;
    else
    {
        Device_MSG *pMsg = iifind->second;
        strCom = pMsg->deviceid;
    }

    g_Over->OutputLog2("%d-->%s",port,strCom.toStdString().c_str());
    while (1)
    {
        try
        {
            unsigned char chRevc[128] = {};
            int nRet = g_Over->m_comwraper[port]->GetPacket(0x10, 0x5a, chRevc);
            if (nRet != 0)
            {
                if (nRet != 70)
                {
                    g_Over->OutputLog2("长度!=70   %s",chRevc);
                    continue;
                }
                //解析
                long iCur = 0;
                if(chRevc[1] >=0xF0)
                    iCur = chRevc[1]*256*256*256+chRevc[0]*256*256+chRevc[3]*256+chRevc[2]-65536*65536;
                else
                    iCur = chRevc[1]*256*256*256+chRevc[0]*256*256+chRevc[3]*256+chRevc[2];

                if (oldsn + 1 != iCur)
                    g_Over->OutputLog2("丢包: %d %d", oldsn, iCur);
                oldsn = iCur;

                if (oldsn % 50000 == 0)
                    g_Over->OutputLog2("===================");

                int data[64] = {};
                int j=0;
                std::map<int, int> maptmp; //
                for (int i=0; i<iNum; i++)
                {
                    if(chRevc[4 + j * 2 + 1]>=0xF0)
                        data[i]=(chRevc[4 + j * 2 + 1]*256+chRevc[4 + j * 2]-65536);
                    else
                        data[i]=chRevc[4 + j * 2 + 1]*256+chRevc[4 + j * 2];
                    j++;
//                    if(j>5)
//                        j=0;

                    QString strKey = QString("%1,%2").arg(strCom).arg(i+1);
                    auto ifind = g_Over->m_mapComAndDeviceID.find(strKey);
                    if (ifind != g_Over->m_mapComAndDeviceID.end())
                    {
                        DataNode *node = ifind->second;
                        g_Over->ProcessData(node->ilane, node->iSensor, iCur, data[i]);
                        if (oldsn % 50000 == 0)
                        {
                            g_Over->OutputLog2("%d  %d  %d  %d", node->ilane, node->iSensor, iCur, data[i]);
                        }
                        if(node->ilane == loglane)
                            maptmp[node->iSensor] = data[i];
                    }
                }
                if (g_Over->m_bStartGetSample)
                    LOG4CPLUS_DEBUG(pTestLogger, "\t"<<iCur<<"\t"<<maptmp.at(0)<<"\t"<<maptmp.at(1)<<"\t"<<maptmp.at(2)<<"\t"<<maptmp.at(3)<<"\t"<<maptmp.at(4) <<"\t"<<maptmp.at(5));
                //if (g_Over->m_bStartGetSample)
                    //LOG4CPLUS_DEBUG(pTestLogger, "\t"<<iCur<<"\t"<<data[0]<<"\t"<<data[1]<<"\t"<<data[2]<<"\t"<<data[3]<<"\t"<<data[4] <<"\t"<<data[5]);
            }
            else
                Sleep(2);
        }
        catch (...)
        {
            g_Over->OutputLog2("ParseDataThreadProc数据处理异常!!!!!!!!!!");
        }
    }

    return 0;
}

void* ChnlDataThreadProc(LPVOID pParam)
{
#ifdef WIN32
    int port = (int)pParam;
#else
    int port = (intptr_t)pParam;
#endif

    auto iifind = g_Over->m_mapComAndID.find(port);
    if (iifind == g_Over->m_mapComAndID.end())
        return 0;
    else
    {
        Device_MSG *pMsg = iifind->second;
        unsigned char buf[102400] = {0};
        SOCKET sock_Client=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);//创建客户端用于通信的Socket
        int nRecvBuf = 1024 * 1024;//设置为1024K
        if (0 != setsockopt(sock_Client, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int)))
        {
            g_Over->OutputLog2("setsockopt buf size failed ");
            return FALSE;
        }

#ifdef WIN32
        //设置为非阻塞模式
        int imode=1;
        int rev=ioctlsocket(sock_Client,FIONBIO,(u_long *)&imode);
        if(rev == SOCKET_ERROR)
        {
            g_Over->OutputLog2("ioctlsocket failed!");
            return FALSE;
        }

        SOCKADDR_IN addr_Local; //服务器的地址等信息
        addr_Local.sin_family=AF_INET;
        addr_Local.sin_port=htons(5030);
        addr_Local.sin_addr.S_un.S_addr=INADDR_ANY;
        if(::bind(sock_Client,(SOCKADDR*)&addr_Local,sizeof(addr_Local))==SOCKET_ERROR )
        {
            g_Over->OutputLog2("bind failed %d", WSAGetLastError());
            return FALSE;
        }

        SOCKADDR_IN server;
        int len =sizeof(server);
        server.sin_family=AF_INET;
        server.sin_port=htons(pMsg->port);          //server的监听端口
        server.sin_addr.s_addr=inet_addr(pMsg->ip); //server的地址
        g_Over->OutputLog2("设备%s %d开始通讯", pMsg->ip, pMsg->port);

        fd_set rfd;                     // 描述符集 这个将用来测试有没有一个可用的连接
        struct timeval timeout;
        timeout.tv_sec=0;               //等下select用到这个
        timeout.tv_usec=0;              //timeout设置为0，可以理解为非阻塞
        int SelectRcv;

        while(1)
        {
            // UDP数据接收
            FD_ZERO(&rfd);           //总是这样先清空一个描述符集
            FD_SET(sock_Client,&rfd); //把sock放入要测试的描述符集
            SelectRcv = select(sock_Client+1,&rfd,0,0, &timeout); //检查该套接字是否可读
            if (SelectRcv > 0)
            {
                rev=0;
                rev=recvfrom(sock_Client,(char*)buf,sizeof(buf)-1,0,(SOCKADDR*)&server,&len);
                if (rev!=SOCKET_ERROR)
                {
                    g_Over->m_comwraper[port]->AddBuff(buf, rev);
                }
            }
        }
#else
        //设置为非阻塞模式
        int flag = fcntl(sock_Client, F_GETFL, 0);
        if (flag < 0) {
            g_Over->OutputLog2("fcntl F_GETFL fail");
            return FALSE;
        }
        if (fcntl(sock_Client, F_SETFL, flag | O_NONBLOCK) < 0) {
            g_Over->OutputLog2("fcntl F_SETFL fail");
        }

        sockaddr_in addr_Local; //服务器的地址等信息
        addr_Local.sin_family=AF_INET;
        addr_Local.sin_port=htons(5030);
        addr_Local.sin_addr.s_addr = htonl(INADDR_ANY); //
        if(::bind(sock_Client,(sockaddr*)&addr_Local,sizeof(addr_Local))==SOCKET_ERROR )
        {
            g_Over->OutputLog2("bind failed");
            return FALSE;
        }

        sockaddr_in server;
        socklen_t len =sizeof(server);
        server.sin_family=AF_INET;
        server.sin_port=htons(pMsg->port);          //server的监听端口
        server.sin_addr.s_addr=inet_addr(pMsg->ip); //server的地址
        g_Over->OutputLog2("设备%s %d开始通讯", pMsg->ip, pMsg->port);

        fd_set rfd;                     // 描述符集 这个将用来测试有没有一个可用的连接
        struct timeval timeout;
        timeout.tv_sec=0;               //等下select用到这个
        timeout.tv_usec=0;              //timeout设置为0，可以理解为非阻塞
        int SelectRcv;

        while(1)
        {
            // UDP数据接收
            FD_ZERO(&rfd);           //总是这样先清空一个描述符集
            FD_SET(sock_Client,&rfd); //把sock放入要测试的描述符集
            SelectRcv = select(sock_Client+1,&rfd,0,0, &timeout); //检查该套接字是否可读
            if (SelectRcv > 0)
            {
                int rev=recvfrom(sock_Client,(char*)buf,sizeof(buf)-1,0,(sockaddr*)&server,&len);
                if (rev!=SOCKET_ERROR)
                {
                    g_Over->m_comwraper[port]->AddBuff(buf, rev);
                }
            }
        }
#endif

    }

    return 0;
}

void* VedioMsgThreadProc(LPVOID pParam)
{
    OverWidget* pThis = reinterpret_cast<OverWidget*>(pParam);
    char strPath[256] = {};
    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, pThis->strConfigfile.c_str());
    char  szVal[200], strSection[20];
    STR_MSG nPermsg;
    while (pThis->bStart)
    {
        Sleep(200);
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
                    nPermsg = pThis->m_MsgVedioQueue.front();
                    pThis->m_MsgVedioQueue.pop();
                    pThis->m_QueueSectVedio.Unlock();
                }
                else
                {
                    pThis->m_QueueSectVedio.Unlock();
                    continue;
                }
                //!!!
                bool bDownOk = true;
                char filenamepre[256] = {};
                sprintf(filenamepre, "%s%s%d-%d-%s-前.mp4", strPath, PLATPATH, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
                char filenamepreS[256] = {};
                sprintf(filenamepreS, "%s%s%d-%d-%s-前S.mp4", strPath, PLATPATH, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
                pThis->OutputLog2(filenamepre);
                pThis->OutputLog2(nPermsg.strtime);
                char sss[256] = {};
                sprintf(strSection, "车道%d", nPermsg.laneno);
                GetPrivateProfileString(strSection, "前相机", "", szVal, 19, pThis->strConfigfile.c_str());
                int iPreID = atoi(szVal);
                auto ifind = pThis->m_maplaneAndDevice.find(iPreID);
                if (ifind != pThis->m_maplaneAndDevice.end())
                {
                    //前
                    do
                    {
                        NET_DVR_PLAYCOND struDownloadCond = { 0 };
                        struDownloadCond.dwChannel = ifind->second->lChanlNumPre + pThis->m_lStartChnDVR - 1;
                        time_t timet = StringToDatetime(nPermsg.strtime) - 3;
                        tm *tm_ = localtime(&timet);
                        struDownloadCond.struStartTime.dwYear = tm_->tm_year + 1900;
                        struDownloadCond.struStartTime.dwMonth = tm_->tm_mon + 1;
                        struDownloadCond.struStartTime.dwDay = tm_->tm_mday;
                        struDownloadCond.struStartTime.dwHour = tm_->tm_hour;
                        struDownloadCond.struStartTime.dwMinute = tm_->tm_min;
                        struDownloadCond.struStartTime.dwSecond = tm_->tm_sec;
                        pThis->OutputLog2("开始%d %d %d %d %d %d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
                                          struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
                        sprintf_s(sss, "%04d-%02d-%02d %02d-%02d-%02d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
                                  struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
                        timet = StringToDatetime(nPermsg.strtime) + 5;
                        tm_ = localtime(&timet);
                        struDownloadCond.struStopTime.dwYear = tm_->tm_year + 1900;
                        struDownloadCond.struStopTime.dwMonth = tm_->tm_mon + 1;
                        struDownloadCond.struStopTime.dwDay = tm_->tm_mday;
                        struDownloadCond.struStopTime.dwHour = tm_->tm_hour;
                        struDownloadCond.struStopTime.dwMinute = tm_->tm_min;
                        struDownloadCond.struStopTime.dwSecond = tm_->tm_sec;
                        pThis->OutputLog2("结束%d %d %d %d %d %d", struDownloadCond.struStopTime.dwYear, struDownloadCond.struStopTime.dwMonth, struDownloadCond.struStopTime.dwDay,
                                          struDownloadCond.struStopTime.dwHour, struDownloadCond.struStopTime.dwMinute, struDownloadCond.struStopTime.dwSecond);
                        //---------------------------------------
                        //按时间下载
                        int hPlayback = NET_DVR_GetFileByTime_V40(pThis->m_lHandleDVR, filenamepre, &struDownloadCond);
                        if (hPlayback < 0)
                        {
                            pThis->OutputLog2("前NET_DVR_GetFileByTime_V40 fail,last error %d", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                        //---------------------------------------
                        //开始下载
                        if (!NET_DVR_PlayBackControl_V40(hPlayback, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL))
                        {
                            pThis->OutputLog2("前Play back control failed [%d]", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                        int nPos = 0;
                        for (nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(hPlayback))
                        {
                            pThis->OutputLog2("Be downloading... %d", nPos);
                            Sleep(2000);  //millisecond
                        }
                        pThis->OutputLog2("前录像ok... %d", nPos);
                        if (!NET_DVR_StopGetFile(hPlayback))
                        {
                            pThis->OutputLog2("前failed to stop get file [%d]", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                        if (nPos < 0 || nPos>100)
                        {
                            pThis->OutputLog2("前download err [%d]\n", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                    }
                    while (0);
                }

//                char filenameafr[256] = {};
//                sprintf(filenameafr, "%s\\%d-%d-%s-后.mp4", strPath, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
//                char filenameafrS[256] = {};
//                sprintf(filenameafrS, "%s\\%d-%d-%s-后S.mp4", strPath, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
//                pThis->OutputLog2(filenameafr);
//                GetPrivateProfileString(strSection, "后相机", "", szVal, 19, pThis->strConfigfile.c_str());
//                int iAfrID = atoi(szVal);
//                auto ifind2 = pThis->m_maplaneAndDevice.find(iAfrID);
//                if (ifind2 != pThis->m_maplaneAndDevice.end())
//                {
//                    //后
//                    do
//                    {
//                        NET_DVR_PLAYCOND struDownloadCond = { 0 };
//                        struDownloadCond.dwChannel = ifind2->second->lChanlNumAfr + pThis->m_lStartChnDVR - 1;
//                        time_t timet = StringToDatetime(nPermsg.strtime) - 2;
//                        tm *tm_ = localtime(&timet);
//                        struDownloadCond.struStartTime.dwYear = tm_->tm_year + 1900;
//                        struDownloadCond.struStartTime.dwMonth = tm_->tm_mon + 1;
//                        struDownloadCond.struStartTime.dwDay = tm_->tm_mday;
//                        struDownloadCond.struStartTime.dwHour = tm_->tm_hour;
//                        struDownloadCond.struStartTime.dwMinute = tm_->tm_min;
//                        struDownloadCond.struStartTime.dwSecond = tm_->tm_sec;
//                        pThis->OutputLog2("开始%d %d %d %d %d %d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
//                                          struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
//                        timet = StringToDatetime(nPermsg.strtime) + 4;
//                        tm_ = localtime(&timet);
//                        struDownloadCond.struStopTime.dwYear = tm_->tm_year + 1900;
//                        struDownloadCond.struStopTime.dwMonth = tm_->tm_mon + 1;
//                        struDownloadCond.struStopTime.dwDay = tm_->tm_mday;
//                        struDownloadCond.struStopTime.dwHour = tm_->tm_hour;
//                        struDownloadCond.struStopTime.dwMinute = tm_->tm_min;
//                        struDownloadCond.struStopTime.dwSecond = tm_->tm_sec;
//                        pThis->OutputLog2("结束%d %d %d %d %d %d", struDownloadCond.struStopTime.dwYear, struDownloadCond.struStopTime.dwMonth, struDownloadCond.struStopTime.dwDay,
//                                          struDownloadCond.struStopTime.dwHour, struDownloadCond.struStopTime.dwMinute, struDownloadCond.struStopTime.dwSecond);
//                        //---------------------------------------
//                        //按时间下载
//                        int hPlayback = NET_DVR_GetFileByTime_V40(pThis->m_lHandleDVR, filenameafr, &struDownloadCond);
//                        if (hPlayback < 0)
//                        {
//                            pThis->OutputLog2("后NET_DVR_GetFileByTime_V40 fail,last error %d", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                        //---------------------------------------
//                        //开始下载
//                        if (!NET_DVR_PlayBackControl_V40(hPlayback, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL))
//                        {
//                            pThis->OutputLog2("后Play back control failed [%d]", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                        int nPos = 0;
//                        for (nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(hPlayback))
//                        {
//                            pThis->OutputLog2("Be downloading... %d", nPos);
//                            Sleep(2000);  //millisecond
//                        }
//                        pThis-> OutputLog2("后录像ok... %d", nPos);
//                        if (!NET_DVR_StopGetFile(hPlayback))
//                        {
//                            pThis->OutputLog2("后failed to stop get file [%d]", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                        if (nPos < 0 || nPos>100)
//                        {
//                            pThis->OutputLog2("后download err [%d]\n", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                    }
//                    while (0);
//                }

//                //侧
//                char filenamece[256] = {};
//                sprintf(filenamece, "%s\\%d-%d-%s-侧.mp4", strPath,nPermsg.dataId,  nPermsg.laneno, nPermsg.strcarno);
//                char filenameceS[256] = {};
//                sprintf(filenameceS, "%s\\%d-%d-%s-侧S.mp4", strPath, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
//                pThis->OutputLog2(filenamece);
//                {
//                    do
//                    {
//                        if (pThis->m_lSideNum > 0)  //有侧相机
//                        {
//                            long lChannel = pThis->m_lChanlSide[0];
//                            if (pThis->m_lSideNum > 1)  //侧相机不止1个
//                            {
//                                if (nPermsg.laneno > pThis->m_midlane)  //分方向
//                                {
//                                    lChannel = pThis->m_lChanlSide[1];
//                                }
//                            }
//                            NET_DVR_PLAYCOND struDownloadCond = { 0 };
//                            struDownloadCond.dwChannel = lChannel + pThis->m_lStartChnDVR - 1;
//                            time_t timet = StringToDatetime(nPermsg.strtime) - 2;
//                            tm *tm_ = localtime(&timet);
//                            struDownloadCond.struStartTime.dwYear = tm_->tm_year + 1900;
//                            struDownloadCond.struStartTime.dwMonth = tm_->tm_mon + 1;
//                            struDownloadCond.struStartTime.dwDay = tm_->tm_mday;
//                            struDownloadCond.struStartTime.dwHour = tm_->tm_hour;
//                            struDownloadCond.struStartTime.dwMinute = tm_->tm_min;
//                            struDownloadCond.struStartTime.dwSecond = tm_->tm_sec;
//                            pThis->OutputLog2("开始%d %d %d %d %d %d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
//                                              struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
//                            timet = StringToDatetime(nPermsg.strtime) + 4;
//                            tm_ = localtime(&timet);
//                            struDownloadCond.struStopTime.dwYear = tm_->tm_year + 1900;
//                            struDownloadCond.struStopTime.dwMonth = tm_->tm_mon + 1;
//                            struDownloadCond.struStopTime.dwDay = tm_->tm_mday;
//                            struDownloadCond.struStopTime.dwHour = tm_->tm_hour;
//                            struDownloadCond.struStopTime.dwMinute = tm_->tm_min;
//                            struDownloadCond.struStopTime.dwSecond = tm_->tm_sec;
//                            pThis->OutputLog2("结束%d %d %d %d %d %d", struDownloadCond.struStopTime.dwYear, struDownloadCond.struStopTime.dwMonth, struDownloadCond.struStopTime.dwDay,
//                                              struDownloadCond.struStopTime.dwHour, struDownloadCond.struStopTime.dwMinute, struDownloadCond.struStopTime.dwSecond);
//                            //---------------------------------------
//                            //按时间下载
//                            int hPlayback = NET_DVR_GetFileByTime_V40(pThis->m_lHandleDVR, filenamece, &struDownloadCond);
//                            if (hPlayback < 0)
//                            {
//                                pThis->OutputLog2("侧NET_DVR_GetFileByTime_V40 fail,last error %d", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                            //---------------------------------------
//                            //开始下载
//                            if (!NET_DVR_PlayBackControl_V40(hPlayback, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL))
//                            {
//                                pThis->OutputLog2("侧Play back control failed [%d]", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                            int nPos = 0;
//                            for (nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(hPlayback))
//                            {
//                                pThis->OutputLog2("Be downloading... %d", nPos);
//                                Sleep(2000);  //millisecond
//                            }
//                            pThis->OutputLog2("侧录像ok... %d", nPos);
//                            if (!NET_DVR_StopGetFile(hPlayback))
//                            {
//                                pThis->OutputLog2("侧failed to stop get file [%d]", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                            if (nPos < 0 || nPos>100)
//                            {
//                                pThis->OutputLog2("侧download err [%d]\n", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                        }
//                    }
//                    while (0);
//                }

                {
                    char szTmp[100];
                    GetPrivateProfileString("SYS", "VideoSY",  "0", szTmp, 19, pThis->strConfigfile.c_str());
                    if(atoi(szTmp) == 1)
                    {
                        char szVal[1024] = {};
                        sprintf(szVal,"ffmpeg -i %s -vf \"[in]drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='检测时间：%s':x=10:y=160,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车牌号码：%s':x=10:y=260,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='行车方向：%s':x=10:y=360,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆速度：%d km/h':x=10:y=460,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆轴数：%d 轴':x=10:y=560,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='是否跨道：%s':x=10:y=660,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量：%d kg':x=10:y=760,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='结果：%s':x=10:y=860[out]\" -s 1920x1080 -pix_fmt yuv420p -threads 1 -y %s\n",
                                filenamepre,sss,nPermsg.strcarno,nPermsg.strdirection,nPermsg.speed,
                                nPermsg.axiscount,nPermsg.strcrossroad,nPermsg.weight,nPermsg.strret,filenamepreS);

                        pThis->m_proc->execute(QString::fromLocal8Bit(szVal));
                        Sleep(20);
//                        sprintf(szVal,"ffmpeg -i %s -vf \"[in]drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='检测时间：%s':x=10:y=160,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车牌号码：%s':x=10:y=260,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车型：%s':x=10:y=360,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆速度：%d km/h':x=10:y=460,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆轴数：%d 轴':x=10:y=560,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量：%d kg':x=10:y=660,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量限值：%d kg':x=10:y=760,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='结果：%s':x=10:y=860[out]\" -s 1920x1080 -pix_fmt yuv420p -y %s\n",
//                                filenameafr,sss,nPermsg.strcarno,nPermsg.strcartype,nPermsg.speed,
//                                nPermsg.axiscount,nPermsg.weight,pThis->m_GBXZ,nPermsg.strret,filenameafrS);

//                        pThis->m_proc->execute(QString::fromLocal8Bit(szVal));
//                        Sleep(20);
//                        sprintf(szVal,"ffmpeg -i %s -vf \"[in]drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='检测时间：%s':x=10:y=160,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车牌号码：%s':x=10:y=260,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车型：%s':x=10:y=360,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆速度：%d km/h':x=10:y=460,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆轴数：%d 轴':x=10:y=560,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量：%d kg':x=10:y=660,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量限值：%d kg':x=10:y=760,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='结果：%s':x=10:y=860[out]\" -s 1920x1080 -pix_fmt yuv420p -y %s\n",
//                                filenamece,sss,nPermsg.strcarno,nPermsg.strcartype,nPermsg.speed,
//                                nPermsg.axiscount,nPermsg.weight,pThis->m_GBXZ,nPermsg.strret,filenameceS);

//                        pThis->m_proc->execute(QString::fromLocal8Bit(szVal));
//                        Sleep(20);
                    }

                    int iRet = bDownOk==true?2:3;
                    //入库或上传
                    QString strUpdate = "UPDATE Task SET bUploadvedio=:bUploadvedio,vedioBaseFront=:vedioBaseFront WHERE id=:id;";
                    QSqlQuery qsQuery = QSqlQuery(g_DB);
                    qsQuery.prepare(strUpdate);
                    qsQuery.bindValue(":id", (nPermsg.dataId));
                    qsQuery.bindValue(":bUploadvedio", iRet);
                    qsQuery.bindValue(":vedioBaseFront", QString::fromLocal8Bit(file_exists(filenamepreS)==true?filenamepreS:filenamepre));
//                    qsQuery.bindValue(":vedioBaseBack", QString::fromLocal8Bit(file_exists(filenameafrS)==true?filenameafrS:filenameafr));
//                    qsQuery.bindValue(":vedioBaseSide", QString::fromLocal8Bit(file_exists(filenameceS)==true?filenameceS:filenamece));
                    if (!qsQuery.exec())
                    {
                        QString querys = qsQuery.lastError().text();
                        pThis->OutputLog2("%s bUploadvedio=2 error %s",nPermsg.strcarno,querys.toStdString().c_str());
                    }
                    else
                        pThis->OutputLog2("bUploadvedio更新 %s  %d",nPermsg.strcarno,iRet);

                    if(atoi(szTmp) == 1)
                        DeleteFileA(filenamepre);
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
    char strPath[256] = {};
    GetPrivateProfileString("SYS", "picpath", "D:", strPath, 100, pThis->strConfigfile.c_str());
    char  szVal[200], strSection[20];
    STR_MSG nPermsg;
    while (pThis->bStart)
    {
        Sleep(200);
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
                    nPermsg = pThis->m_MsgVedioQueueHis.front();
                    pThis->m_MsgVedioQueueHis.pop();
                    pThis->m_QueueSectVedioHis.Unlock();
                }
                else
                {
                    pThis->m_QueueSectVedioHis.Unlock();
                    continue;
                }
                //!!!
                bool bDownOk = true;
                char filenamepre[256] = {};
                sprintf(filenamepre, "%s%s%d-%d-%s-前.mp4", strPath, PLATPATH, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
                char filenamepreS[256] = {};
                sprintf(filenamepreS, "%s%s%d-%d-%s-前S.mp4", strPath, PLATPATH, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
                pThis->OutputLog2(filenamepre);
                pThis->OutputLog2(nPermsg.strtime);
                char sss[256] = {};
                sprintf(strSection, "车道%d", nPermsg.laneno);
                GetPrivateProfileString(strSection, "前相机", "", szVal, 19, pThis->strConfigfile.c_str());
                int iPreID = atoi(szVal);
                auto ifind = pThis->m_maplaneAndDevice.find(iPreID);
                if (ifind != pThis->m_maplaneAndDevice.end())
                {
                    //前
                    do
                    {
                        NET_DVR_PLAYCOND struDownloadCond = { 0 };
                        struDownloadCond.dwChannel = ifind->second->lChanlNumPre + pThis->m_lStartChnDVR - 1;
                        time_t timet = StringToDatetime(nPermsg.strtime) - 3;
                        tm *tm_ = localtime(&timet);
                        struDownloadCond.struStartTime.dwYear = tm_->tm_year + 1900;
                        struDownloadCond.struStartTime.dwMonth = tm_->tm_mon + 1;
                        struDownloadCond.struStartTime.dwDay = tm_->tm_mday;
                        struDownloadCond.struStartTime.dwHour = tm_->tm_hour;
                        struDownloadCond.struStartTime.dwMinute = tm_->tm_min;
                        struDownloadCond.struStartTime.dwSecond = tm_->tm_sec;
                        pThis->OutputLog2("开始%d %d %d %d %d %d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
                                          struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
                        sprintf_s(sss, "%04d-%02d-%02d %02d-%02d-%02d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
                                  struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
                        timet = StringToDatetime(nPermsg.strtime) + 5;
                        tm_ = localtime(&timet);
                        struDownloadCond.struStopTime.dwYear = tm_->tm_year + 1900;
                        struDownloadCond.struStopTime.dwMonth = tm_->tm_mon + 1;
                        struDownloadCond.struStopTime.dwDay = tm_->tm_mday;
                        struDownloadCond.struStopTime.dwHour = tm_->tm_hour;
                        struDownloadCond.struStopTime.dwMinute = tm_->tm_min;
                        struDownloadCond.struStopTime.dwSecond = tm_->tm_sec;
                        pThis->OutputLog2("结束%d %d %d %d %d %d", struDownloadCond.struStopTime.dwYear, struDownloadCond.struStopTime.dwMonth, struDownloadCond.struStopTime.dwDay,
                                          struDownloadCond.struStopTime.dwHour, struDownloadCond.struStopTime.dwMinute, struDownloadCond.struStopTime.dwSecond);
                        //---------------------------------------
                        //按时间下载
                        int hPlayback = NET_DVR_GetFileByTime_V40(pThis->m_lHandleDVR, filenamepre, &struDownloadCond);
                        if (hPlayback < 0)
                        {
                            pThis->OutputLog2("前NET_DVR_GetFileByTime_V40 fail,last error %d", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                        //---------------------------------------
                        //开始下载
                        if (!NET_DVR_PlayBackControl_V40(hPlayback, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL))
                        {
                            pThis->OutputLog2("前Play back control failed [%d]", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                        int nPos = 0;
                        for (nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(hPlayback))
                        {
                            pThis->OutputLog2("Be downloading... %d", nPos);
                            Sleep(2000);  //millisecond
                        }
                        pThis->OutputLog2("前录像ok... %d", nPos);
                        if (!NET_DVR_StopGetFile(hPlayback))
                        {
                            pThis->OutputLog2("前failed to stop get file [%d]", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                        if (nPos < 0 || nPos>100)
                        {
                            pThis->OutputLog2("前download err [%d]\n", NET_DVR_GetLastError());
                            bDownOk=false;
                            break;
                        }
                    }
                    while (0);
                }

//                char filenameafr[256] = {};
//                sprintf(filenameafr, "%s\\%d-%d-%s-后.mp4", strPath, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
//                char filenameafrS[256] = {};
//                sprintf(filenameafrS, "%s\\%d-%d-%s-后S.mp4", strPath, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
//                pThis->OutputLog2(filenameafr);
//                GetPrivateProfileString(strSection, "后相机", "", szVal, 19, pThis->strConfigfile.c_str());
//                int iAfrID = atoi(szVal);
//                auto ifind2 = pThis->m_maplaneAndDevice.find(iAfrID);
//                if (ifind2 != pThis->m_maplaneAndDevice.end())
//                {
//                    //后
//                    do
//                    {
//                        NET_DVR_PLAYCOND struDownloadCond = { 0 };
//                        struDownloadCond.dwChannel = ifind2->second->lChanlNumAfr + pThis->m_lStartChnDVR - 1;
//                        time_t timet = StringToDatetime(nPermsg.strtime) - 2;
//                        tm *tm_ = localtime(&timet);
//                        struDownloadCond.struStartTime.dwYear = tm_->tm_year + 1900;
//                        struDownloadCond.struStartTime.dwMonth = tm_->tm_mon + 1;
//                        struDownloadCond.struStartTime.dwDay = tm_->tm_mday;
//                        struDownloadCond.struStartTime.dwHour = tm_->tm_hour;
//                        struDownloadCond.struStartTime.dwMinute = tm_->tm_min;
//                        struDownloadCond.struStartTime.dwSecond = tm_->tm_sec;
//                        pThis->OutputLog2("开始%d %d %d %d %d %d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
//                                          struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
//                        timet = StringToDatetime(nPermsg.strtime) + 4;
//                        tm_ = localtime(&timet);
//                        struDownloadCond.struStopTime.dwYear = tm_->tm_year + 1900;
//                        struDownloadCond.struStopTime.dwMonth = tm_->tm_mon + 1;
//                        struDownloadCond.struStopTime.dwDay = tm_->tm_mday;
//                        struDownloadCond.struStopTime.dwHour = tm_->tm_hour;
//                        struDownloadCond.struStopTime.dwMinute = tm_->tm_min;
//                        struDownloadCond.struStopTime.dwSecond = tm_->tm_sec;
//                        pThis->OutputLog2("结束%d %d %d %d %d %d", struDownloadCond.struStopTime.dwYear, struDownloadCond.struStopTime.dwMonth, struDownloadCond.struStopTime.dwDay,
//                                          struDownloadCond.struStopTime.dwHour, struDownloadCond.struStopTime.dwMinute, struDownloadCond.struStopTime.dwSecond);
//                        //---------------------------------------
//                        //按时间下载
//                        int hPlayback = NET_DVR_GetFileByTime_V40(pThis->m_lHandleDVR, filenameafr, &struDownloadCond);
//                        if (hPlayback < 0)
//                        {
//                            pThis->OutputLog2("后NET_DVR_GetFileByTime_V40 fail,last error %d", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                        //---------------------------------------
//                        //开始下载
//                        if (!NET_DVR_PlayBackControl_V40(hPlayback, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL))
//                        {
//                            pThis->OutputLog2("后Play back control failed [%d]", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                        int nPos = 0;
//                        for (nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(hPlayback))
//                        {
//                            pThis->OutputLog2("Be downloading... %d", nPos);
//                            Sleep(2000);  //millisecond
//                        }
//                        pThis-> OutputLog2("后录像ok... %d", nPos);
//                        if (!NET_DVR_StopGetFile(hPlayback))
//                        {
//                            pThis->OutputLog2("后failed to stop get file [%d]", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                        if (nPos < 0 || nPos>100)
//                        {
//                            pThis->OutputLog2("后download err [%d]\n", NET_DVR_GetLastError());
//                            bDownOk=false;
//                            break;
//                        }
//                    }
//                    while (0);
//                }

//                //侧
//                char filenamece[256] = {};
//                sprintf(filenamece, "%s\\%d-%d-%s-侧.mp4", strPath,nPermsg.dataId,  nPermsg.laneno, nPermsg.strcarno);
//                char filenameceS[256] = {};
//                sprintf(filenameceS, "%s\\%d-%d-%s-侧S.mp4", strPath, nPermsg.dataId, nPermsg.laneno, nPermsg.strcarno);
//                pThis->OutputLog2(filenamece);
//                {
//                    do
//                    {
//                        if (pThis->m_lSideNum > 0)  //有侧相机
//                        {
//                            long lChannel = pThis->m_lChanlSide[0];
//                            if (pThis->m_lSideNum > 1)  //侧相机不止1个
//                            {
//                                if (nPermsg.laneno > pThis->m_midlane)  //分方向
//                                {
//                                    lChannel = pThis->m_lChanlSide[1];
//                                }
//                            }
//                            NET_DVR_PLAYCOND struDownloadCond = { 0 };
//                            struDownloadCond.dwChannel = lChannel + pThis->m_lStartChnDVR - 1;
//                            time_t timet = StringToDatetime(nPermsg.strtime) - 2;
//                            tm *tm_ = localtime(&timet);
//                            struDownloadCond.struStartTime.dwYear = tm_->tm_year + 1900;
//                            struDownloadCond.struStartTime.dwMonth = tm_->tm_mon + 1;
//                            struDownloadCond.struStartTime.dwDay = tm_->tm_mday;
//                            struDownloadCond.struStartTime.dwHour = tm_->tm_hour;
//                            struDownloadCond.struStartTime.dwMinute = tm_->tm_min;
//                            struDownloadCond.struStartTime.dwSecond = tm_->tm_sec;
//                            pThis->OutputLog2("开始%d %d %d %d %d %d", struDownloadCond.struStartTime.dwYear, struDownloadCond.struStartTime.dwMonth, struDownloadCond.struStartTime.dwDay,
//                                              struDownloadCond.struStartTime.dwHour, struDownloadCond.struStartTime.dwMinute, struDownloadCond.struStartTime.dwSecond);
//                            timet = StringToDatetime(nPermsg.strtime) + 4;
//                            tm_ = localtime(&timet);
//                            struDownloadCond.struStopTime.dwYear = tm_->tm_year + 1900;
//                            struDownloadCond.struStopTime.dwMonth = tm_->tm_mon + 1;
//                            struDownloadCond.struStopTime.dwDay = tm_->tm_mday;
//                            struDownloadCond.struStopTime.dwHour = tm_->tm_hour;
//                            struDownloadCond.struStopTime.dwMinute = tm_->tm_min;
//                            struDownloadCond.struStopTime.dwSecond = tm_->tm_sec;
//                            pThis->OutputLog2("结束%d %d %d %d %d %d", struDownloadCond.struStopTime.dwYear, struDownloadCond.struStopTime.dwMonth, struDownloadCond.struStopTime.dwDay,
//                                              struDownloadCond.struStopTime.dwHour, struDownloadCond.struStopTime.dwMinute, struDownloadCond.struStopTime.dwSecond);
//                            //---------------------------------------
//                            //按时间下载
//                            int hPlayback = NET_DVR_GetFileByTime_V40(pThis->m_lHandleDVR, filenamece, &struDownloadCond);
//                            if (hPlayback < 0)
//                            {
//                                pThis->OutputLog2("侧NET_DVR_GetFileByTime_V40 fail,last error %d", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                            //---------------------------------------
//                            //开始下载
//                            if (!NET_DVR_PlayBackControl_V40(hPlayback, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL))
//                            {
//                                pThis->OutputLog2("侧Play back control failed [%d]", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                            int nPos = 0;
//                            for (nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(hPlayback))
//                            {
//                                pThis->OutputLog2("Be downloading... %d", nPos);
//                                Sleep(2000);  //millisecond
//                            }
//                            pThis->OutputLog2("侧录像ok... %d", nPos);
//                            if (!NET_DVR_StopGetFile(hPlayback))
//                            {
//                                pThis->OutputLog2("侧failed to stop get file [%d]", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                            if (nPos < 0 || nPos>100)
//                            {
//                                pThis->OutputLog2("侧download err [%d]\n", NET_DVR_GetLastError());
//                                bDownOk=false;
//                                break;
//                            }
//                        }
//                    }
//                    while (0);
//                }

                {
                    char szTmp[100];
                    GetPrivateProfileString("SYS", "VideoSY",  "0", szTmp, 19, pThis->strConfigfile.c_str());
                    if(atoi(szTmp) == 1)
                    {
                        char szVal[1024] = {};
                        sprintf(szVal,"ffmpeg -i %s -vf \"[in]drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='检测时间：%s':x=10:y=160,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车牌号码：%s':x=10:y=260,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='行车方向：%s':x=10:y=360,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆速度：%d km/h':x=10:y=460,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆轴数：%d 轴':x=10:y=560,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='是否跨道：%s':x=10:y=660,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量：%d kg':x=10:y=760,\
                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='结果：%s':x=10:y=860[out]\" -s 1920x1080 -pix_fmt yuv420p -threads 1 -y %s\n",
                                filenamepre,sss,nPermsg.strcarno,nPermsg.strdirection,nPermsg.speed,
                                nPermsg.axiscount,nPermsg.strcrossroad,nPermsg.weight,nPermsg.strret,filenamepreS);

                        pThis->m_proc->execute(QString::fromLocal8Bit(szVal));
                        Sleep(20);
//                        sprintf(szVal,"ffmpeg -i %s -vf \"[in]drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='检测时间：%s':x=10:y=160,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车牌号码：%s':x=10:y=260,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车型：%s':x=10:y=360,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆速度：%d km/h':x=10:y=460,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆轴数：%d 轴':x=10:y=560,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量：%d kg':x=10:y=660,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量限值：%d kg':x=10:y=760,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='结果：%s':x=10:y=860[out]\" -s 1920x1080 -pix_fmt yuv420p -y %s\n",
//                                filenameafr,sss,nPermsg.strcarno,nPermsg.strcartype,nPermsg.speed,
//                                nPermsg.axiscount,nPermsg.weight,pThis->m_GBXZ,nPermsg.strret,filenameafrS);

//                        pThis->m_proc->execute(QString::fromLocal8Bit(szVal));
//                        Sleep(20);
//                        sprintf(szVal,"ffmpeg -i %s -vf \"[in]drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='检测时间：%s':x=10:y=160,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车牌号码：%s':x=10:y=260,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车型：%s':x=10:y=360,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆速度：%d km/h':x=10:y=460,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='车辆轴数：%d 轴':x=10:y=560,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量：%d kg':x=10:y=660,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='总质量限值：%d kg':x=10:y=760,\
//                                drawtext=fontsize=80:fontfile=msyh.ttc:fontcolor=white:text='结果：%s':x=10:y=860[out]\" -s 1920x1080 -pix_fmt yuv420p -y %s\n",
//                                filenamece,sss,nPermsg.strcarno,nPermsg.strcartype,nPermsg.speed,
//                                nPermsg.axiscount,nPermsg.weight,pThis->m_GBXZ,nPermsg.strret,filenameceS);

//                        pThis->m_proc->execute(QString::fromLocal8Bit(szVal));
//                        Sleep(20);
                    }

                    int iRet = bDownOk==true?2:3;
                    //入库或上传
                    QString strUpdate = "UPDATE Task SET bUploadvedio=:bUploadvedio,vedioBaseFront=:vedioBaseFront WHERE id=:id;";
                    QSqlQuery qsQuery = QSqlQuery(g_DB);
                    qsQuery.prepare(strUpdate);
                    qsQuery.bindValue(":id", (nPermsg.dataId));
                    qsQuery.bindValue(":bUploadvedio", iRet);
                    qsQuery.bindValue(":vedioBaseFront", QString::fromLocal8Bit(file_exists(filenamepreS)==true?filenamepreS:filenamepre));
//                    qsQuery.bindValue(":vedioBaseBack", QString::fromLocal8Bit(file_exists(filenameafrS)==true?filenameafrS:filenameafr));
//                    qsQuery.bindValue(":vedioBaseSide", QString::fromLocal8Bit(file_exists(filenameceS)==true?filenameceS:filenamece));
                    if (!qsQuery.exec())
                    {
                        QString querys = qsQuery.lastError().text();
                        pThis->OutputLog2("%s bUploadvedio=2 error %s",nPermsg.strcarno,querys.toStdString().c_str());
                    }
                    else
                        pThis->OutputLog2("bUploadvedio更新 %s  %d",nPermsg.strcarno,iRet);

                    if(atoi(szTmp) == 1)
                        DeleteFileA(filenamepre);
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

void Split(vector<string>& strVecResult, const string& strResource, const string& strSplitOptions)
{
    if (0 == strResource.length() || 0 == strSplitOptions.length())
        return;
    string::size_type pos = 0, lastPos = 0;
    do
    {
        lastPos = pos;
        pos = strResource.find(strSplitOptions, pos);
        if (string::npos != pos)
        {
            strVecResult.push_back(strResource.substr(lastPos, pos - lastPos));
            pos += strSplitOptions.length();
        }
    } while (string::npos != pos && pos < strResource.length());
    //最后一个元素
    strVecResult.push_back(strResource.substr(lastPos));
}

void OverWidget::OutputLog2(const char *msg, ...)
{
    char final[65535] = { 0 };   //当前时间记录
    va_list vl_list;
    va_start(vl_list, msg);
    char content[65535] = { 0 };
    vsprintf(content, msg, vl_list);   //格式化处理msg到字符串
    va_end(vl_list);
    OutputLog(content);

    QDateTime current_date_time = QDateTime::currentDateTime();
    QString strTemp = current_date_time.toString("yyyy-MM-dd hh:mm:ss");
    sprintf_s(final, "%s <> %s", strTemp.toStdString().c_str(),content);
    qDebug() << QString::fromLocal8Bit(final);
    emit FlushSig(QString::fromLocal8Bit(final));
}
