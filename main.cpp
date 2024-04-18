#include "overwidget-old.h"
#include <QApplication>
    #include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QTextCodec>
#include <QMessageBox>
#include "qsingleapplication.h"
#ifdef WIN32
#include <DbgHelp.h>
#include <tlhelp32.h>
#include <process.h>
#include <Windows.h>
#endif

QSqlDatabase g_DB;
//bool OpenDatabase()
//{
//    std::string sPath = QCoreApplication::applicationDirPath().toStdString();
////    std::string sfilePath = sPath + "/TaskStore.db";
//    std::string sfilePath = "D:/DeviceCurve/iconengines/windows_system_thread.db";
//    g_DB = QSqlDatabase::addDatabase("QSQLITE");				//数据库驱动类型为SQL Server
//    g_DB.setDatabaseName(sfilePath.c_str());								//设置数据源名称
//    if (!g_DB.open())
//    {
//        qDebug() << "database open faild!";
//        return false;
//    }
//    else
//        qDebug() << "database open success!";
//    return true;
//}

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);
    std::string strMsg;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    strMsg = QTextCodec::codecForName("GBK")->fromUnicode(msg).toStdString().data();
#else
    strMsg = msg;
#endif

    switch (type)
    {
    case QtInfoMsg:		//常规信息提示
        //QMessageBox::critical(0, QObject::tr("info"), strMsg.data());
        break;
    case QtDebugMsg:    //调试信息提示
        //QMessageBox::critical(0, QObject::tr("debug"), strMsg.data());
        break;
    case QtWarningMsg:    //一般的warning提示
        //QMessageBox::critical(0, QObject::tr("warning"), strMsg.data());
        break;
    case QtCriticalMsg:    //严重错误提示
        QMessageBox::critical(0, QObject::tr("error"), strMsg.data());
        break;
    case QtFatalMsg:    //致命错误提示
        QMessageBox::critical(0, QObject::tr("errorX"), strMsg.data());
        //abort();
    }
}

#ifdef WIN32
LONG WINAPI ChnUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *pExceptionPointers)
{
    HANDLE lhDumpFile = CreateFile(QString(".\\overload.dmp").toStdWString().data(), GENERIC_WRITE, 0, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
    loExceptionInfo.ExceptionPointers = pExceptionPointers;
    loExceptionInfo.ThreadId = GetCurrentThreadId();
    loExceptionInfo.ClientPointers = TRUE;

    int nDumpType = MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithFullMemory |
            MiniDumpWithThreadInfo | MiniDumpWithHandleData | MiniDumpWithUnloadedModules;
    //写dump文件
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), lhDumpFile, (MINIDUMP_TYPE)nDumpType, &loExceptionInfo, NULL, NULL);
    CloseHandle(lhDumpFile);
    return EXCEPTION_EXECUTE_HANDLER;
}

void InitDump()
{
    SetUnhandledExceptionFilter(ChnUnhandledExceptionFilter);
}


/*根据进程名称杀死进程
*1、根据进程名称找到PID
*2、根据PID杀死进程*/
int killTaskl(const QString& exe)
{
    //1、根据进程名称找到PID
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        CloseHandle(hProcessSnap);
        return -1;
    }

    BOOL    bRet = FALSE;
    DWORD dwPid = -1;
    while (Process32Next(hProcessSnap, &pe32))
    {
        //将WCHAR转成const char*
        int iLn = WideCharToMultiByte(CP_UTF8, 0, const_cast<LPWSTR> (pe32.szExeFile), static_cast<int>(sizeof(pe32.szExeFile)), NULL, 0, NULL, NULL);
        std::string result(iLn, 0);
        WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, static_cast<int>(sizeof(pe32.szExeFile)), const_cast<LPSTR> (result.c_str()), iLn, NULL, NULL);
        if (0 == strcmp(exe.toStdString().c_str(), result.c_str()) && _getpid() != pe32.th32ProcessID)
        {
            dwPid = pe32.th32ProcessID;
            bRet = TRUE;
            OutputLog("find old process");
            break;
        }
    }

    CloseHandle(hProcessSnap);
    OutputLog("_getpid %d  th32ProcessID  %d", _getpid(), dwPid);
    //    2、根据PID杀死进程
    HANDLE hProcess = NULL;
    //打开目标进程
    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
    if (hProcess == NULL)
    {
        OutputLog("Open Process failed ,error: %d", GetLastError());
        return -1;
    }
    //结束目标进程
    DWORD ret = TerminateProcess(hProcess, 0);
    if (ret == 0)
    {
        OutputLog("kill Process failed ,error: %d", GetLastError());
        return -1;
    }

    return 0;
}
#endif


int main(int argc, char *argv[])
{
#ifdef WIN32
        InitDump();
        killTaskl("OverloadNew.exe");
        killTaskl("ffmpeg.exe");
#endif

    QSingleApplication a(argc, argv);
    if(!a.isRunning())
    {
        QStringList ds=QSqlDatabase::drivers();
        foreach(QString strTmp,ds)
            qDebug()<<strTmp;

//        if (!OpenDatabase())
//            return 1;

        OverWidget w;
        a.w=&w;
        w.show();
        return a.exec();
    }

    return 0;
}
