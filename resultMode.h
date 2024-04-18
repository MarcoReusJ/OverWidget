#ifndef RESULTMODE_H
#define RESULTMODE_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QList>
#include <QVariant>

enum  ResultColumnEnum
{
	RESULT_FID_COLUMN,
    RESULT_TIME_COLUMN,
    RESULT_CARNO_COLUMN,
    RESULT_COLOR_COLUMN,
    RESULT_SPEED_COLUMN,
    RESULT_LANENO_COLUMN,
    RESULT_AXIS_COLUMN,
    RESULT_DIRECTION_COLUMN,
    RESULT_WEIGHT_COLUMN,
    RESULT_WEIGHT1_COLUMN,
    RESULT_WEIGHT2_COLUMN,
    RESULT_WEIGHT3_COLUMN,
    RESULT_WEIGHT4_COLUMN,
    RESULT_WEIGHT5_COLUMN,
    RESULT_WEIGHT6_COLUMN,
    RESULT_WEIGHT7_COLUMN,
    RESULT_WEIGHT8_COLUMN
};

struct ResultRecord
{
    QString qstrpicpre;
    QString qstrpicpre1;
    QString qstrpichce;
    QString qstrpicce;
    QString qstrpicce2;
    QString qstrpicafr;
    QString qstrpicsmall;
    QString qstrpicvideo;
    QString qstrs1;
    QString qstrs2;
    QString qstrs3;
    QString qstrs4;
    QString qstrs5;
    QString qstrs6;
    QString qstrs7;
    QString qstrdirection;
    QString qstrcrossroad;

    QString qstrfid;
    QString qstrtime;
	QString qstrcarno;
    QString qstrcolor;
    QString qstrspeed;
    QString qstrweight;
    QString qstrlane;
    QString qstraxis;
    QString qstrcartype;
    QString qstrweight1;
    QString qstrweight2;
    QString qstrweight3;
    QString qstrweight4;
    QString qstrweight5;
    QString qstrweight6;
    QString qstrweight7;
    QString qstrweight8;

    QString qstrw;
    QString qstrh;
    QString qstrl;

    QString qstrVideoState;
    QString qstrUploadState;
    QString qstrXz;
};

class ResultTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
	ResultTableModel(QObject *parent = 0);
    ~ResultTableModel();
public:
    /*
    updateData  主要用于更新数据，刷新界面。
    data 用来显示数据，根据角色（颜色、文本、对齐方式、选中状态等）判断需要显示的内容。
    setData 用来设置数据，根据角色（颜色、文本、对齐方式、选中状态等）判断需要设置的内容。
    headerData 用来显示水平/垂直表头的数据。
    flags 用来设置单元格的标志（可用、可选中、可复选等）。
    */
    void updateData(ResultRecord recod);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QList<ResultRecord> getRecordList()
    {
        return m_recordList;
    }
	void clearRecordList()
	{
		m_recordList.clear();
	}

private:
    QList<ResultRecord> m_recordList;
};


#endif // MYMODE_H
