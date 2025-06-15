#pragma execution_character_set("utf-8")

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QSerialPort>
#include "pathutils.h"
#include "Serialport_listener.h"
#include "FilterController.h"
#include "CameraController.h"
#include "AutoObserveController.h"
#include "LastImageDisplayWindow.h"
#include <QTimer>
#include <QFileDialog>  //提供文件选择对话框
#include <QFileInfo>
#include <QButtonGroup> //给radioBtn分组
#include <QCloseEvent>  //在关闭主窗口时执行一些操作然后才退出程序
#include <QShortcut>    //提供快捷键支持

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void ReadAndShow_FilterParamtxt();  //读取filterParam.txt文件，显示相关参数到UI

    void Do_LinkageControl();  //UI控制开始执行联动控制

private:

    void closeEvent(QCloseEvent *event);  //重写closeEvent函数，在退出程序前执行某些特定操作

    //滤波轮各滤波片的孔位Index
    int index_750850_window = 0;
    int index_862_window = 1;
    int index_630_window = 2;
    int index_000_window = 3;


    //相关设置文件名和存储路径
    QString ROOT_DIR_window = PathUtils::getRootPath(); //项目目录
    QString SETTING_DIR_window = "/Setting";  //设置文件存放位置路径
    QString FILTER_FILENAME_window = "/FilterParam.txt"; //滤波轮设置文件名
    QString TIMETABLE_DIR_window = "/Setting"; //时间表存放路径
    QString TIMETABLE_FILENAME_window = "/Timetable_UTC.txt"; //时间表文件名
    QString TIMETABLE_FULLPATH_window ; //时间表文件的完整路径
    QString OBSERVEMODE_FULLPATH_window ; //观测模式定义文件的完整路径
    QString LINKAGECONTROL_DIRPATH_window ; //联动控制文件的文件夹路径


    char relay_1_Open = 2;  //继电器1是否开启的标志,0:关闭, 1:开启, 2:未知
    char relay_2_Open = 2;  //继电器2是否开启的标志,0:关闭, 1:开启, 2:未知
    char relay_3_Open = 2;
    char relay_4_Open = 2;
    char relay_5_Open = 2;
    char relay_6_Open = 2;
    char relay_7_Open = 2;
    char relay_8_Open = 2;

    bool AOCThread_pause = false;  //UI中记录AOC线程是否暂停的falg

    int UI_CurSlotIndex = 0 ;
    float UI_CameraSensorTemperature = 0;
    float UI_CameraExposureTime = 0;  //ms
    int UI_CameraShuttingTimeMode = 0; //矫正过了，范围=1，2，3


    int UI_Timerout_period = 100;    //UI专属定时器的超时周期
    int UI_LinkageControlFile_HowManyTimesReadOnce = 50;  //定时器超时多少次，才读一次联动控制文件
    int UI_LinkageControlFile_AlreadyTimeoutHowManyTimes = 0;  //定时器已经超时多少次

    bool UI_IfLinkageControlOpened = false;         //联动控制是否开启
    bool UI_IfCameraIsAcquiring = false;            //相机是否正在采集


    Ui::MainWindow *ui;
    QSerialPort mySerialPort; //电源串口对象
    LastImageDisplayWindow* m_LastImageFigure;  //拍摄到的图片显示窗口
    QShortcut *m_ShortCut_Ctrl_W;     //快捷键“Ctrl+W”：开启/关闭联动控制
    QDateTime m_AcquireProgressBar_StartTime;  //拍摄进度条的其实计时时间

    QTimer* UI_TimerPtr; //UI的专属定时器的指针


private slots:
    void on_pB_ParameterPreset_clicked();

    void on_pB_ObservModePerset_clicked();

    void on_pB_ManualOperate_clicked();

    void on_pB_Automatic_clicked();

    void on_btn_addMode_clicked();

    void on_btn_deleteMode_clicked();

    void on_btn_ActionAdd_clicked();

    void on_btn_ActionRemove_clicked();

    void on_btn_UpMode_clicked();

    void on_btn_DownMode_clicked();

    void on_btn_ActionUp_clicked();

    void on_btn_ActionDown_clicked();

    void on_btn_PowerConnect_clicked();

    void on_btn_PowerDisconnect_clicked();

    void do_com_readyRead(); //当串口缓冲区有数据时，处理

    void on_btn_MotorConnect_clicked();

    void on_btn_MotorDisconnect_clicked();

    void on_btn_CameraConnect_clicked();

    void on_btn_CameraDisconnect_clicked();

    void on_lineEdit_SetTargetTemperature_returnPressed();

    void on_lineEdit_SetExposureTime_returnPressed();

    void on_btn_on_1_clicked();

    void on_btn_StartAutoObserve_clicked();

    void on_btn_PauseAutoObserve_clicked();

    void on_btn_TerminateAutoObserve_clicked();

    void UI_onTimeout();  //UI定时器超时处理函数

    void on_btn_ShootOneImage_clicked();

    void on_pushButton_2_clicked();

    void on_btn_SelectTimeTable_clicked();

    void on_btn_on_all_clicked();

    void on_btn_off_all_clicked();

    void on_btn_on_2_clicked();

    void on_btn_on_3_clicked();

    void on_btn_on_4_clicked();

    void on_btn_on_5_clicked();

    void on_btn_on_6_clicked();

    void on_btn_on_7_clicked();

    void on_btn_on_8_clicked();

    void on_btn_off_1_clicked();

    void on_btn_off_2_clicked();

    void on_btn_off_3_clicked();

    void on_btn_off_4_clicked();

    void on_btn_off_5_clicked();

    void on_btn_off_6_clicked();

    void on_btn_off_7_clicked();

    void on_btn_off_8_clicked();

    void on_btn_ClearAllMessage_clicked();

    void on_comboBox_changeSlotTo_textActivated(const QString &arg1);

    void on_checkBox_IfShowLIDW_2_stateChanged(int arg1);

    void on_checkBox_IfShowLIDW_stateChanged(int arg1);

    void on_checkBox_UseUTC_stateChanged(int arg1);

    void on_checkBox_UseLocalTime_stateChanged(int arg1);

    void on_btn_SelectObserveMode_clicked();

    void on_radioButton_Binning1x1_toggled(bool checked);

    void on_radioButton_Binning2x2_toggled(bool checked);

    void on_radioButton_Binning4x4_toggled(bool checked);

    void on_radioButton_Binning8x8_toggled(bool checked);

signals:
    //UI控制FC、CC
    void UI_Send_ChangeSlotTo(int SlotIndex);   //UI通知FC将滤波轮切换至索引为SlotIndex的slot
    void LineEditEnter_SetTargetTemperature(float TargetTemperature); //设定了新目标温度信号
    void LineEditEnter_SetExposureTime(float ExposureTime); //设定了新曝光时间信号
    void UI_Send_ShootOneImage(QString FileNamingInfo);     //UI指挥CC在手动模式下拍摄一张照片,FileNamingInfo: 文件名（不含后缀和路径）

    //void UI_Report_DataSaveDirectoryPath(QString DataSavePath);     //通知CC更新数据存储路径
    void UI_Report_TimeTableFullPath(QString DataSavePath);         //通知AOC更新时间表路径

    void UI_Report_StartAutoObserve();      //通知-自动观测启动
    void UI_Report_PauseAutoObserve();      //通知-自动观测暂停
    void UI_Report_ResumeAutoObserve();     //通知-自动观测继续
    void UI_Report_TerminateAutoObserve();  //通知-自动观测结束

    void UI_PSC_Report_OperatePowerSwitch_Finish();    //通知-在AOC指挥下控制电源 完成


public slots:

    void UI_Respond_AwakeSubThreadAOC();        //在UI线程中唤醒AOC线程，从而执行后面的观测动作

    void UI_Respond_FinishedAllWorkPeriod();    //AOC完成了所有的有效观测时段，通知UI更新“启动暂停结束btn” 并 退出AOC线程

    void UI_PSC_Respond_OperatePowerSwitch(int RelayIndex, char ONorOFF); //slots: 在AOC控制下，操纵继电器开启（ONorOFF==1）或关闭（ONorOFF==0），RelayIndex==-1表示全部继电器);

    void UI_Respond_DisplayLastImage(QString TifFileFullPath);    //CC通知UI在子窗口中加载最新一张拍到的图片

    void UI_Respond_LinkageControlONorOFF();     //快捷键被按下后，UI线程开启/关闭联动控制

    void UI_Respond_StartAcquireData(QDateTime StartAcquireTime);         //CC通知UI，相机开始采集数据

    void UI_Update_WhetherCameraIsConnected(bool IfConnected); //根据CC返回的相机是否连接信息，更新UI上的“连接”、“断开”按钮状态

    void UI_Update_CurSlotIndex(int slotIndex); //更新UI上的当前滤波片index
    void UI_UpdateCurSensorTemperature(float NewTemperature); //更新UI上相机传感器温度值
    void UI_UpdateCurExposureTime(float NewExposureTime); //更新UI上相机的当前曝光时间
    void UI_UpdateCurShuttingTimeMode(int ShuttingTimeMode);  //更新UI上相机的快门模式,形参范围0，1，2

    void UI_Update_CurWorkingStartAndEndTime(QString StartWorkingTime1,QString EndWorkingTime1,QString StartWorkingTime2,QString EndWorkingTime2);  //UI更新当前工作时段、下一工作时段
    void UI_Update_PauseBtnAfterAOCpaused();    //AOC阻塞后，UI上自动观测模式的“暂停”btn要切换显示“继续”，

    void UI_Show_FCError(QString ErrorInfo); //在UI上显示FC的错误信息
    void UI_Show_CCError(QString ErrorInfo); //在UI上显示CC的错误信息
    void UI_Show_CCInfo(QString Info); //在UI上显示CC的提示信息
    void UI_Show_MessageBoxOutput(QString OneMessage);    //UI在消息盒子上更新从AOC收到的信息

};

#endif // MAINWINDOW_H
