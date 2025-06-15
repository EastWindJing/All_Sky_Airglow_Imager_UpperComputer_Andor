#pragma execution_character_set("utf-8")

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <QObject>
#include <QDebug>
#include <QTimer> //提供定时器
#include <QThread>
#include <QString>
#include <QFile>
#include "picam.h"  //提供Princeton Instruments 的控制程序PICam的 SDK
#include "pathutils.h"
#include <QCoreApplication>     //提供QCoreApplication::processEvents()
#include <QDateTime>
//#include <cstdio>  //提供std::remove函数



class CameraController : public QObject
{
    Q_OBJECT

public:
    explicit CameraController(QObject *parent = nullptr);

    QString show_PICamVersion(); //输出PICam版本
    void show_CurCameraID(); //输出相机ID信息
    int get_ReadoutStride(); //从硬件读取当前相机的“读出步幅”
    //float get_ExposureTime(); //<ExposureTime不可读取！！！>从硬件读取当前相机的“曝光时间”, 成功则返回数值，失败返回-1
    float get_CurSensorTemperature(); //从硬件读取相机传感器当前温度, 成功则返回数值，失败返回-1
    char get_CurSensorTemperatureStatus(); //从硬件读取相机传感器当前温度状态,1=UnLocked, 2=Locked, 3=Faulted, 失败返回-1
    float get_TargetSensorTemperature() { return m_TargetSensorTemperature; } //获取相机传感器目标温度
    bool get_CameraIsConnected(PicamCameraID CurCamera_id); //判断相机是否连接
    //char get_CurShuttingTimingMode(); //从硬件获取相机当前的快门模式

    bool set_ShuttingTimingMode(PicamShutterTimingMode ShutterTimingMode); //设置相机当前的快门模式，1自动、2常闭、3常开
    bool set_ExposureTime(float exposureTime); //设置曝光时间
    bool set_TargetSensorTemperature(float targetSensorTemperature); //设置传感器的目标温度


    bool InitializeAndOpenCamera(); //初始化并打开相机
    bool UnInitializeAndCloseCamera(); //UnInit并且关闭相机
    void WaitSensorGotoTargetTemperature(int time_out); //等待传感器达到目标温度,time_out:等待时长(ms), -1表示无限等待。
    bool ShootOneImage(QString FileNamingInfo);  //拍摄1张照片，FileNamingInfo: png的文件名（不含路径、不含后缀）
    bool SaveDataToTiff(QString FileName, QString SavePath_raw, QString SavePath_tif); //保存拍摄的数据为.tif文件(FileName:不含后缀的文件名)
    bool SaveDataToPng(QString FileName, QString SavePath_raw, QString SavePath_png); //保存拍摄的数据为.png文件(FileName:不含后缀的文件名)
    //*************************************************************
    void CameraControllerMain(); //CC的主工作函数，子线程moveto这个函数
    //*************************************************************
    void CCSetCameraXYBinning(int xyBinning); //CC设置相机xy方向的像素合并值

    void StopAcquireImmediately();  //立即让相机停止采集数据，退出Picam_Acquire()函数


private:
    //float CurExposureTime = 0.0; //当前曝光时间
    float m_ExposureTimeAtBeginning = 30000;  //程序启动时相机的初始曝光时间ms
    piflt m_ExposureTime = 0.0;             //曝光时间
    float m_CurSensorTemperature =0.0;      //相机传感器当前温度
    float m_TargetSensorTemperature =0.0;   //相机传感器目标温度
    piint m_ReadoutStride = 0;              //读出步幅（每张照片在数据缓冲区中占多少位）
    int m_ImageWidth = 1024;              //相机传感器的获取的图像宽度（单位：像素）
    int m_ImageHeight = 1024;             //相机传感器的获取的图像高度（单位：像素）
    PicamAvailableData m_data;              //储存指向数据 起始位置指针、连续可用数据数量 的结构体
    QString m_FileName = "MySample_TestFile";      //不含后缀的数据文件名


    QString m_SavePath_raw = "/";
    QString m_SavePath_tif = "/";
    QString m_SavePath_png = "/";

    pibln m_CameraIsConnected = false; //是否连接了相机

    PicamHandle m_CurCamera_Handle; //当前连接的相机句柄
    PicamCameraID m_CurCamera_id;   //当前连接的相机ID
    PicamSensorTemperatureStatus m_SensorTemperatureStatus; //相机传感器温度状态（UnLocked(1)、Locked(2)、Faulted(3)）
    PicamAcquisitionErrorsMask m_ErrorsMask; //采集Error Mask
    PicamShutterTimingMode m_ShutterTimingMode = PicamShutterTimingMode_Normal;  //相机快门模式：自动、常闭、常开

    QTimer* m_TimerPtr; //CC的专属定时器的指针

    //bool m_ReachTargetTemperatureNow = false;  //当前是否到达目标温度

    //const PicamCameraID* m_CameraID_array; //指向存储可用相机id列表的数组的内存地址的指针。
    //piint m_CameraID_count = 0; //存储在CameraID_array中的可用相机id的总数。这等于已经创建的数组的长度。当没有可用的摄像机id时，返回0 [zero]
    //const PicamCameraID* m_CurCamera_id_ptr = m_CameraID_array;


signals:
    void CC_Report_Error(QString ErrorInfo); //CC向MainWindow通告错误信息
    void CC_Report_Info(QString Info); //CC向MainWindow通告提示信息

    void CC_Report_WhetherCameraIsConnected(bool IfConnected); //向UI通知相机是否连接上

    void CC_Report_NewCurSensorTemperature(float NewTemperature); //传感器当前温度有了新值
    void CC_Report_NewCurExposureTime(float NewExposureTime); //曝光时间有了新值
    void CC_Report_NewCurShutterTimingMode(int NewShutterTimingMode); //快门模式有了新值

    void CC_Report_OperateCameraTemperature_Finish();   //当前温度到达目标温度
    void CC_Report_OperateCameraShutterTimingMode_Finish();  //AOC设置相机快门模式完成
    void CC_Report_OperateCameraExposureTime_Finish();       //AOC设置相机曝光时间完成
    void CC_Report_OperateCameraAcquireAndSave_Finish();     //AOC指挥相机采集数据并保存 完成
    void CC_Report_OperateCameraConnect_Finish();            //AOC控制连接/断开相机 完成

    void CC_Report_PauseAutoObserve();  //当模式中的某一个动作执行失败时，发射此信号暂停自动观测（阻塞AOC线程）

    void CC_Report_DisplayLastImage(QString TiffFileFullPath);  //每成功保存一张tif文件就通知MainWindow把该图加载到LastImageDisplayWindow上（但子窗口不一定显示）

    void CC_Report_StartAcquireData(QDateTime StartAcquireTime);    //CC通知UI，相机开始采集数据，StartAcquireTime: 开始采集的时间

public slots:

    void CC_Respond_ConnectedCamera(); //处理 连接相机
    void CC_Respond_DisConnectedCamera(); //处理 断开相机

    void CC_Respond_SetTargetTemperature(float TargetTemperature); //处理 设置目标温度
    void CC_Respond_SetShutterTimingMode(int ShutterTimingMode); //处理 设置快门模式，1自动、2常闭、3常开
    void CC_Respond_SetExposureTime(float ExposureTime); //处理 设置曝光时间
    void CC_Respond_ShootOneImage(QString FileNamingInfo); //处理 拍摄单张照片

    //AOC专用的槽函数
    void CC_Respond_OperateCameraTemperature(float TargetTemperature); //slots: 处理 AOC置目标温度
    void CC_Respond_OperateCameraShutterTimingMode(int ShutterTimingMode); //slots: 处理 AOC设置快门模式，1自动、2常闭、3常开
    void CC_Respond_OperateCameraExposureTime(float ExposureTime); //slots: 处理 AOC设置曝光时间
    void CC_Respond_OperateCameraAcquireAndSave(QString FileNamingInfo, float CurExposureTime); //slots: 处理 AOC拍摄单张照片,FileNamingInfo: 当前拍摄文件的文件名（不含路径、不含后缀）
    void CC_Respond_OperateCameraConnect(bool ConnectOrDisconnect);     //slots: 处理 AOC控制连接/断开相机

private slots:
    void onTimeout(); //CCTimer超时后执行

};

#endif // CAMERACONTROLLER_H
