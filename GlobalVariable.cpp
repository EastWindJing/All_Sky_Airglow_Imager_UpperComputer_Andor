#include "GlobalVariable.h"
#include "pathutils.h"

//在此文件中对全局变量进行定义。
//不能够将全局变量定义在头文件中，否则在会因为头文件被多次引用而造成变量多次定义，进而导致链接失败。
//在其他文件中使用全局变量的时候，要用extern关键字声明变量

UsedTimeType g_UsedTimeType = UTC_Time;  //Local_Time  、 UTC_Time
QMutex g_Mutex_UsedTimeType;    //g_UsedTimeType的互斥量

bool g_isWorkingUnderAOC = false;  //是否在AOC的控制下工作
QMutex g_Mutex_isWorkingUnderAOC;  //g_isWorkingUnderAOC的互斥量

QDate g_CurWorkingPeriodStartDate = currentDateTime_UorL(get_UsedTimeType()).date();  //当前所在工作时段的开始日期（不管观测时段是否跨天，数据文件一律保存在开始那天的文件夹下）
QMutex g_Mutex_CurWorkingPeriodStartDate;  //g_CurWorkingPeriodStartDate的互斥量

QString g_ObserveModeTXTFullPath = PathUtils::getRootPath() + "/Setting/ObserveMode_1.txt" ;  //观测模式定义文件的完整路径
QMutex g_Mutex_ObserveModeTXTFullPath;

int g_XYbinning = 1;  //xy方向采取相同的像素合并值（1,2,4,8）
QMutex g_Mutex_XYbinning;

QString g_DataSaveDirPath = PathUtils::getRootPath()+"/DataSaveHere"; //数据文件存储路径（默认为: 项目路径/DataSaveHere）
QMutex g_Mutex_DataSaveDirPath;

//根据当前选择的是地方时还是世界时，返回当前日期时间
QDateTime currentDateTime_UorL(const UsedTimeType &TimeType)
{
    QDateTime NowDateTime;
    switch (TimeType)
    {
    case UTC_Time:
        NowDateTime = QDateTime::currentDateTimeUtc();
        break;
    case Local_Time:
        NowDateTime = QDateTime::currentDateTime();
        break;
    default:
        NowDateTime = QDateTime::fromString("0000-00-00 00:00:00", "yyyy-MM-dd hh:mm:ss");
        break;
    }
    return NowDateTime;
}


//修改UsedTimeType
void set_UsedTimeType(UsedTimeType type)
{
    g_Mutex_UsedTimeType.lock();

    g_UsedTimeType = type;

    g_Mutex_UsedTimeType.unlock();
}

//读取UsedTimeType
UsedTimeType get_UsedTimeType()
{
    g_Mutex_UsedTimeType.lock();

    UsedTimeType type = g_UsedTimeType;

    g_Mutex_UsedTimeType.unlock();

    return type;
}

//修改g_isWorkingUnderAOC
void set_isWorkingUnderAOC(bool ifWorkUnderAOC)
{
    g_Mutex_isWorkingUnderAOC.lock();

    g_isWorkingUnderAOC = ifWorkUnderAOC;

    g_Mutex_isWorkingUnderAOC.unlock();
}

//读取g_isWorkingUnderAOC
bool get_isWorkingUnderAOC()
{
    g_Mutex_isWorkingUnderAOC.lock();

    bool isWorking = g_isWorkingUnderAOC;

    g_Mutex_isWorkingUnderAOC.unlock();

    return isWorking;
}

//修改g_CurWorkingPeriodStartDate
void set_CurWorkingPeriodStartDate(QDate StartDate)
{
    g_Mutex_CurWorkingPeriodStartDate.lock();

    g_CurWorkingPeriodStartDate = StartDate;

    g_Mutex_CurWorkingPeriodStartDate.unlock();
}

//读取g_CurWorkingPeriodStartDate
QDate get_CurWorkingPeriodStartDate()
{
    g_Mutex_CurWorkingPeriodStartDate.lock();

    QDate StartDate = g_CurWorkingPeriodStartDate;

    g_Mutex_CurWorkingPeriodStartDate.unlock();

    return StartDate;
}

//修改
void set_ObserveModeTXTFullPath(QString FullPath)
{
    g_Mutex_ObserveModeTXTFullPath.lock();

    g_ObserveModeTXTFullPath = FullPath;

    g_Mutex_ObserveModeTXTFullPath.unlock();
}

//读取
QString get_ObserveModeTXTFullPath()
{
    g_Mutex_ObserveModeTXTFullPath.lock();

    QString Path = g_ObserveModeTXTFullPath;

    g_Mutex_ObserveModeTXTFullPath.unlock();

    return Path;
}

//设置
void set_XYbinning(int xyBinning)
{
    g_Mutex_XYbinning.lock();

    g_XYbinning = xyBinning;

    g_Mutex_XYbinning.unlock();
}

//读取
int get_XYbinning()
{
    g_Mutex_XYbinning.lock();

    int xyBinning = g_XYbinning;

    g_Mutex_XYbinning.unlock();

    return xyBinning;
}

//设置
void set_DataSaveDirPath(QString DataSavePath)
{
    g_Mutex_DataSaveDirPath.lock();

    g_DataSaveDirPath = DataSavePath;

    g_Mutex_DataSaveDirPath.unlock();
}

//读取
QString get_DataSaveDirPath()
{
    g_Mutex_DataSaveDirPath.lock();

    QString SavePath = g_DataSaveDirPath;

    g_Mutex_DataSaveDirPath.unlock();

    return SavePath;
}

