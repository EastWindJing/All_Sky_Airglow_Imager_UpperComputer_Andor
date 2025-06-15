#ifndef GLOBALVARIABLE_H
#define GLOBALVARIABLE_H

#include <QDateTime>
#include <QMutex>  //提供互斥量

//只进行全局变量的声明，不定义


typedef enum UsedTimeType
{
    UTC_Time,       //世界调整时间（Universal Time Coordinated）
    Local_Time,     //地方时
} UsedTimeType;

extern UsedTimeType g_UsedTimeType;  //上位机使用的时间  UTC_Time: 世界时。Local_Time: 地方时。
extern QMutex g_Mutex_UsedTimeType;

extern bool g_isWorkingUnderAOC;        //是否在AOC的控制下工作
extern QMutex g_Mutex_isWorkingUnderAOC;

extern QDate g_CurWorkingPeriodStartDate;  //当前所在工作时段的开始日期（不管观测时段是否跨天，数据文件一律保存在开始那天的文件夹下）
extern QMutex g_Mutex_CurWorkingPeriodStartDate;

extern QString g_ObserveModeTXTFullPath;    //观测模式定义文件的完整路径
extern QMutex g_Mutex_ObserveModeTXTFullPath;

extern int g_XYbinning;  //xy方向采取相同的像素合并值（1,2,4,8）
extern QMutex g_Mutex_XYbinning;

extern QString g_DataSaveDirPath; //数据文件存储路径（默认为: 项目路径/DataSaveHere）
extern QMutex g_Mutex_DataSaveDirPath;

QDateTime currentDateTime_UorL(const UsedTimeType &TimeType);  //根据当前选择的是地方时还是世界时，返回当前日期时间

void set_UsedTimeType(UsedTimeType type);   //修改UsedTimeType
UsedTimeType get_UsedTimeType();            //读取UsedTimeType

void set_isWorkingUnderAOC(bool isWorking); //修改g_isWorkingUnderAOC
bool get_isWorkingUnderAOC();               //读取g_isWorkingUnderAOC

void set_CurWorkingPeriodStartDate(QDate StartDate);  //修改g_CurWorkingPeriodStartDate
QDate get_CurWorkingPeriodStartDate();                //读取g_CurWorkingPeriodStartDate

void set_ObserveModeTXTFullPath(QString FullPath);
QString get_ObserveModeTXTFullPath();

void set_XYbinning(int xyBinning);      //xy方向的像素合并值
int get_XYbinning();

void set_DataSaveDirPath(QString DataSavePath);  //数据文件存储路径
QString get_DataSaveDirPath();


#endif // GLOBALVARIABLE_H
