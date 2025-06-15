#pragma execution_character_set("utf-8")  //告诉编译器在编译源代码时使用 UTF-8 字符编码作为执行字符集。

#include "CameraController.h"
//#include "opencv2/opencv.hpp"     //一个头文件提供所有支持
#include <opencv2/core.hpp>         //提供cv::Mat等
#include <opencv2/imgcodecs.hpp>    //提供cv::imwrite等
#include "GlobalVariable.h"

//构造函数
CameraController::CameraController(QObject *parent)
    : QObject{parent}
{
    //Do Something……

}

//输出PICam版本
QString CameraController::show_PICamVersion()
{
    piint major,minor,distribution,released;
    Picam_GetVersion(&major,&minor,&distribution,&released);
    QString PICamVersion = QString::number(major)+'.'+QString::number(minor)+'.'+QString::number(distribution)+'.'+QString::number(released);
    qDebug()<<"当前PICam版本是： "<< PICamVersion ;
        return PICamVersion;
}

//输出相机的ID信息
void CameraController::show_CurCameraID()
{
    const pichar* charArray_ptr;
    Picam_GetEnumerationString( PicamEnumeratedType_Model, m_CurCamera_id.model, &charArray_ptr );
    qDebug()<<"当前相机ID信息：";
    qDebug()<<"    1.Model : "<<charArray_ptr;
    qDebug()<<"    2.Sensor_Name："<<m_CurCamera_id.sensor_name;
    qDebug()<<"    3.Serial_Number："<<m_CurCamera_id.serial_number;
    Picam_DestroyString( charArray_ptr );

    const PicamFirmwareDetail*  FirmwareDetail_array;
    piint FirmwareDetail_count = 0;
    Picam_GetFirmwareDetails(&m_CurCamera_id, &FirmwareDetail_array, &FirmwareDetail_count); //动态创建一个长度为n的数组。该数组存储与指定相机ID相关的固件详细信息。
    if(FirmwareDetail_count > 0)
    {
        qDebug()<<"读取到"<<FirmwareDetail_count<<"条当前相机固件的详细信息：";
        for(char i=0; i<FirmwareDetail_count; i++)
        {
            qDebug()<<"    1.PicamFirmwareDetail[" << i+1 << "].name："<< FirmwareDetail_array[i].name;
            qDebug()<<"    2.PicamFirmwareDetail[" << i+1 << "].detail："<< FirmwareDetail_array[i].detail;
        }
    }


}

//从硬件读取当前相机的“读出步幅”
int CameraController::get_ReadoutStride()
{
    Picam_GetParameterIntegerValue( m_CurCamera_Handle, PicamParameter_ReadoutStride, &m_ReadoutStride );
    return m_ReadoutStride;
}

// //从硬件读取当前相机的“曝光时间”  <函数无用，ExposureTime是Not Readable参数！！！>
// float CameraController::get_ExposureTime()
// {
//     PicamError ReadExposureTime_Error;
//     piflt CurExposureTime;
//     ReadExposureTime_Error = Picam_ReadParameterFloatingPointValue( m_CurCamera_Handle, PicamParameter_ExposureTime, &CurExposureTime );
//     if( ReadExposureTime_Error == PicamError_None )
//     {
//         //如果读取曝光时间成功，则输出曝光时间数值
//         qDebug() << "当前曝光时间 ExposureTime = "<< CurExposureTime << "ms";
//         return CurExposureTime;
//     }
//     else
//     {
//         //如果读取曝光时间失败，则输出提示信息
//         const pichar* CharArray_ptr;
//         Picam_GetEnumerationString( PicamEnumeratedType_Error, ReadExposureTime_Error, &CharArray_ptr );
//         qDebug() << "读取曝光时间失败！错误信息：PicamError = " << CharArray_ptr;
//         Picam_DestroyString( CharArray_ptr );
//         return -1.0;
//     }
// }


//从硬件读取相机传感器当前温度
float CameraController::get_CurSensorTemperature()
{
    PicamError ReadTemperature_Error;
    piflt CurTemperature;
    ReadTemperature_Error = Picam_ReadParameterFloatingPointValue( m_CurCamera_Handle, PicamParameter_SensorTemperatureReading, &CurTemperature );
    if( ReadTemperature_Error == PicamError_None )
    {
        //如果读取温度成功，则返回温度数值
        //qDebug() << "相机传感器的当前温度 CurTemperature = "<< CurTemperature << "°C";
        //return CurTemperature;
        return (CurTemperature-5);
    }
    else
    {
        //如果读取温度失败，则输出提示信息，返回-1
        const pichar* CharArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, ReadTemperature_Error, &CharArray_ptr );
        //QString ErrorMessage = "读取温度失败！执行Picam_ReadParameterFloatingPointValue()时出错，PicamError = " + QString::fromUtf8(CharArray_ptr);
        QString ErrorMessage = "读取温度失败！PicamError = " + QString::fromUtf8(CharArray_ptr);
        qDebug() << ErrorMessage;
        emit CC_Report_Info(ErrorMessage);
        Picam_DestroyString( CharArray_ptr );
        return -1.0;
    }
}


//从硬件读取相机传感器当前温度状态
char CameraController::get_CurSensorTemperatureStatus()
{
    //PicamSensorTemperatureStatus_Unlocked = 1 : 温度还没有稳定到设定值
    //PicamSensorTemperatureStatus_Locked   = 2 : 温度已稳定在设定值
    //PicamSensorTemperatureStatus_Faulted  = 3 : 传感器冷却故障。
    PicamError ReadSensorTemperatureStatus_Error;
    ReadSensorTemperatureStatus_Error = Picam_ReadParameterIntegerValue(m_CurCamera_Handle,
                                                                        PicamParameter_SensorTemperatureStatus,
                                                                        reinterpret_cast<piint*>( &m_SensorTemperatureStatus ) );
    if( ReadSensorTemperatureStatus_Error == PicamError_None )
    {
        //如果读取温度状态成功，则输出温度状态
        const pichar* CharArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_SensorTemperatureStatus, m_SensorTemperatureStatus, &CharArray_ptr );
        qDebug() << "相机传感器温度状态 SensorTemperatureStatus = " << CharArray_ptr;
        Picam_DestroyString( CharArray_ptr );
        return m_SensorTemperatureStatus;
    }
    else
    {
        //如果读取温度状态失败，则输出提示信息,返回-1
        const pichar* CharArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, ReadSensorTemperatureStatus_Error, &CharArray_ptr );
        QString ErrorMessage = "读取温度状态失败！错误信息：PicamError = " + QString::fromUtf8(CharArray_ptr);
        qDebug() << ErrorMessage;
        emit CC_Report_Error(ErrorMessage);
        Picam_DestroyString( CharArray_ptr );
        return -1;
    }
}

//判断相机是否连接
bool CameraController::get_CameraIsConnected(PicamCameraID CurCamera_id)
{
    //检测指定ID的相机是否已连接并打开
    const PicamCameraID* NowConnectCameraID = &CurCamera_id;
    Picam_IsCameraIDConnected(NowConnectCameraID, &m_CameraIsConnected); //确定指定的相机是否已插入主机并已打开
    emit CC_Report_WhetherCameraIsConnected(m_CameraIsConnected); //通知UI更新相机连接状态
    return m_CameraIsConnected;
}


//设置相机的快门模式：1自动、2常闭、3常开
bool CameraController::set_ShuttingTimingMode(PicamShutterTimingMode ShutterTimingMode)
{
    //step1：SetParameter枚举值
    PicamError SetShuttingTimingMode_Error = Picam_SetParameterIntegerValue(m_CurCamera_Handle,
                                                                            PicamParameter_ShutterTimingMode,
                                                                            ShutterTimingMode );
    //---------------------------------------------------------------------------------------------------------//
    if(SetShuttingTimingMode_Error == PicamError_None)         //①如果setParam成功
    {
        //step2: 将更改过的参数commit到硬件
        const PicamParameter* failed_parameters;
        piint failed_parameters_count;
        PicamError CommitShuttingTimingMode_Error = Picam_CommitParameters(m_CurCamera_Handle,
                                                                           &failed_parameters,
                                                                           &failed_parameters_count );
        //------------------------------------------------------------------------------------------------------//
        if(CommitShuttingTimingMode_Error == PicamError_None)  //②如果Commit Param也成功
        {
            m_ShutterTimingMode =  ShutterTimingMode;
            switch (m_ShutterTimingMode)
            {
            case PicamShutterTimingMode_Normal:
                qDebug() << "快门模式修改成功！当前快门模式 = 自动";
                break;
            case PicamShutterTimingMode_AlwaysClosed:
                qDebug() << "快门模式修改成功！当前快门模式 = 常闭";
                break;
            case PicamShutterTimingMode_AlwaysOpen:
                qDebug() << "快门模式修改成功！当前快门模式 = 常开";
                break;
            case PicamShutterTimingMode_OpenBeforeTrigger:
                qDebug() << "快门模式修改成功！当前快门模式 = OpenBeforeTrigger";
                break;
            default:
                qDebug() << "快门模式修改成功！当前快门模式 = 未知";
                break;
            }
            emit CC_Report_NewCurShutterTimingMode(int(m_ShutterTimingMode)); //通知UI更新快门模式
            Picam_DestroyParameters( failed_parameters );
            return true;
        }
        else                                                   //③如果Commit Param错误
        {
            const pichar* CommitErrorCharArray_ptr;
            Picam_GetEnumerationString( PicamEnumeratedType_Error, CommitShuttingTimingMode_Error, &CommitErrorCharArray_ptr );
            QString ErrorMessage = "修改快门模式失败！具体是在Picam_CommitParameters()时出错，PicamError = " + QString::fromUtf8(CommitErrorCharArray_ptr);
            qDebug() << ErrorMessage ;
            emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
            Picam_DestroyString( CommitErrorCharArray_ptr );
            Picam_DestroyParameters( failed_parameters );
            return false;
        }

    }
    else                                                       //④如果setParam出错
    {
        const pichar* CharArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, SetShuttingTimingMode_Error, &CharArray_ptr );
        QString ErrorMessage = "修改快门模式失败！\n具体是在Picam_SetParameterIntegerValue()时出错，PicamError = " + QString::fromUtf8(CharArray_ptr);
        qDebug() << ErrorMessage ;
        emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
        Picam_DestroyString( CharArray_ptr );
        return false;
    }

}


//设置曝光时间
bool CameraController::set_ExposureTime(float exposureTime)
{
    //step1: SetParameter浮点值
    piflt ExposureTime = exposureTime;
    PicamError SetExposureTime_Error;
    SetExposureTime_Error = Picam_SetParameterFloatingPointValue(m_CurCamera_Handle, PicamParameter_ExposureTime, ExposureTime );
    if(SetExposureTime_Error == PicamError_None)
    {
        //step2: 将更改过的参数commit到硬件
        const PicamParameter* failed_parameters;
        piint failed_parameters_count;
        PicamError CommitExposureTime_Error = Picam_CommitParameters( m_CurCamera_Handle, &failed_parameters, &failed_parameters_count );
        //    Picam_CommitParameters()验证并提交参数值给设备。在系统设置和配置期间将有效的参数值应用于指定的相机。
        //    若某些参数未能满足 所需约束 ，则被标记为无效，并存储在failed_parameter_array中。
        //    无效参数的数量存储在failed_parameter_count中。
        if(CommitExposureTime_Error == PicamError_None)
        {
            m_ExposureTime = ExposureTime;
            qDebug() << "曝光时间修改成功！新的ExposureTime = "<< m_ExposureTime <<"ms";
            emit CC_Report_NewCurExposureTime(m_ExposureTime/1000); //通知UI更新曝光时间s
            Picam_DestroyParameters( failed_parameters );
            return true;
        }
        else
        {
            // 输出错误提示
            const pichar* CommitErrorCharArray_ptr;
            Picam_GetEnumerationString( PicamEnumeratedType_Error, CommitExposureTime_Error, &CommitErrorCharArray_ptr );
            QString ErrorMessage = "修改曝光时间（至" + QString::number(ExposureTime) + "ms）失败！\n"+
                                   "具体是在Picam_CommitParameters()时出错，PicamError = " + QString::fromUtf8(CommitErrorCharArray_ptr);
            qDebug() << ErrorMessage ;
            emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
            Picam_DestroyString( CommitErrorCharArray_ptr );
            Picam_DestroyParameters( failed_parameters );
            return false;
        }

    }
    else
    {
        const pichar* CharArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, SetExposureTime_Error, &CharArray_ptr );
        QString ErrorMessage = "修改曝光时间（至" + QString::number(ExposureTime) + "ms）失败！\n"+
                               "具体是在Picam_SetParameterFloatingPointValue()时出错，PicamError = " + QString::fromUtf8(CharArray_ptr);
        qDebug() << ErrorMessage ;
        emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
        Picam_DestroyString( CharArray_ptr );
        return false;
    }
}


//设置传感器的目标温度
bool CameraController::set_TargetSensorTemperature(float targetSensorTemperature)
{
    //NOTE: 当前传感器温度状态=Unlocked才能开始校准温度。
    m_SensorTemperatureStatus = PicamSensorTemperatureStatus_Unlocked;
    if(m_SensorTemperatureStatus == PicamSensorTemperatureStatus_Unlocked)
    {
        PicamError SetTemperature_Error;
        //piflt TargetTemperature = targetSensorTemperature;
        piflt TargetTemperature = targetSensorTemperature + 5;
        SetTemperature_Error = Picam_SetParameterFloatingPointValue(m_CurCamera_Handle, PicamParameter_SensorTemperatureSetPoint, TargetTemperature);
        if(SetTemperature_Error == PicamError_None)
        {
            // 将设定的目标温度Commit到硬件
            const PicamParameter* failed_parameters;
            piint failed_parameters_count;
            PicamError CommitTargetTemperature_Error = Picam_CommitParameters( m_CurCamera_Handle, &failed_parameters, &failed_parameters_count );
            if(CommitTargetTemperature_Error == PicamError_None)
            {
                //如果Commit也没错误，则目标温度设定成功，输出提示
                qDebug() << "目标温度设定成功！Target_Temperature = " << targetSensorTemperature << "°C";
                m_TargetSensorTemperature = TargetTemperature;  //更新m_TargetSensorTemperature
                //m_ReachTargetTemperatureNow = false;            //更新m_ReachTargetTemperatureNow
                Picam_DestroyParameters( failed_parameters );
                return true;
            }
            else
            {
                //如果设定目标温度失败，则输出错误信息
                const pichar* CommitErrorCharArray_ptr;
                Picam_GetEnumerationString( PicamEnumeratedType_Error, CommitTargetTemperature_Error, &CommitErrorCharArray_ptr );
                QString ErrorMessage = "设定目标温度（为" + QString::number(TargetTemperature) + "°C）失败！\n"+
                                       "具体是在Picam_CommitParameters()时出错，PicamError = " + QString::fromUtf8(CommitErrorCharArray_ptr);
                qDebug() << ErrorMessage ;
                emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
                Picam_DestroyString( CommitErrorCharArray_ptr );
                Picam_DestroyParameters( failed_parameters );
                return false;
            }

        }
        else
        {
            //如果设定目标温度失败，则输出错误信息
            const pichar* CharArray_ptr;
            Picam_GetEnumerationString( PicamEnumeratedType_Error, SetTemperature_Error, &CharArray_ptr );    
            QString ErrorMessage = "设定目标温度（为" + QString::number(TargetTemperature) + "°C）失败！\n"+
                                    "具体是在Picam_SetParameterFloatingPointValue()时出错，PicamError = " + QString::fromUtf8(CharArray_ptr);
            qDebug() << ErrorMessage ;
            emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
            Picam_DestroyString( CharArray_ptr );
            return false;
        }
    }
    else
    {
        QString ErrorMessage = "设定传感器TargetTemperature失败！仅当传感器温度状态 SensorTemperatureStatus = Unlocked 时，才能开始校准温度!";
        qDebug() << ErrorMessage ;
        emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
        return false;
    }

}


//初始化并打开相机
bool CameraController::InitializeAndOpenCamera()
{
    //初始化PICamLibrary
    Picam_InitializeLibrary(); //除非特别说明，否则必须在调用任何其他库API例程之前调用Picam_InitializeLibrary()

    //打开相机（若无可用相机, 则创建虚拟相机）
    PicamError OpenFirstCamera_Error = Picam_OpenFirstCamera(&m_CurCamera_Handle);
    if ( OpenFirstCamera_Error== PicamError_None) //打开第一个可用的相机，并返回相机句柄。
    {
        Picam_GetCameraID(m_CurCamera_Handle, &m_CurCamera_id); //获取已打开相机的ID
        qDebug()<<"打开相机成功！";
        show_CurCameraID(); //打印输出当前相机的ID信息
        //CCSetCameraXYBinning(get_XYbinning()); //设置像素合并值
        qDebug()<<"读出步幅 ReadoutStride = "<< get_ReadoutStride(); //从硬件读取相机的“读出步幅”
        emit CC_Report_WhetherCameraIsConnected(true); //向UI通知相机是否连接上
        m_CameraIsConnected = true;
        return true;
    }
    else
    {
        //Picam_ConnectDemoCamera( PicamModel_Pixis100F, "0008675309", &m_CurCamera_id );
        Picam_ConnectDemoCamera( PicamModel_Pixis1024BRExcelon, "X030035824", &m_CurCamera_id );
        Picam_OpenCamera( &m_CurCamera_id, &m_CurCamera_Handle );
        qDebug()<<"未检测到可用相机, 程序已自动创建虚拟相机。";
        show_CurCameraID(); //输出相机ID信息
        qDebug()<<"读出步幅 ReadoutStride = "<< get_ReadoutStride(); //从硬件读取相机的“读出步幅”

        const pichar* CharArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, OpenFirstCamera_Error, &CharArray_ptr );
        QString ErrorMessage = "未检测到可用相机, PicamError = " + QString::fromUtf8(CharArray_ptr) +". 程序已自动创建虚拟相机！";
        emit CC_Report_Error(ErrorMessage);
        emit CC_Report_WhetherCameraIsConnected(true);
        m_CameraIsConnected = true;
        return true;
    }
}

//UnInit并且关闭相机
bool CameraController::UnInitializeAndCloseCamera()
{
    //关闭相机
    Picam_CloseCamera(m_CurCamera_Handle); //释放与指定相机关联的所有资源
    Picam_DestroyCameraIDs (&m_CurCamera_id); //释放与CameraID_array相关的picam分配的内存。
    Picam_UninitializeLibrary(); //必须在程序终止前调用Picam_UninitializeLibrary()，释放API已使用的资源库，包括开放式摄像头和存储器。
    qDebug()<<"相机已关闭!";
    emit CC_Report_WhetherCameraIsConnected(false);
    m_CameraIsConnected = false;
    return true;
}

//等待传感器达到目标温度(未使用)
void CameraController::WaitSensorGotoTargetTemperature(int time_out)
{
    piint Time_out = time_out;
    PicamError WaitSensorForTargetTemp_Error;
    //Picam_WaitForStatusParameterValue()等待指定参数parameter到达特定的状态value 或 直到time_out毫秒已经过。
    WaitSensorForTargetTemp_Error = Picam_WaitForStatusParameterValue( m_CurCamera_Handle,
                                                                       PicamParameter_SensorTemperatureStatus, //parameter:等待状态的参数。备注 : 指定的参数必须为可等待状态。参考Picam_CanWaitForStatusParameter()获取更多信息。
                                                                       PicamSensorTemperatureStatus_Locked, //value : 指定等待的状态。备注 : 指定的值必须在状态范围之内。有关信息，参阅Picam_GetStatusParameterPurview()。
                                                                       Time_out ); //time_out : 等待时间，单位为毫秒。注意 : 使用 -1 表示无限期等待
    //???????????????????????????????????????????????????????????????????????????????
    //      可以考虑弃用 等待降温函数 ，因为会占用线程
    //???????????????????????????????????????????????????????????????????????????????
    if(WaitSensorForTargetTemp_Error == PicamError_None)
    {
        //如果成功到达目标温度，则输出提示
        qDebug() << "成功到达目标温度！CurTemperature = " << get_CurSensorTemperature() << "°C";
    }
    else
    {
        //如果到达目标温度失败，则输出错误信息
        const pichar* WaitCharArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, WaitSensorForTargetTemp_Error, &WaitCharArray_ptr );
        QString ErrorMessage = "前往目标温度失败！\n错误信息：PicamError = " + QString::fromUtf8(WaitCharArray_ptr);
        qDebug() << ErrorMessage ;
        emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
        Picam_DestroyString( WaitCharArray_ptr );
    }
}


// 拍摄1张照片
bool CameraController::ShootOneImage(QString FileNamingInfo)
{
    //在开始采集之前读取当前日期，可防止手动拍摄时曝光完成后进入第二天，从而导致照片被存到第二天的文件夹
    //自动观测时，无论是否跨天，文件一律存在开始日期对应的文件夹内
    QString CurrentDay;
    if(get_isWorkingUnderAOC())
        CurrentDay = get_CurWorkingPeriodStartDate().toString("yyyyMMdd");    //获取当前工作时段的起始日期
    else
        CurrentDay = currentDateTime_UorL(get_UsedTimeType()).date().toString("yyyyMMdd");  //获取当前日期


    qDebug()<< "----------------------- 一次拍摄&存储开始！-----------------------";
    pi64s readout_count = 1;
    piint readout_time_out = static_cast<piint>(m_ExposureTime)+10*1000; //每两次连续读出的时间间隔(ms)，-1:无限长.在曝光时间的基础上多等10s
    PicamAcquisitionErrorsMask AcqErrorMask;

    emit CC_Report_StartAcquireData(QDateTime::currentDateTime());    //CC通知UI，相机开始采集数据/曝光了

    PicamError Acquire_Error = Picam_Acquire( m_CurCamera_Handle, readout_count, readout_time_out, &m_data, &AcqErrorMask );
    //      Picam_Acquire()执行指定数量的数据读出(由readout_count指定)，并在采集完成后返回错误代码（枚举类型）。
    //      必须在开始数据采集之前提交参数。
    //      Picam_Acquire()数据采集成功的条件是:
    //      •连续读出之间的延迟不超过readout_time_out，并且 •没有错误发生。
    //      PICAM_API Picam_Acquire(PicamHandle camera,
    //                              pi64s readout_count,  //希望读出的数量
    //                              piint readout_time_out, //每次连续读出之间的等待时长(ms)，必须在这个时限之内完成一帧的采集。-1表示无限长。
    //                              PicamAvailableData* available, //硬件数据的输出缓冲区。拍摄的readout_time_out张图片在其中按顺序存储，每张图片的长度为ReadoutStride。此缓冲区中的数据在以下情况有效: •启动下一个采集周期;或 •硬件关闭
    //                              PicamAcquisitionErrorsMask* AcqErrorMask); //存储在数据采集期间引发的任何错误消息

    // 一、如果Picam_Acquire()执行完毕没有任何错误
    if( (Acquire_Error == PicamError_None) && (AcqErrorMask == PicamAcquisitionErrorsMask_None) )
    {
        qDebug()<<"数据采集函数Picam_Acquire()执行成功！";
        qDebug()<<"采集到的数据起始地址：m_data.initial_readout = "<<m_data.initial_readout;
        qDebug()<<"采集到的数据帧数：m_data.readout_count = "<<m_data.readout_count;

        //检验本次采集的数据是否为空
        if(m_data.readout_count == 0)   //用StopAcquire()手动退出采集时， m_data.readout_count 为0
        {
            QString ErrorMessage = "本次曝光相机未采集到数据";
            emit CC_Report_Info(ErrorMessage);
            qDebug()<< ErrorMessage;
            return false;
        }
        else    //如果相机采集的数据>0帧, 则把数据写入新文件
        {

            // 1.构建存储tif文件的文件夹路径
            QString targetDir = get_DataSaveDirPath() + m_SavePath_tif + CurrentDay;

            // 2.检查目标目录是否存在，若没有就新建
            QDir dir;
            if (!dir.exists(targetDir))  //如果不存在
            {
                if (!dir.mkpath(targetDir))  //新建文件夹
                {
                    QString ErrorMessage = "函数CameraController::ShootOneImage()在自动新建文件夹:" + targetDir + "时失败！";
                                           emit CC_Report_Error(ErrorMessage);
                    qDebug()<< ErrorMessage;
                    return false;
                }
            }

            // 3.构建文件路径、QFile文件对象、把数据写入文件
            QString DataFileFullName = targetDir+"/"+ FileNamingInfo + ".tif";
            QFile file(DataFileFullName);
            if(!file.open(QIODevice::WriteOnly))
            {
                //如果打开空的tif文件失败
                QString ErrorMessage = "无法打开用于存储数据的tif文件!\n错误信息：" + file.errorString();
                emit CC_Report_Error(ErrorMessage);
                qDebug() << ErrorMessage;
                return false;
            }
            else  //如果打开tif文件成功
            {
                //step1: 将m_data缓冲区的数据写入cv::Mat对象中
                cv::Mat img(m_ImageHeight/get_XYbinning(), m_ImageWidth/get_XYbinning(), CV_16UC1, m_data.initial_readout);

                //step2: 设置TIFF文件的压缩参数
                std::vector<int> Tiff_CompressionParams;
                Tiff_CompressionParams.push_back(cv::IMWRITE_TIFF_COMPRESSION);  //表示接下来的值是 TIFF 文件压缩的设置
                Tiff_CompressionParams.push_back(1); // 无压缩

                //step3: 将Mat对象中的数据写入TIFF文件
                if(cv::imwrite(DataFileFullName.toStdString(), img, Tiff_CompressionParams))
                {
                    QString Message = "相机所采数据成功写入tif文件!";
                    emit CC_Report_Info(Message);
                    qDebug() << Message;

                    //关闭文件
                    file.close();

                    //通知UI把最新一张tif加载到显示图片的子窗口
                    emit CC_Report_DisplayLastImage(DataFileFullName);

                    return true;
                }
                else
                {
                    QString ErrorMessage = "相机所采数据写入tif文件失败!";
                    emit CC_Report_Error(ErrorMessage);
                    qDebug() << ErrorMessage;
                    file.close();
                    return false;
                }

            }
        }

    }
    else if((Acquire_Error != PicamError_None) && (AcqErrorMask == PicamAcquisitionErrorsMask_None))
    {
        //二、如果仅 Acquire_Error 有错误

        const pichar* charArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, Acquire_Error, &charArray_ptr );
        QString ErrorMessage = "相机采集数据失败！\n错误信息：PicamError = " + QString::fromUtf8(charArray_ptr);
        qDebug() << ErrorMessage ;
        emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
        Picam_DestroyString( charArray_ptr );
        return false;
    }
    else if((Acquire_Error == PicamError_None) && (AcqErrorMask != PicamAcquisitionErrorsMask_None))
    {
        //三、如果仅 AcqErrorMask 有错误

        const pichar* charArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_AcquisitionErrorsMask, AcqErrorMask, &charArray_ptr );
        QString ErrorMessage = "相机采集数据失败！\n错误信息：PicamAcquisitionErrorsMask = " + QString::fromUtf8(charArray_ptr);
        qDebug() << ErrorMessage ;
        emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
        Picam_DestroyString( charArray_ptr );
        return false;
    }
    else
    {
        //四、如果 Acquire_Error 和 AcqErrorMask 都有错误

        const pichar* charArray_ptr, *charArray_ptr_2;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, Acquire_Error, &charArray_ptr );
        Picam_GetEnumerationString( PicamEnumeratedType_AcquisitionErrorsMask, AcqErrorMask, &charArray_ptr_2 );
        QString ErrorMessage = "相机采集数据失败！\n错误信息：PicamError = " + QString::fromUtf8(charArray_ptr) +
                               ".  PicamAcquisitionErrorsMask = "+ QString::fromUtf8(charArray_ptr_2);
        qDebug() << ErrorMessage ;
        emit CC_Report_Error(ErrorMessage); //通知UI出错及错误信息
        Picam_DestroyString( charArray_ptr );
        Picam_DestroyString( charArray_ptr_2 );
        return false;
    }
}



//把raw转成tif文件,再把raw删掉(FileName是不含路径、不含后缀的文件名！)
bool CameraController::SaveDataToTiff(QString FileName, QString SavePath_raw, QString SavePath_tif)
{

    //自动观测时，无论是否跨天，文件一律存在开始日期对应的文件夹内
    QString CurrentDay;
    if(get_isWorkingUnderAOC())
        CurrentDay = get_CurWorkingPeriodStartDate().toString("yyyyMMdd");    //获取当前工作时段的起始日期
    else
        CurrentDay = currentDateTime_UorL(get_UsedTimeType()).date().toString("yyyyMMdd");  //获取当前日期

    QString targetDir = SavePath_raw + CurrentDay;

    // 检查目标目录是否存在，若没有就新建
    QDir dir;
    if (!dir.exists(targetDir))  //如果不存在
    {
        if (!dir.mkpath(targetDir))
        {
            QString ErrorMessage = "函数CameraController::SaveDataToTiff()自动新建文件夹:" + targetDir + "失败！";
                                   emit CC_Report_Error(ErrorMessage);
            qDebug()<< ErrorMessage;
            return false;
        }
    }

    // 构建文件路径
    QString rawFilePath = targetDir + "/" + FileName + ".raw";     //要精确到文件名
    QString tifFilePath = targetDir + "/" + FileName + ".tif";     //要精确到文件名

    // 打开.raw文件
    QFile file(rawFilePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug()<< "无法打开raw文件! file.errorString = " << file.errorString();
        return false;
    }
    // 读取文件数据到QByteArray
    QByteArray fileData = file.readAll();
    file.close();



    // 将QByteArray的数据转换为OpenCV的Mat对象
    cv::Mat img(m_ImageHeight/get_XYbinning(), m_ImageWidth/get_XYbinning(), CV_16UC1, fileData.data());

    // 将Mat对象保存为.tif文件
    std::vector<int> Tiff_CompressionParams;
    Tiff_CompressionParams.push_back(cv::IMWRITE_TIFF_COMPRESSION);
    Tiff_CompressionParams.push_back(1); // 无压缩
    if (!cv::imwrite(tifFilePath.toStdString(), img, Tiff_CompressionParams))
    {
        qDebug() << "图像数据写入tif 文件失败!";
        return false;
    }

    //删除相应的raw文件
    QByteArray byteArray_rawPath = rawFilePath.toUtf8();
    const char* rawFilePath_charptr = byteArray_rawPath.constData();
    if ( !(std::remove(rawFilePath_charptr) == 0) )    //std::remove如果成功返回0，失败返回“EOF”（ -1）。
    {
        qDebug()<< "raw文件"+ rawFilePath +"删除失败！";
    }

    //通知UI把最新一张tif加载到显示图片的子窗口
    emit CC_Report_DisplayLastImage(tifFilePath);

    qDebug()<<"照片数据成功写入tif文件.";
    qDebug()<<"完整的 tifFilePath = " << tifFilePath;
    return true;


}


//保存拍摄的数据为.png文件(FileName:不含后缀的文件名)
bool CameraController::SaveDataToPng(QString FileName, QString SavePath_raw, QString SavePath_png)
{
    //自动观测时，无论是否跨天，文件一律存在开始日期对应的文件夹内
    QString CurrentDay;
    if(get_isWorkingUnderAOC())
        CurrentDay = get_CurWorkingPeriodStartDate().toString("yyyyMMdd");    //获取当前工作时段的起始日期
    else
        CurrentDay = currentDateTime_UorL(get_UsedTimeType()).date().toString("yyyyMMdd");  //获取当前日期

    QString targetDir = SavePath_raw + CurrentDay;

    // 检查目标目录是否存在，若没有就新建
    QDir dir;
    if (!dir.exists(targetDir))  //如果不存在
    {
        if (!dir.mkpath(targetDir))
        {
            QString ErrorMessage = "函数CameraController::SaveDataToTiff()自动新建文件夹:" + targetDir + "失败！";
                                   emit CC_Report_Error(ErrorMessage);
            qDebug()<< ErrorMessage;
            return false;
        }
    }

    // 构建文件路径
    QString rawFilePath = targetDir + "/" + FileName + ".raw";     //要精确到文件名
    QString pngFilePath = targetDir + "/" + FileName + ".png";     //要精确到文件名

    // 打开.raw文件
    QFile file(rawFilePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug()<< "无法打开raw文件! file.errorString = " << file.errorString();
        return false;
    }
    // 读取文件数据到QByteArray
    QByteArray fileData = file.readAll();
    file.close();


    // 将QByteArray的数据转换为OpenCV的Mat对象
    cv::Mat img(m_ImageHeight/get_XYbinning(), m_ImageWidth/get_XYbinning(), CV_16UC1, fileData.data());

    // 将Mat对象保存为.png文件
    std::vector<int> PNG_CompressionParams;   //PNG文件的压缩参数
    PNG_CompressionParams.push_back(cv::IMWRITE_PNG_COMPRESSION);  //cv::IMWRITE_PNG_COMPRESSION: OpenCV常量,用于指定PNG文件的压缩参数。
    PNG_CompressionParams.push_back(0); //PNG文件的压缩级别：0~9，值越大压缩越多
    if (!cv::imwrite(pngFilePath.toStdString(), img, PNG_CompressionParams))
    {
        QString ErrorMessage = "图像数据写入png文件失败!期望写入的文件路径是: "+pngFilePath;
        emit CC_Report_Error(ErrorMessage);
        qDebug() << ErrorMessage;
        return false;
    }

    // //删除相应的raw文件
    // QByteArray byteArray_rawPath = rawFilePath.toUtf8();
    // const char* rawFilePath_charptr = byteArray_rawPath.constData();
    // if ( !(std::remove(rawFilePath_charptr) == 0) )    //std::remove如果成功返回0，失败返回“EOF”（ -1）。
    // {
    //     qDebug()<< "raw文件"+ rawFilePath +"删除失败！";
    // }
    // qDebug()<<"照片数据成功写入png文件.";
    // qDebug()<<"完整的 pngFilePath = " << pngFilePath;
    return true;
}


//#####################################################################
//#################### CameraController的主工作函数 #####################
//#####################################################################
void CameraController::CameraControllerMain()
{
    //子线程的事件循环默认是开启的
    qDebug() << "进入CameraController子线程，子线程对象地址: " << QThread::currentThread();
    //创建并开启定时器
    m_TimerPtr = new QTimer; //创建CC专属定时器
    m_TimerPtr->setInterval(2000);//每50ms超时一次
    connect(m_TimerPtr, SIGNAL(timeout()), this, SLOT(onTimeout()));
    m_TimerPtr->start(); //启动定时器
}


//CC设置相机xy方向的像素合并值
void CameraController::CCSetCameraXYBinning(int xyBinning)
{
    PicamError err;                          // 错误码
    const PicamRois *region;                 // 多个感兴趣区域（ROIs）
    const PicamParameter *paramsFailed;      // 提交失败的参数
    piint paramsFailedCount;                 // 提交失败的参数数目

    //获取原始ROI
    err = Picam_GetParameterRoisValue(m_CurCamera_Handle, PicamParameter_Rois, &region);

    if (err == PicamError_None)
    {
        //修改ROI
        if (region->roi_count == 1)
        {
            //ROI在芯片上的偏移依旧是0
            region->roi_array[0].x = 0;
            region->roi_array[0].y = 0;

            //ROI的绝对大小保持全底片大小
            region->roi_array[0].width = m_ImageWidth;
            region->roi_array[0].height = m_ImageHeight;

            //垂直和水平方向分别几个像素一合并
            region->roi_array[0].x_binning = xyBinning;
            region->roi_array[0].y_binning = xyBinning;
        }
        else
        {
            QString ErrorStr = "修改像素合并失败！具体是因为ROI数量不等于1。";
            emit CC_Report_Error(ErrorStr);
            qDebug()<<ErrorStr;
        }


        //set新的ROI
        err = Picam_SetParameterRoisValue(m_CurCamera_Handle, PicamParameter_Rois, region);

        if (err == PicamError_None)
        {
            // 提交ROI参数到硬件
            err = Picam_CommitParameters(m_CurCamera_Handle, &paramsFailed, &paramsFailedCount);
            Picam_DestroyParameters(paramsFailed);

            if (err == PicamError_None)
            {
                //设置并commit像素合并参数 PicamRois 成功
                QString Message = "设置像素合并值成功！当前Binning = "+ QString::number(get_XYbinning())+"x"+ QString::number(get_XYbinning());
                emit CC_Report_Info(Message);
                qDebug()<<Message;
            }
            else
            {
                const pichar* charArray_ptr;
                Picam_GetEnumerationString( PicamEnumeratedType_Error, err, &charArray_ptr );
                QString ErrorStr = "修改像素合并失败！具体是在Picam_CommitParameters()时出错，PicamError = " + QString::fromUtf8(charArray_ptr);
                                   emit CC_Report_Error(ErrorStr);
                qDebug()<<ErrorStr;
            }
        }
        else
        {
            const pichar* charArray_ptr;
            Picam_GetEnumerationString( PicamEnumeratedType_Error, err, &charArray_ptr );
            QString ErrorStr = "修改像素合并失败！具体是在Picam_SetParameterRoisValue()时出错，PicamError = " + QString::fromUtf8(charArray_ptr);
                               emit CC_Report_Error(ErrorStr);
            qDebug()<<ErrorStr;
        }
    }
    else
    {
        const pichar* charArray_ptr;
        Picam_GetEnumerationString( PicamEnumeratedType_Error, err, &charArray_ptr );
        QString ErrorStr = "修改像素合并失败！具体是在Picam_GetParameterRoisValue()时出错，PicamError = " + QString::fromUtf8(charArray_ptr);
                           emit CC_Report_Error(ErrorStr);
        qDebug()<<ErrorStr;
    }
    //释放区域
    Picam_DestroyRois(region);



}



//立即退出Picam_Acquire()函数
void CameraController::StopAcquireImmediately()
{
    Picam_StopAcquisition(m_CurCamera_Handle);
    //  Picam_StopAcquisition()异步请求正在进行的数据采集停止并立即返回。
    //  如果成功，数据采集将在该函数返回后的某个时间停止运行。
    //  当Picam_IsAcquitionRunning()返回FALSE或Picam_AcquitionStatus.running为FALSE
}



//slots: 处理 连接相机
void CameraController::CC_Respond_ConnectedCamera()
{
    if(!InitializeAndOpenCamera()) //Init并连接相机
    {
        emit CC_Report_Error("相机连接失败");
        return;
    }
    set_ExposureTime(m_ExposureTimeAtBeginning);    //设置初始曝光时间s
    set_ShuttingTimingMode( m_ShutterTimingMode );  //设置快门模式
    CCSetCameraXYBinning(get_XYbinning());          //设置像素合并
}

//slots: 处理 断开相机
void CameraController::CC_Respond_DisConnectedCamera()
{
    if(!UnInitializeAndCloseCamera()) //UnInit并且关闭相机
        CC_Report_Error("相机关闭失败");
}




//slots: 处理 AOC设置目标温度
void CameraController::CC_Respond_OperateCameraTemperature(float TargetTemperature)
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        emit CC_Report_PauseAutoObserve();
        return;
    }

    if(set_TargetSensorTemperature(TargetTemperature)) //如果 设置目标温度成功
    {
        QThread::msleep(50);
        while(1)  //等待到达目标温度
        {
            QCoreApplication::processEvents();
            QThread::msleep(200);
            //if( (abs(get_CurSensorTemperature() - TargetTemperature) < 0.5) )
            if( (abs(get_CurSensorTemperature() - TargetTemperature) < 0.5) )
            {
                emit CC_Report_OperateCameraTemperature_Finish();    //到达目标温度, 模式中的动作执行成功，通知AOC线程可以继续下一动作了
                break;
            }
        }
    }
    else  //如果设置目标温度失败，就睡眠1s后再试一次
    {
        QThread::msleep(1000);
        if( set_TargetSensorTemperature(TargetTemperature) )
        {
            QThread::msleep(50);
            while(1)  //等待到达目标温度
            {
                QCoreApplication::processEvents();
                QThread::msleep(200);
                //if( (abs(get_CurSensorTemperature() - TargetTemperature) < 0.5) )
                if( (abs(get_CurSensorTemperature() - TargetTemperature) < 0.5) )
                {
                    emit CC_Report_OperateCameraTemperature_Finish();    //到达目标温度, 模式中的动作执行成功，通知AOC线程可以继续下一动作了
                    break;
                }
            }
        }
        else    //如果第二次设置温度依旧不成功，就阻塞AOC
        {
            emit CC_Report_Error("第二次设置目标温度依旧失败！期望TargetTemperature ="+QString::number(TargetTemperature));
            emit CC_Report_PauseAutoObserve();  //模式中的动作执行失败，中止自动观测(阻塞AOC线程)
        }

    }
}

//slots: 处理 AOC设置快门模式
void CameraController::CC_Respond_OperateCameraShutterTimingMode(int ShutterTimingMode)
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        emit CC_Report_PauseAutoObserve();
        return;
    }

    if( set_ShuttingTimingMode(PicamShutterTimingMode(ShutterTimingMode+1)) )
    {
        QThread::msleep(500);
        emit CC_Report_OperateCameraShutterTimingMode_Finish();    //模式中的动作执行成功，通知AOC线程可以继续下一动作了
    }
    else    //如果第一次失败，就睡眠1s后再试一次
    {
        QThread::msleep(1000);
        if( set_ShuttingTimingMode(PicamShutterTimingMode(ShutterTimingMode+1)) )
        {
            QThread::msleep(500);
            emit CC_Report_OperateCameraShutterTimingMode_Finish();
        }
        else    //如果第二次依旧不成功，就阻塞AOC
        {
            emit CC_Report_Error("第二次设置快门模式依旧失败！期望ShutterTimingMode编号 ="+QString::number(ShutterTimingMode+1));
            emit CC_Report_PauseAutoObserve();  //模式中的动作执行失败，中止自动观测(阻塞AOC线程)
        }

    }
}

 //slots: 处理 AOC设置曝光时间
void CameraController::CC_Respond_OperateCameraExposureTime(float ExposureTime)
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        emit CC_Report_PauseAutoObserve();
        return;
    }

    if(set_ExposureTime(ExposureTime)) //设置曝光时间
    {
        QThread::msleep(500);
        emit CC_Report_OperateCameraExposureTime_Finish();  //模式中的动作执行成功，通知AOC线程可以继续下一动作了
    }
    else//如果第一次失败，就睡眠1s后再试一次
    {
        QThread::msleep(1000);
        if(set_ExposureTime(ExposureTime))
        {
            QThread::msleep(500);
            emit CC_Report_OperateCameraExposureTime_Finish();  //模式中的动作执行成功，通知AOC线程可以继续下一动作了
        }
        else
        {
            emit CC_Report_Error("第二次设置曝光时间依旧失败！期望ExposureTime ="+QString::number(ExposureTime));
            emit CC_Report_PauseAutoObserve();  //模式中的动作执行失败，中止自动观测(阻塞AOC线程)
        }
    }

}

 //slots: 处理 AOC拍摄单张照片
void CameraController::CC_Respond_OperateCameraAcquireAndSave(QString FileNamingInfo, float CurExposureTime)
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        emit CC_Report_PauseAutoObserve();
        return;
    }

    m_ExposureTime = CurExposureTime;
    set_isWorkingUnderAOC(true);  //明确当前是手动模式下执行的拍摄

    if( ShootOneImage(FileNamingInfo) ) //调用单张拍摄函数
    {
        QThread::msleep(500);
        emit CC_Report_OperateCameraAcquireAndSave_Finish();  //模式中的动作执行成功，通知AOC线程可以继续下一动作了
    }
    else
    {
        // emit CC_Report_Error("两次执行Picam_Acquire()均未成功，CC_Respond_OperateCameraAcquireAndSave()执行失败！");
        // emit CC_Report_PauseAutoObserve();  //模式中的动作执行失败，中止自动观测(阻塞AOC线程)
        emit CC_Report_OperateCameraAcquireAndSave_Finish();  //二次拍摄不成功也通知AOC继续执行下一观测动作
    }
}


//slots: 处理 AOC控制相机连接/断开
void CameraController::CC_Respond_OperateCameraConnect(bool ConnectORDisconnect)
{
    if(ConnectORDisconnect)
    {
        CC_Respond_ConnectedCamera();
        emit CC_Report_OperateCameraConnect_Finish();    //模式中的动作执行成功，通知AOC线程可以继续下一动作了
    }
    else
    {
        CC_Respond_DisConnectedCamera();
        emit CC_Report_OperateCameraConnect_Finish();    //模式中的动作执行成功，通知AOC线程可以继续下一动作了
    }
}

//slots: 处理 设置目标温度
void CameraController::CC_Respond_SetTargetTemperature(float TargetTemperature )
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        return;
    }
    set_TargetSensorTemperature(TargetTemperature);
}


//处理 设置快门模式，1自动、2常闭、3常开、4 OpenBeforeTrigger
void CameraController::CC_Respond_SetShutterTimingMode(int ShutterTimingMode)
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        return;
    }
    set_ShuttingTimingMode( PicamShutterTimingMode(ShutterTimingMode+1) );  //int->enum强制类型转换（接收ComBox的index是从0开始的，所以要+1）
}

//slots: 处理 设置曝光时间
void CameraController::CC_Respond_SetExposureTime(float ExposureTime)
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        return;
    }
    set_ExposureTime(ExposureTime); //设置曝光时间
}

//slots: 手动拍摄单张照片
void CameraController::CC_Respond_ShootOneImage(QString FileNamingInfo)
{
    //若相机未连接，则警告并退出
    if(!get_CameraIsConnected(m_CurCamera_id))
    {
        emit CC_Report_Error("相机未连接！请连接后再操作相机。");
        return;
    }
    set_isWorkingUnderAOC(false);  //明确当前是手动模式下执行的拍摄
    ShootOneImage(FileNamingInfo); //调用单张拍摄函数
}

//CCTimer超时后执行
void CameraController::onTimeout()
{
    if(m_CameraIsConnected) //如果相机连接着，就更新相机状态信息
    {
        emit CC_Report_NewCurSensorTemperature(get_CurSensorTemperature()); //通知UI更新相机当前温度

    }

}
