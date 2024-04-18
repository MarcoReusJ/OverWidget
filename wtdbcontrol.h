#ifndef WTDBCONTROL_H
#define WTDBCONTROL_H

#include "common_utils.h"
//#include <QSqlDatabase>
//#include <QtSql>
//#include <QJsonDocument>
//#include <QJsonObject>

class CWTDBControl {
private:
    QSqlDatabase db;
    int m_type;

public:
    CWTDBControl(int type,QString host,QString DBname,QString user,QString pass) {
        m_type = type;
        if (type == 1) {
            db = QSqlDatabase::addDatabase("QSQLITE");
            db.setDatabaseName(DBname);
        } else if (type == 2) {
            db = QSqlDatabase::addDatabase("QMYSQL");
            db.setHostName(host);
            db.setDatabaseName(DBname);
            db.setUserName(user);
            db.setPassword(pass);
        } else if (type == 3) {
            db = QSqlDatabase::addDatabase("QODBC");
            QString dbname = QString("DRIVER={SQL Server};SERVER=%1;DATABASE=%2;UID=%3;PWD=%4").arg(host).arg(DBname).arg(user).arg(pass);
            db.setDatabaseName(dbname);
        }
        if (!db.open()) {
            OutputLog("----dataBase: openDB false name:%s",DBname.toLocal8Bit().toStdString().c_str());
            return;
        }
    }
    QSqlDatabase& getDatabase(){
        return db;
    }

    bool insertData(QString table,const QJsonObject& jsonData) {
        // 实现插入数据操作
        QString columns;
        QString values;

        // 解析colne中的字段名
        for (const auto& key : jsonData.keys()) {
            columns += key + ",";
            values += "'" + jsonData[key].toString() + "',";
        }
        columns.chop(1); // 去除最后一个逗号
        values.chop(1); // 去除最后一个逗号

        // 拼接SQL插入语句
        QString sql = "INSERT INTO " + table + " (" + columns + ") VALUES (" + values + ");";

        // 执行SQL语句，例如使用Qt的QSqlQuery类执行该语句
        QSqlQuery query=QSqlQuery(db);
        bool bsuccess = query.exec(sql);
        OutputLog("----dataBase: insertData %d",bsuccess);
        QString querys;
        if (!bsuccess){
            querys = query.lastError().text();
            return false;
        }
        else{
            return true;
        }
    }

    bool isBlack(QString strcar)
    {
        QSqlQuery query(db);
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
                query.finish();
                return 1;
            }
            else
            {
                query.finish();
                return 0;
            }
        }
        query.finish();
        return 0;
    }

    bool queryNoVedio(QVector<STR_MSG>& msglist)
    {
        QSqlQuery query(db);
        QString sql = "SELECT * FROM Task WHERE bUploadvedio = 0 order by id desc;";
        query.prepare(sql);
        if (!query.exec())
        {
            return false;
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
                    msglist.append(nPmsg);
                }
            }
        }
//        OutputLog("----dataBase: queryNoVedio %d",bsuccess);
        return true;
    }
    QJsonArray query(QJsonArray& jsonArry,QString sql)
    {
        QJsonArray nameArray;

        QSqlQuery msgQuery(db);
        msgQuery.prepare(sql);
        QJsonObject jsonData;
        if (!msgQuery.exec())
        {
            QString error = msgQuery.lastError().text();
        }
        else
        {
            int iCount = 0;
            if (msgQuery.last())
            {
                iCount = msgQuery.at() + 1;
                msgQuery.first();
                msgQuery.previous();
                if(iCount > 0)
                {
                    while (msgQuery.next())
                    {
                        for (int i = 0; i < jsonArry.size(); i++) {
                            QString name = jsonArry.at(i).toString();
                            nameArray.append(QString(msgQuery.value(name).toString().toUtf8()));
                        }
                    }
                }
            }
        }
        return nameArray;

    }

    int findMaxID() {
        // 实现查找数据操作
        int nFID = -1;
        if(!db.isOpen())
            return -2;
        QString strSelect = "Select max(id) from Task;";
        QSqlQuery query(db);
        QString querys;
        query.prepare(strSelect);
        if (!query.exec())
        {
            querys = query.lastError().text();
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
                    nFID = query.value(0).toInt();
                }
            }
        }
        return nFID;
    }

    void updateData(const QJsonObject& jsonData) {
        Q_UNUSED(jsonData);
        // 实现更新数据操作
    }
};
#endif // WTDBCONTROL_H
