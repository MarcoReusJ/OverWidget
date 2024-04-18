#include "resultMode.h"
#include <QColor>

ResultTableModel::ResultTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

ResultTableModel::~ResultTableModel()
{

}

// 更新表格数据
void ResultTableModel::updateData(ResultRecord recod)
{
    m_recordList.push_back(recod);
    beginResetModel();  //开始初始化Model，接着QTableView将重新
    endResetModel();  //显示容器中的信息
}

// 行数
int ResultTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_recordList.count();
}

// 列数
int ResultTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 17;
}

// 设置表格项数据 用来修改和特殊的模型索引相关的项目。修改的数据必须是Qt::EditRole，发送一个dataChanged信号。
bool ResultTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

	int nColumn = index.column();
	ResultRecord record = m_recordList.at(index.row());
	switch (role)
	{
	case Qt::DisplayRole:
	{
        if (nColumn == RESULT_FID_COLUMN)
		{
			record.qstrfid = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_TIME_COLUMN)
		{
            record.qstrtime = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_CARNO_COLUMN)
		{
            record.qstrcarno = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_COLOR_COLUMN)
		{
            record.qstrcolor = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_SPEED_COLUMN)
		{
            record.qstrspeed = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_LANENO_COLUMN)
		{
            record.qstrlane = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_AXIS_COLUMN)
		{
            record.qstraxis = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_DIRECTION_COLUMN)
		{
            record.qstrcrossroad/*qstrdirection*/ = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_WEIGHT_COLUMN)
		{
            record.qstrweight = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_WEIGHT1_COLUMN)
		{
            record.qstrweight1 = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_WEIGHT2_COLUMN)
		{
            record.qstrweight2 = value.toString();
			m_recordList.replace(index.row(), record);
			emit dataChanged(index, index);
			return true;
		}
        if (nColumn == RESULT_WEIGHT3_COLUMN)
        {
            record.qstrweight3 = value.toString();
            m_recordList.replace(index.row(), record);
            emit dataChanged(index, index);
            return true;
        }
        if (nColumn == RESULT_WEIGHT4_COLUMN)
        {
            record.qstrweight4 = value.toString();
            m_recordList.replace(index.row(), record);
            emit dataChanged(index, index);
            return true;
        }
        if (nColumn == RESULT_WEIGHT5_COLUMN)
        {
            record.qstrweight5 = value.toString();
            m_recordList.replace(index.row(), record);
            emit dataChanged(index, index);
            return true;
        }
        if (nColumn == RESULT_WEIGHT6_COLUMN)
        {
            record.qstrweight6 = value.toString();
            m_recordList.replace(index.row(), record);
            emit dataChanged(index, index);
            return true;
        }
        if (nColumn == RESULT_WEIGHT7_COLUMN)
        {
            record.qstrweight7 = value.toString();
            m_recordList.replace(index.row(), record);
            emit dataChanged(index, index);
            return true;
        }
        if (nColumn == RESULT_WEIGHT8_COLUMN)
        {
            record.qstrweight8 = value.toString();
            m_recordList.replace(index.row(), record);
            emit dataChanged(index, index);
            return true;
        }
	}
	break;
	default:
		return false;
	}

	return false;
}

// 表格项数据
QVariant ResultTableModel::data(const QModelIndex &index, int role) const
{
    //index: model索引，这个索引指明这个数据位于model的位置，比如行、列等
    if (!index.isValid())
        return QVariant();

    int nRow = index.row();
    int nColumn = index.column();
	ResultRecord record = m_recordList.at(nRow);

    switch (role)
    {
    case Qt::TextColorRole: //文本颜色
    {
//        if (nColumn == RESULT_RET_COLUMN && record.qstrret == QStringLiteral("不合格"))
//            return QColor(255, 0, 0);
//        if (nColumn == RESULT_RET_COLUMN && record.qstrret == QStringLiteral("合格"))
//            return QColor(0, 255, 0);
    }
        break;
    case Qt::TextAlignmentRole:
        return QVariant(Qt::AlignCenter);
        break;
	case Qt::DisplayRole:
	{
		if (nColumn == RESULT_FID_COLUMN) //mazq
			return record.qstrfid;
        if (nColumn == RESULT_TIME_COLUMN) //mazq
            return record.qstrtime;
        if (nColumn == RESULT_CARNO_COLUMN) //mazq
            return record.qstrcarno;
        if (nColumn == RESULT_COLOR_COLUMN) //mazq
            return record.qstrcolor;
        if (nColumn == RESULT_SPEED_COLUMN) //mazq
            return record.qstrspeed;
        if (nColumn == RESULT_LANENO_COLUMN) //mazq
            return record.qstrlane;
        if (nColumn == RESULT_AXIS_COLUMN) //mazq
            return record.qstraxis;
        if (nColumn == RESULT_DIRECTION_COLUMN) //mazq
            return record.qstrcrossroad/*qstrdirection*/;
        if (nColumn == RESULT_WEIGHT_COLUMN) //mazq
            return record.qstrweight;
        if (nColumn == RESULT_WEIGHT1_COLUMN) //mazq
            return record.qstrweight1;
        if (nColumn == RESULT_WEIGHT2_COLUMN) //mazq
            return record.qstrweight2;
        if (nColumn == RESULT_WEIGHT3_COLUMN) //mazq
            return record.qstrweight3;
        if (nColumn == RESULT_WEIGHT4_COLUMN) //mazq
            return record.qstrweight4;
        if (nColumn == RESULT_WEIGHT5_COLUMN) //mazq
            return record.qstrweight5;
        if (nColumn == RESULT_WEIGHT6_COLUMN) //mazq
            return record.qstrweight6;
        if (nColumn == RESULT_WEIGHT7_COLUMN) //mazq
            return record.qstrweight7;
        if (nColumn == RESULT_WEIGHT8_COLUMN) //mazq
            return record.qstrweight8;
		return "";
	}
	break;
	default:
		return QVariant();
	}

	return QVariant();
}

// 表头数据
QVariant ResultTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    //role: 每个数据元素有一组属性值，称为角色(roles),这个属性值并不是数据的内容，而是它的属性，比如说这个数据是用来展示数据的，还是用于显示列头的
    switch (role)
    {
    case Qt::TextAlignmentRole:
        return QVariant(Qt::AlignCenter);
        break;
    case Qt::DisplayRole: //用于视图的文本显示
    {
        if (orientation == Qt::Horizontal) //水平方向，若不写此行，则水平和竖直方向都有头
        {
			if (section == 0)
                return QStringLiteral("ID");
			if (section == 1)
                return QStringLiteral("测量时间");
			if (section == 2)
                return QStringLiteral("车牌号码");
			if (section == 3)
                return QStringLiteral("车牌颜色");
			if (section == 4)
                return QStringLiteral("车速");
			if (section == 5)
                return QStringLiteral("车道");
			if (section == 6)
                return QStringLiteral("轴数");
			if (section == 7)
                return QStringLiteral("行车方向");
			if (section == 8)
                return QStringLiteral("总重量");
			if (section == 9)
                return QStringLiteral("轴1重");
			if (section == 10)
                return QStringLiteral("轴2重");
            if (section == 11)
                return QStringLiteral("轴3重");
            if (section == 12)
                return QStringLiteral("轴4重");
            if (section == 13)
                return QStringLiteral("轴5重");
            if (section == 14)
                return QStringLiteral("轴6重");
            if (section == 15)
                return QStringLiteral("轴7重");
            if (section == 16)
                return QStringLiteral("轴8重");
        }
    }
        break;
    default:
        return QVariant();
    }

    return QVariant();
}

