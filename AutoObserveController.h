#pragma execution_character_set("utf-8")

#ifndef AUTOOBSERVECONTROLLER_H
#define AUTOOBSERVECONTROLLER_H

#include <QObject>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QTimer> //提供定时器
#include <QMutex> //提供互斥量
#include <QWaitCondition>  //提供用来同步线程的条件变量QWaitCondition
#include "pathutils.h"
#include <QCoreApplication>     //提供QCoreApplication::processEvents()


struct FileNamingInfo
{
    // QString VFAJ_ASI01_IA;      // = "VFAJ_ASI01_IA";
    QString VFAJ_ASA01_IA;      // = "VFAJ_ASI01_IA";
    QString CurWaveBand;        //X: 862nm. Y: 630nm. Z: 750-850nm.
    QString _L11_STP_;          // = "_L11_STP_";
    QDateTime   CurDateTime;    //年月日时分秒（世界时）
    FileNamingInfo():VFAJ_ASA01_IA("VFAJ_ASA01_IA"),_L11_STP_("_L11_STP_"){}  //用构造函数初始化部分成员
};


class AutoObserveController : public QObject
{
    Q_OBJECT

public:
    explicit AutoObserveController(QObject *parent = nullptr);

    bool ReadTimeTableTXT(QString TimeTableTXTFullPath);  //读取时间表TimeTable.txt， TimeTableTXTFullPath 包含文件名
    void ExtractRemainWorkPeriodList();                   //从时间表中提取出所有起始时间大于currentTime的时段，它们就是剩余工作时段
    //bool ReadObserveModeTXT(QString ObserveModeTXTPath, int CurMode_index);  //读取观测模式文件ObserveMode_index.txt, ObserveModeTXTPath不含文件名
    bool ReadObserveModeTXT(QString FilePath);  //读取观测模式定义文件，FilePath：完整路径
    void ExecuteCurObserveModeOneSection(int SectionNum);    //执行当前所选观测模式的一个section
    void WriteObservLogTXT(QString DataSavePath, QString LogEntry);            //在存数据的文件夹中，每天一个日志文件
    //###################################
    void AutoObserveControllerMain();  //AOC主工作函数，UI上的“开始自动观测btn_clicked” connect到此函数
    //###################################
    void PauseAutoObserve();          //暂停自动观测
    void ResumeAutoObserve();         //继续自动观测***UI线程调用从而消除AOC线程的阻塞***
    void TerminateAutoObserve();      //结束自动观测

    void ReWrite_ASI_CurWorkPeriodStartDateTXT();    //重写文件ASI_CurWorkPeriodStartDate.txt

    //void AwakeSubThreadAOC();       //***供UI线程调用从而消除AOC线程的阻塞***（改用了ResumeAutoObserve()）
private:

    QString m_ObserveLogTXTPrefixName = "VFAJ_ASA01_DLG_L11_STP_";     //观测日志文件的名字前缀（不含路径，不含时间戳，不含.txt后缀）
    float m_CurExposureTime = 30000;  //当前曝光时间ms（传给CC，用于控制Picam_Acquire()按曝光时间超时，从而报错）、也用于写日志文件
    QDateTime m_ShoottingDateTime;   //用于写日志文件(开始拍摄照片的时刻)
    QString m_CurFilter = "Unknown";                                //用于写日志文件
    QString m_CurCameraShuttingTimingMode = "Auto";                 //用于写日志文件
    QString m_AutoObserveState = "End";                             //用于写日志文件(Start、Working、End)
    QString m_LatestLogEntry = "";                                  //日志文件中最近的一条记录


    QString m_ROOT_DIR = PathUtils::getRootPath(); //项目目录
    QString m_TimeTableTXTFullPath = m_ROOT_DIR + "/Setting/TimeTable.txt" ;     //TimeTable.txt存储路径(含文件名)

    int m_RemainWorkPeriodNum = 0;    //剩余有效工作时段数量
    int m_CurWorkingPeriodIndex = -1;    //当前在第几个工作时段（在TimeTable.txt里de第几行，-1表示不在任何工作时段）
    bool m_WhetherWorkingLastTimeout = false;  //记录上一次定时器超时的时候，设备是否在工作（是否处于观测时段内）
    int m_SectionNumLastTimeout = -1;     //上一次定时器超时的时候，在执行哪一节（-1表示没在执行任何一节）

    //QString m_ObserveModeTXTPath = m_ROOT_DIR + "/Setting" ;   //ObserveMode_index.txt存储路径(不包含 /文件名)
    unsigned int m_CurMode_index = 1;          //当前所选模式的Index(>0)
    int m_SectionRepeatTimes = 1;     //Section重复执行的次数，-1表示无限重复
    bool m_WhetherSectionContinueRepeat = false;  //Section是否继续循环.当不在观测时段时，置为false;当重复次数到0时，置为false;进入观测时段，置为true;


    // 时间表
    QVector<QPair<QDateTime, QDateTime>> m_TimeTable;  // 存储时间表的向量容器，每个元素是一个<开始时间,结束时间>对
    QVector<QPair<QDateTime, QDateTime>> m_RemainWorkPeriodList;  //剩余工作时段List（m_TimeTable中所有起始时间大于currentTime的时段加入此中）

    QPair<QDateTime, QDateTime> m_CurWorkingStartAndEndTime;  //当前所在的工作时段<开始时间，结束时间>

    // 指令集合
    QVector<QPair<QString,QString>> m_ModeSectionOne;
    QVector<QPair<QString,QString>> m_ModeSectionTwo;
    QVector<QPair<QString,QString>> m_ModeSectionThree;

    FileNamingInfo m_FileNamingInfo;        //tif文件命名信息（最新文件名）

    QTimer* m_TimerPtr; //AOC的专属定时器的指针

    bool m_pause = false;   // 线程暂停标志
    QMutex m_mutex; // 保护m_paused的互斥量
    QWaitCondition m_pauseCondition; // 用于m_mutex的条件变量
    //QMutex m_mutex_Auto;  //用于阻塞自动观测的一步步进行，直至UI(Power)、MC、CC完成动作后给出信号
    //QWaitCondition m_autoCondition;  // 用于m_mutex_Auto的条件变量

    //bool m_stopped = false;  // 线程停止标志

    bool m_IfCameraConnected = false;           //AOC中存储的 相机 是否连接状态
    bool m_IfFilterControllerConnedted = false;  //AOC中存储的 滤波轮 是否连接状态
    bool m_IfPowerConnedted = false;            //AOC中存储的 电源 是否连接状态

    bool m_IfOperatePowerSwitch_Finished = false;             //AOC控制电源 完成标志
    //bool m_IfOperateSwivelTable_Finished = false;             //AOC控制转台 完成标志
    bool m_IfOperateFilter_Finished = false;                  //AOC控制滤波轮 完成标志
    bool m_IfOperateCameraTemperature_Finished = false;       //AOC控制相机温度 完成标志
    bool m_IfOperateCameraShutterTimingMode_Finished = false; //AOC控制快门模式 完成标志
    bool m_IfOperateCameraExposureTime_Finished = false;      //AOC控制曝光时间 完成标志
    bool m_IfOperateCameraAcquireAndSave_Finished = false;    //AOC控制相机拍摄 完成标志
    bool m_IfOperateCameraConnect_Finished = false;           //AOC控制相机连接/断开 完成标志


signals:
    //1.自己发信号告诉自己一些当前状态，然后执行下一步，就不用使用死循环等待占用线程了。比如：观测Date到了？观测Time到了？结束Time到了？……
    //2.通知MC和CC该执行什么动作了
    void AOC_Report_Error(QString ErrorInfo); //AOC向MainWindow通告错误信息
    void AOC_Report_PauseAutoObserve();   //AOC内部产生一些需要阻塞AOC的时候，要发此信号通知UI更新暂停按钮为“继续”
    void AOC_Report_FinishedAllWorkPeriod();    //完成了所有的有效观测时段，通知UI更新“启动暂停结束btn” 并 退出AOC线程

    void AOC_Send_DataFileNamingInfo(QString DataFileNamingInfo);   //发送数据文件命名信息

    void AOC_Send_OperatePowerSwitch(int RelayIndex, char ONorOFF);                             //操纵继电器开启（ONorOFF==1）或关闭（ONorOFF==0），编号RelayIndex==-1表示全部继电器
    void AOC_Send_OperateFilter(QString CurWaveBand);                                           //操纵滤波轮波段
    void AOC_Send_OperateCameraTemperature(float CameraTargetTemperature);                      //操纵相机温度
    void AOC_Send_OperateCameraShutterTimingMode(int CameraShutterTimingMode);                  //操纵相机快门模式
    void AOC_Send_OperateCameraExposureTime(float CameraExposureTime);                          //操纵相机设置曝光时间
    void AOC_Send_OperateCameraAcquireAndSave(QString FileNamingInfo, float CurExposureTime);   //操纵相机采集数据并保存文件
    void AOC_Send_OperateCameraConnect(bool ConnectOrDisConnect);                               //操纵相机连接/断开

    void AOC_Send_CurWorkingStartAndEndTime(QString StartWorkingTime1,QString EndWorkingTime1,QString StartWorkingTime2,QString EndWorkingTime2);  //AOC向UI通告当前工作时段
    void AOC_Send_MessageBoxOutput(QString OneMessage);    //AOC在自动观测时，向UI上的消息盒子发送输出信息

public slots:
    //1.接收MC和CC的状态更新信号，做出反应
    //2.接收UI的开始观测、暂停、结束 信号
    void AOC_Update_IfCameraConnected(bool ifConnected) { m_IfCameraConnected = ifConnected;}         //处理-更新AOC记录的 相机 连接状态
    void AOC_Update_IfFilterControllerConnected(bool ifConnected){m_IfFilterControllerConnedted = ifConnected;}//处理-更新AOC记录的 滤波轮 连接状态
    void AOC_Update_IfPowerConnected(bool ifConnected){m_IfPowerConnedted = ifConnected;}             //处理-更新AOC记录的 电源 连接状态


    void AOC_Respond_PauseAutoObserve(){ PauseAutoObserve(); }            //处理-暂停自动观测
    void AOC_Respond_ResumeAutoObserve(){ ResumeAutoObserve(); }          //处理-继续自动观测
    void AOC_Respond_TerminateAutoObserve(){ TerminateAutoObserve(); }    //处理-终止自动观测

    void AOC_Update_IfOperatePowerSwitch_Finished(){m_IfOperatePowerSwitch_Finished = true;}  //处理-更新AOC操作电源是否完成标志
    void AOC_Update_IfOperateFilter_Finished(){m_IfOperateFilter_Finished = true;}
    void AOC_Update_IfOperateCameraTemperature_Finished(){m_IfOperateCameraTemperature_Finished = true;}
    void AOC_Update_IfOperateCameraShutterTimingMode_Finished(){m_IfOperateCameraShutterTimingMode_Finished = true;}
    void AOC_Update_IfOperateCameraExposureTime_Finished(){m_IfOperateCameraExposureTime_Finished = true;}
    void AOC_Update_IfOperateCameraAcquireAndSave_Finished(){m_IfOperateCameraAcquireAndSave_Finished = true;}
    void AOC_Update_IfOperateCameraConnect_Finished(){m_IfOperateCameraConnect_Finished = true;}

    void AOC_Update_TimeTableFullPath(QString TimeTableFullPath){m_TimeTableTXTFullPath = TimeTableFullPath;}   //更新时间表文件完整路径


private slots:
    //接收自己
    void onTimeout(); //AOC Timer超时后执行

};

#endif // AUTOOBSERVECONTROLLER_H
