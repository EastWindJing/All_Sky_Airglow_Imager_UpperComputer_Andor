#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "actionedit.h"
#include "GlobalVariable.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThread>
#include "MessageTips.h"  //提供可以自动淡出最终消失的提示信息显示框


//-------------------------通信帧的关键字-------------------------
#define FRAMES_HEADER   "48"  //帧头
#define FRAMES_TAIL     "4d"    //帧尾
#define FACILITY_TYPE   "1a"  //设备类型
#define FACILITY_ID     "00"    //设备ID
#define STANDBY_OR_NOT  "01" //待机与否
#define PATTERN_CALL    "02"   //模式调用
#define PATTERN_SAVE    "03"   //模式保存
#define RELAY_ALL       "04"      //所有继电器全开/全关
#define RELAY_SINGAL    "05"   //单个继电器 开/关
#define BAUDRATE_SET    "06"   //波特率设置
#define LOCAL_ID        "07"       //本机ID设置
#define CASCADE_SWITCH  "08" //级联开关
#define INITIAL         "09"        //初始化
#define ON              "01"             //开
#define OFF             "00"            //关
#define BLANK           "00"          //0x00

#define RELAY_1 "00"        //继电器1~8
#define RELAY_2 "01"
#define RELAY_3 "02"
#define RELAY_4 "03"
#define RELAY_5 "04"
#define RELAY_6 "05"
#define RELAY_7 "06"        //激光器插在7号通道
#define RELAY_8 "07"

#define GREEN 80,245,80        //绿色的RGB值
#define GRAY 240,240,240       //灰色的RGB值

#define SLEEP_DURATION 100     //操作电源时给的延时余量ms(根据实际测试来看至少20ms)
//--------------------------宏的定义 结束-----------------------------



//***************定义全局变量***************
SerialPort_Listener *mySerialPort_Listener = Q_NULLPTR;

FilterController* myFilterController = new FilterController;
QThread* subThread_FilterController = new QThread; //滤波轮子线程（子线程的事件循环默认是开启的）

CameraController* myCameraController = new CameraController;
QThread* subThread_CameraController = new QThread; //相机子线程（子线程的事件循环默认是开启的）

AutoObserveController* myAutoObserveController = new AutoObserveController;
QThread* subThread_AutoObserveController = new QThread; //自动观测控制器子线程（子线程的事件循环默认是开启的）


//MainWindow构造函数
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //add

    //this->setWindowFlags(Qt::FramelessWindowHint);  //去除默认边框
    //this->setAttribute(Qt::WA_TranslucentBackground);
    //this->setWindowOpacity(0.98);  //设置透明度

    qDebug() << "主线程对象的地址: " << QThread::currentThread();

    m_ShortCut_Ctrl_W = new QShortcut(QKeySequence("Ctrl+W"), this);  //创建快捷键"Ctrl+W"



    connect(&mySerialPort, &QIODevice::readyRead, this, &MainWindow::do_com_readyRead);//连接串口的readRead信号和对应槽函数

    //----------------------------------------------------UI--FilterController------------------------------------------------------------------------//
    connect(ui->btn_MotorConnect, &QPushButton::clicked, myFilterController, &FilterController::FilterControllerMain);//连接上滤波轮后，进入滤波轮子线程
    connect(myFilterController, &FilterController::FC_Report_Error, this, &MainWindow::UI_Show_FCError); //把FC的错误信息在UI上弹窗输出
    connect(myFilterController, &FilterController::FC_Report_NewCurSlotIndex, this, &MainWindow::UI_Update_CurSlotIndex);//更新UI上的当前滤波片index
    connect(this, &MainWindow::UI_Send_ChangeSlotTo, myFilterController, &FilterController::FC_Respond_ChangeSlotTo);

    //----------------------------------------------------UI--CameraController------------------------------------------------------------------------//
    connect(ui->btn_CameraConnect, &QPushButton::clicked, myCameraController, &CameraController::CameraControllerMain);//连接上相机后，进入相机子线程
    connect(ui->btn_CameraConnect, &QPushButton::clicked, myCameraController, &CameraController::CC_Respond_ConnectedCamera); //连接相机
    connect(ui->btn_CameraDisconnect, &QPushButton::clicked, myCameraController, &CameraController::CC_Respond_DisConnectedCamera); //断开相机
    connect(this, &MainWindow::UI_Send_ShootOneImage, myCameraController, &CameraController::CC_Respond_ShootOneImage);//相机拍摄单张照片
    connect(this, &MainWindow::LineEditEnter_SetTargetTemperature, myCameraController, &CameraController::CC_Respond_SetTargetTemperature);
    connect(this, &MainWindow::LineEditEnter_SetExposureTime, myCameraController, &CameraController::CC_Respond_SetExposureTime);
    connect(ui->comboBox_SetShutterTimingMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            myCameraController, &CameraController::CC_Respond_SetShutterTimingMode);//设置快门模式
    //( QComboBox中的currentIndexChanged信号存在重载，在使用时，如果使用QT5新版的信号槽语法，须进行强制类型转换 )
    connect(myCameraController, &CameraController::CC_Report_Error, this, &MainWindow::UI_Show_CCError);
    connect(myCameraController, &CameraController::CC_Report_Info, this, &MainWindow::UI_Show_CCInfo);
    connect(myCameraController, &CameraController::CC_Report_WhetherCameraIsConnected, this, &MainWindow::UI_Update_WhetherCameraIsConnected );
    connect(myCameraController, &CameraController::CC_Report_NewCurSensorTemperature, this, &MainWindow::UI_UpdateCurSensorTemperature);//UI上更新相机温度
    connect(myCameraController, &CameraController::CC_Report_NewCurShutterTimingMode, this, &MainWindow::UI_UpdateCurShuttingTimeMode);
    connect(myCameraController, &CameraController::CC_Report_NewCurExposureTime, this, &MainWindow::UI_UpdateCurExposureTime); //UI上更新相机曝光时间
    connect(myCameraController, &CameraController::CC_Report_StartAcquireData, this, &MainWindow::UI_Respond_StartAcquireData);  //CC通知UI，相机开始采集数据

    //----------------------------------------------------UI--AutoObserveController---------------------------------------------------------------------------------------//
    connect(this, &MainWindow::UI_Report_StartAutoObserve, myAutoObserveController, &AutoObserveController::AutoObserveControllerMain);//subThread_AOC start()之后，进入AOC子线程
    connect(this, &MainWindow::UI_Report_PauseAutoObserve, myAutoObserveController, &AutoObserveController::AOC_Respond_PauseAutoObserve);  //UI通知AOC阻塞自身，暂停自动观测
    connect(myFilterController, &FilterController::FC_Report_PauseAutoObserve, myAutoObserveController, &AutoObserveController::AOC_Respond_PauseAutoObserve);//模式中的观测动作执行失败，就暂停自动观测
    connect(myCameraController, &CameraController::CC_Report_PauseAutoObserve, myAutoObserveController, &AutoObserveController::AOC_Respond_PauseAutoObserve);//模式中的观测动作执行失败，就暂停自动观测

    //AOC开始执行某一观测动作，连接到FC、CC、UI(PowerSwitchController)
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperatePowerSwitch, this, &MainWindow::UI_PSC_Respond_OperatePowerSwitch);
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperateFilter, myFilterController, &FilterController::FC_Respond_OperateFilter);
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperateCameraTemperature, myCameraController, &CameraController::CC_Respond_OperateCameraTemperature);
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperateCameraShutterTimingMode, myCameraController, &CameraController::CC_Respond_OperateCameraShutterTimingMode);
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperateCameraExposureTime, myCameraController, &CameraController::CC_Respond_OperateCameraExposureTime);
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperateCameraAcquireAndSave, myCameraController, &CameraController::CC_Respond_OperateCameraAcquireAndSave);
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperateCameraConnect, myCameraController, &CameraController::CC_Respond_OperateCameraConnect);


    //观测动作完成信号，连接到 ，AOC更新 动作完成标记槽函数
    connect(this,&MainWindow::UI_PSC_Report_OperatePowerSwitch_Finish, myAutoObserveController, &AutoObserveController::AOC_Update_IfOperatePowerSwitch_Finished);
    connect(myFilterController, &FilterController::FC_Report_OperateFilter_Finish, myAutoObserveController, &AutoObserveController::AOC_Update_IfOperateFilter_Finished);
    connect(myCameraController, &CameraController::CC_Report_OperateCameraTemperature_Finish, myAutoObserveController, &AutoObserveController::AOC_Update_IfOperateCameraTemperature_Finished);
    connect(myCameraController, &CameraController::CC_Report_OperateCameraShutterTimingMode_Finish, myAutoObserveController, &AutoObserveController::AOC_Update_IfOperateCameraShutterTimingMode_Finished);
    connect(myCameraController, &CameraController::CC_Report_OperateCameraExposureTime_Finish, myAutoObserveController, &AutoObserveController::AOC_Update_IfOperateCameraExposureTime_Finished);
    connect(myCameraController, &CameraController::CC_Report_OperateCameraAcquireAndSave_Finish, myAutoObserveController, &AutoObserveController::AOC_Update_IfOperateCameraAcquireAndSave_Finished);
    connect(myCameraController, &CameraController::CC_Report_OperateCameraConnect_Finish, myAutoObserveController, &AutoObserveController::AOC_Update_IfOperateCameraConnect_Finished);

    //其他
    //connect(myAutoObserveController, &AutoObserveController::AOC_Send_OperateCameraShutterTimingMode, this, &MainWindow::UI_UpdateCurShuttingTimeMode);
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_CurWorkingStartAndEndTime, this, &MainWindow::UI_Update_CurWorkingStartAndEndTime);
    //connect(this, &MainWindow::UI_Report_DataSaveDirectoryPath, myCameraController, &CameraController::CC_Update_DataSaveDirectoryPath);//UI通知CC更新数据保存路径
    //connect(this, &MainWindow::UI_Report_DataSaveDirectoryPath, myAutoObserveController, &AutoObserveController::AOC_Update_DataSaveDirectoryPath);//UI通知AOC更新数据保存路径
    connect(this, &MainWindow::UI_Report_TimeTableFullPath, myAutoObserveController, &AutoObserveController::AOC_Update_TimeTableFullPath);//UI通知AOC更新时间表路径
    connect(myAutoObserveController, &AutoObserveController::AOC_Send_MessageBoxOutput, this, &MainWindow::UI_Show_MessageBoxOutput);  //AOC向UI发送消息，更新在消息盒子上
    connect(myFilterController, &FilterController::FC_Report_PauseAutoObserve, this, &MainWindow::UI_Update_PauseBtnAfterAOCpaused);  //MC向AOC发出阻塞信号的时候也提醒UI将暂停按钮切换为“继续”
    connect(myCameraController, &CameraController::CC_Report_PauseAutoObserve, this, &MainWindow::UI_Update_PauseBtnAfterAOCpaused);  //CC向AOC发出阻塞信号的时候也提醒UI将暂停按钮切换为“继续”
    connect(myAutoObserveController, &AutoObserveController::AOC_Report_PauseAutoObserve, this, &MainWindow::UI_Update_PauseBtnAfterAOCpaused);  //AOC向AOC发出阻塞信号的时候也提醒UI将暂停按钮切换为“继续”
    connect(this, &MainWindow::UI_Report_PauseAutoObserve, this, &MainWindow::UI_Update_PauseBtnAfterAOCpaused);  //UI向AOC发出阻塞信号的时候也提醒UI将暂停按钮切换为“继续”

    connect(myAutoObserveController, &AutoObserveController::AOC_Report_FinishedAllWorkPeriod, this, &MainWindow::UI_Respond_FinishedAllWorkPeriod);

    connect(myCameraController, &CameraController::CC_Report_DisplayLastImage, this, &MainWindow::UI_Respond_DisplayLastImage);  //CC通知UI在子窗口中加载最新一张拍到的图片

    connect(m_ShortCut_Ctrl_W, &QShortcut::activated, this, &MainWindow::UI_Respond_LinkageControlONorOFF);


    //设置子线程
    myFilterController->moveToThread(subThread_FilterController);  //FilterController类的工作move到子线程1中
    subThread_FilterController->start();  //启动滤波轮子线程
    myCameraController->moveToThread(subThread_CameraController);  //CameraController类的工作move到子线程2中
    subThread_CameraController->start();  //启动相机子线程
    myAutoObserveController->moveToThread(subThread_AutoObserveController);  //AutoObserveController类的工作move到子线程3中
    //subThread_AutoObserveController->start(); //启动自动观测控制器子线程（搬到btn_StartAutoObserve的槽函数中了）

    //-----------------------------------------------------
    //-------------------1.初始化参数预设界面------------------
    //-----------------------------------------------------

    //隐藏“模式预设”按钮
    ui->pB_ObservModePerset->hide();

    //限制LineEdit的输入
    QRegExp regExp("^[1]|[2]|[3]|[4]$");
    QRegExpValidator *Filter_RegExpValidator = new QRegExpValidator(regExp, this);
    ui->lineEdit_750850->setValidator(Filter_RegExpValidator);
    ui->lineEdit_862->setValidator(Filter_RegExpValidator);
    ui->lineEdit_630->setValidator(Filter_RegExpValidator);
    ui->lineEdit_000->setValidator(Filter_RegExpValidator);


    QButtonGroup *UseTimeGroup = new QButtonGroup(this);
    UseTimeGroup->addButton(ui->checkBox_UseUTC,1);
    UseTimeGroup->addButton(ui->checkBox_UseLocalTime,2);
    if(get_UsedTimeType() == Local_Time)
        ui->checkBox_UseLocalTime->setCheckState(Qt::Checked);
    else if(get_UsedTimeType() == UTC_Time)
        ui->checkBox_UseUTC->setCheckState(Qt::Checked);

    ReadAndShow_FilterParamtxt(); //从paraMotor.txt读取电机预设参数，显示到UI
    myFilterController->read_FilterPreSetParam();//从FilterMotor.txt读取参数到myFilterController
    ui->lineEdit_DataStorePath->setPlaceholderText(tr("请选择数据存储路径……"));   
    // ui->lineEdit_TimeTablePath->setPlaceholderText(tr("请输入时间表存储路径……"));
    // ui->lineEdit_FileNamed->setPlaceholderText(tr("请设置文件保存时命名规则……"));
    ui->btn_PowerDisconnect->setEnabled(false);
    ui->btn_MotorDisconnect->setEnabled(false);
    ui->btn_CameraDisconnect->setEnabled(false);
    //ui->tabWidget_2->setStyleSheet("QTabWidget{background: transparent;}");

    //初始化像素合并选择框
    if(get_XYbinning()==1)
        ui->radioButton_Binning1x1->setChecked(true);
    else if(get_XYbinning()==2)
        ui->radioButton_Binning2x2->setChecked(true);
    else if(get_XYbinning()==4)
        ui->radioButton_Binning4x4->setChecked(true);
    else if(get_XYbinning()==8)
        ui->radioButton_Binning8x8->setChecked(true);
    //ui->groupBox_Binning->setEnabled(false);


    //初始化前置增益选择框
    ui->radioButton_Gain3->setChecked(true);
    //ui->groupBox_Gain->setEnabled(false);

    //检查默认路径的模式定义文件是否存在，若不存在就弹窗提示
    QFileInfo OMFileInfo(get_ObserveModeTXTFullPath());
    if(OMFileInfo.isFile())
        ui->lineEdit_ObserveModeFullPath->setText(get_ObserveModeTXTFullPath());
    else
        QMessageBox::warning(nullptr, "错误",("默认路径的观测模式文件：" + get_ObserveModeTXTFullPath() + " 不存在, 请重新选择观测模式。"  ));

    //检查默认的数据存储路径是否存在，若不存在则弹窗提示
    QFileInfo DataPathDir(get_DataSaveDirPath());
    if( DataPathDir.isDir() )
        ui->lineEdit_DataStorePath->setText(get_DataSaveDirPath());
    else
        QMessageBox::warning(nullptr, "错误",("默认的数据文件存储路径: " + get_DataSaveDirPath() + " 不存在, 请重新选择路径。") );

    //扫描计算机上的可用串口
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);
        if (serial.open(QIODevice::ReadWrite))
        {
            ui->comboBox_powerLink->addItem(serial.portName());
            //ui->comboBox_MotorLink->addItem(serial.portName());
            //ui->comboBox_CameraLink->addItem(serial.portName());
            serial.close();
        }
    }
    //（通过检测WM_DEVICECHANGE信号实现USB热插拔，实时更新可用的COM口）
    mySerialPort_Listener = new SerialPort_Listener;
    qApp->installNativeEventFilter(mySerialPort_Listener);
    //串口新增信号连接到槽
    connect(mySerialPort_Listener, &SerialPort_Listener::DevicePlugIn, [=](){
        //先清除之前的所有项
        ui->comboBox_powerLink->clear();
        //ui->comboBox_MotorLink->clear();
        //ui->comboBox_CameraLink->clear();
        //扫描可用串口，并添加
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
        {
            QSerialPort serial;
            serial.setPort(info);
            if (serial.open(QIODevice::ReadWrite))
            {
                ui->comboBox_powerLink->addItem(serial.portName());
                //ui->comboBox_MotorLink->addItem(serial.portName());
                //ui->comboBox_CameraLink->addItem(serial.portName());
                serial.close();
            }
        }
        qDebug() << "串口增加1个";
    });
    //串口减少信号连接到槽
    connect(mySerialPort_Listener, &SerialPort_Listener::DevicePlugOut, [=](){
        //先清除之前的所有项
        ui->comboBox_powerLink->clear();
        //ui->comboBox_MotorLink->clear();
        //ui->comboBox_CameraLink->clear();
        //扫描可用串口，并添加
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
        {
            QSerialPort serial;
            serial.setPort(info);
             if (serial.open(QIODevice::ReadWrite))
             {
                ui->comboBox_powerLink->addItem(serial.portName());
                //ui->comboBox_MotorLink->addItem(serial.portName());
                //ui->comboBox_CameraLink->addItem(serial.portName());
                serial.close();
            }
        }
        qDebug() << "串口减少1个";
    });


    //-----------------------------------------------------
    //-------------------2.初始化模式预设界面------------------
    //-----------------------------------------------------

    /*初始化模式列表tableWidget*/
    ui->tableWidget_Mode->setRowCount(3);
    ui->tableWidget_Mode->setColumnCount(2);
    ui->tableWidget_Mode->setAlternatingRowColors(true);//相邻两行不同色
    ui->tableWidget_Mode->setShowGrid(true);
    ui->tableWidget_Mode->horizontalHeader()->setStyleSheet("QHeaderView::section{background:#DDDDDD;color: black;}");//标题栏加背景色和边框
    ui->tableWidget_Mode->setSelectionBehavior(QAbstractItemView::SelectRows);//设置单击的选择方式-选择整行
    ui->tableWidget_Mode->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);//列宽自适应

    /*初始化动作列表1 tableWidget_section1*/
    ui->tableWidget_section1->setAlternatingRowColors(true);
    ui->tableWidget_section1->setShowGrid(true);
    ui->tableWidget_section1->horizontalHeader()->setStyleSheet("QHeaderView::section{background:#DDDDDD;color: black;}");
    ui->tableWidget_section1->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_section1->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    /*初始化动作列表2 tableWidget_section2*/
    ui->tableWidget_section2->setAlternatingRowColors(true);
    ui->tableWidget_section2->setShowGrid(true);
    ui->tableWidget_section2->horizontalHeader()->setStyleSheet("QHeaderView::section{background:#DDDDDD;color: black;}");
    ui->tableWidget_section2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_section2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    /*初始化动作列表3 tableWidget_section3*/
    ui->tableWidget_section3->setAlternatingRowColors(true);
    ui->tableWidget_section3->setShowGrid(true);
    ui->tableWidget_section3->horizontalHeader()->setStyleSheet("QHeaderView::section{background:#DDDDDD;color: black;}");
    ui->tableWidget_section3->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_section3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    //初始化电源待机按钮
    // ui->btn_standby->setIcon(QIcon(":/ResourceFiles/switch_on.ico")); // 初始状态为开
    // ui->btn_standby->setIconSize(QSize(50, 50));// 设置图标大小
    // ui->btn_standby->setFixedSize(50,50);     // 设置按钮大小
    // ui->btn_standby->setStyleSheet("border:none;background-color: rgb(255, 255, 255);");

    //初始化电源手控卡片
    ui->label_realyFlag_1->setStyleSheet("border:1px solid black;");
    ui->label_realyFlag_2->setStyleSheet("border:1px solid black;");
    ui->label_realyFlag_3->setStyleSheet("border:1px solid black;");
    ui->label_realyFlag_4->setStyleSheet("border:1px solid black;");
    ui->label_realyFlag_5->setStyleSheet("border:1px solid black;");
    ui->label_realyFlag_6->setStyleSheet("border:1px solid black;");
    ui->label_realyFlag_7->setStyleSheet("border:1px solid black;");
    ui->label_realyFlag_8->setStyleSheet("border:1px solid black;");
    //add ending

    //-----------------------------------------------------
    //-------------------3.初始化手动模式界面-------------------
    //-----------------------------------------------------


    ui->tabWidget_SmartPowerManual->setTabEnabled(0,false);     //电源手控卡片
    ui->tabWidget_Filter->setTabEnabled(0,false);               //滤波轮手控卡片
    ui->tabWidget_CameraManual->setTabEnabled(0,false);         //相机手控卡片
    ui->lineEdit_SetTargetTemperature->setPlaceholderText("输入目标温度后按Enter生效");
    ui->lineEdit_SetExposureTime->setPlaceholderText("输入新的曝光时间后按Enter生效");

    m_LastImageFigure = new LastImageDisplayWindow();  //创建子窗口-显示最近刚拍的一张图片（但不显示）

    //限制LineEdit的输入内容
    ui->lineEdit_SetTargetTemperature->setValidator(new QRegExpValidator(QRegExp("^-?(?:[0-9]|[1-9][0-9]|99)(?:\\.[0-9]{1,2})?$")));  //限制[-99,99] float,2位小数
    ui->lineEdit_SetExposureTime->setValidator(new QRegExpValidator(QRegExp("^([0-9]{1,5})(\\.[0-9]{1,5})?$")));  //限制[0,99999] float,5位小数


    ui->progressBar_ShootProgress->setValue(0);    //拍摄进度条置零
    ui->progressBar_ShootProgress->hide();         //隐藏拍摄进度条
    QSizePolicy sp_retain = ui->progressBar_ShootProgress->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    ui->progressBar_ShootProgress->setSizePolicy(sp_retain);  //设置拍摄进度条在隐藏后仍占位

    ui->progressBar_ShootProgress_2->setValue(0);    //拍摄进度条置零
    ui->progressBar_ShootProgress_2->hide();         //隐藏拍摄进度条
    QSizePolicy sp_retain_2 = ui->progressBar_ShootProgress_2->sizePolicy();
    sp_retain_2.setRetainSizeWhenHidden(true);
    ui->progressBar_ShootProgress_2->setSizePolicy(sp_retain_2);  //设置拍摄进度条在隐藏后仍占位

    //-----------------------------------------------------
    //-------------------4.初始化 自动观测界面-------------------
    //-----------------------------------------------------
    ui->btn_StartAutoObserve->setEnabled(true);
    ui->btn_PauseAutoObserve->setEnabled(false);
    ui->btn_TerminateAutoObserve->setEnabled(false);
    ui->lab_CurShuttingTimeMode->setText("自动");
    ui->lab_CurShuttingTimeMode_2->setText("自动");


    //检查默认路径的时间表是否存在，若不存在就弹窗提示
    TIMETABLE_FULLPATH_window = ROOT_DIR_window + TIMETABLE_DIR_window +TIMETABLE_FILENAME_window;
    QFileInfo fileInfo(TIMETABLE_FULLPATH_window);
    if(fileInfo.isFile())       //如果存在默认路径的时间表
        ui->lineEdit_TimeTableFullPath->setText(TIMETABLE_FULLPATH_window);
    else
        QMessageBox::warning(nullptr, "错误",("默认路径的时间表文件：" + TIMETABLE_FULLPATH_window + " 不存在, 请重新选择时间表。"  ));


    //设置MessageBoxOutupt字体样式
    QTextCharFormat charFormat; // 创建一个新的字符格式。
    charFormat.setForeground(QBrush(QColor(50, 205, 50)));
    ui->textEdit_MessageOutput->setCurrentCharFormat(charFormat);

    ui->lcdNumber_SensorTemperature->setMode(QLCDNumber::Dec);  //十进制显示
    // 设置显示外观
    ui->lcdNumber_SensorTemperature->setSegmentStyle(QLCDNumber::Flat);  //Outline 、 Filled  、 Flat
    // 设置样式
    ui->lcdNumber_SensorTemperature->setStyleSheet("border: none; background-color:white;  color: green;");

    //创建并开启UI定时器
    UI_TimerPtr = new QTimer; //创建UI专属定时器
    UI_TimerPtr->setInterval(UI_Timerout_period);//超时间隔
    connect(UI_TimerPtr, SIGNAL(timeout()), this, SLOT(UI_onTimeout()));
    UI_TimerPtr->start(); //启动定时器


}

MainWindow::~MainWindow()
{
    delete ui;
}

//读取FilterParam.txt文件，显示相关参数到UI、更新UI中存储的相关值
void MainWindow::ReadAndShow_FilterParamtxt()
{
    QString filePath = ROOT_DIR_window + SETTING_DIR_window + FILTER_FILENAME_window;
    QFile file(filePath); //创建QFile对象用于操作文件
    QFileInfo fileinfo(filePath);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))  //以读写和文本方式打开文件
    {
        QMessageBox::warning(nullptr, "错误",("无法打开文件：" + filePath + "\n错误信息：" + file.errorString()));
    }
    else
    {
        if(fileinfo.size() == 0)
        {
            QMessageBox::warning(nullptr, "错误", "文件： " + filePath + " 为空！请新建该文件，保存后重新启动上位机！" );
        }
        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            QString str(line);
            QStringList strList = str.split(" ");
            if (str.startsWith("750-850", Qt::CaseInsensitive))
                ui->lineEdit_750850->setText(QString::number(strList.at(1).toInt())), index_750850_window =strList.at(1).toInt()-1;
            else if(str.startsWith("862", Qt::CaseInsensitive))
                ui->lineEdit_862->setText(QString::number(strList.at(1).toInt())), index_862_window =strList.at(1).toInt()-1;
            else if(str.startsWith("630", Qt::CaseInsensitive))
                ui->lineEdit_630->setText(QString::number(strList.at(1).toInt())), index_630_window =strList.at(1).toInt()-1;
            else if(str.startsWith("000", Qt::CaseInsensitive))
                ui->lineEdit_000->setText(QString::number(strList.at(1).toInt())), index_000_window =strList.at(1).toInt()-1;
        }
        file.close();
    }

}



//UI控制开始执行联动控制
void MainWindow::Do_LinkageControl()
{


    //从 ASI_command_observation_control.txt 读取开启/停止自动观测的指令
    QString ASI_command_observation_controlTXTFullPath = LINKAGECONTROL_DIRPATH_window + "/ASI_command_observation_control.txt";
    QFile LinkageControlFile(ASI_command_observation_controlTXTFullPath);
    QFileInfo LinkageControl_FileInfo(ASI_command_observation_controlTXTFullPath);
    if(LinkageControl_FileInfo.isFile())  //检测文件ASI_command_observation_control.txt是否存在
    {
        if(LinkageControlFile.isOpen())  //若文件ASI_command_observation_control.txt已打开，则结束函数
        {
            return;
        }
        else  //若文件ASI_command_observation_control.txt未被打开，则尝试打开
        {
            if(LinkageControlFile.open(QIODevice::ReadWrite | QIODevice::Text))  //尝试打开文件ASI_command_observation_control.txt
            {
                qDebug()<<"ASI_command_observation_control.txt打开成功!";

                //读取 ASI_command_observation_control.txt 中的内容
                QTextStream FileStream(&LinkageControlFile); //将QFile对象与QTextStream对象关联起来.
                QString On_Off_Control_Observe = FileStream.readAll();

                qDebug()<<"ASI_command_observation_control.txt里的内容 = "<< On_Off_Control_Observe;

                //一、如果写的开始观测
                if(On_Off_Control_Observe == "on_control")
                {
                    QString SelectedObserveModeFile_Name = "";

                    //1.打开文件 ASI_selected_observation_mode.txt , 看选择哪个观测模式
                    QString ASI_selected_observation_modeTXTFullPath = LINKAGECONTROL_DIRPATH_window + "/ASI_selected_observation_mode.txt";
                    QFile SelectedObserveModeFile(ASI_selected_observation_modeTXTFullPath);
                    if(SelectedObserveModeFile.open(QIODevice::ReadOnly | QIODevice::Text))  //打开ASI_selected_observation_mode.txt
                    {
                        //读取 ASI_selected_observation_mode.txt 中的内容
                        QTextStream FileStream(&SelectedObserveModeFile); //将QFile对象与QTextStream对象关联起来.
                        SelectedObserveModeFile_Name = FileStream.readAll();

                        qDebug()<<"ASI_selected_observation_mode.txt里的内容 = "<< SelectedObserveModeFile_Name;

                        if(SelectedObserveModeFile_Name == "")
                        {
                            QString ErrorMessage = "从文件"+ASI_selected_observation_modeTXTFullPath +"读取到的信息为空，无法开始自动观测！请更正文件内容后重试。";
                            qDebug()<<ErrorMessage;
                            //QMessageBox::warning(this, "错误",ErrorMessage);
                            return;
                        }

                        SelectedObserveModeFile.close();  //及时关闭文件！！！
                    }
                    else  //如果打开 ASI_selected_observation_mode.txt 失败
                    {
                        QString ErrorMessage = "打开文件"+ASI_selected_observation_modeTXTFullPath +"失败！错误信息：" + SelectedObserveModeFile.errorString();
                        qDebug()<<ErrorMessage;
                        QMessageBox::warning(this, "错误",ErrorMessage);
                        return;
                    }

                    //2.设置新的观测模式
                    QString SelectedObserveModeFile_FullPath = PathUtils::ExtractParentDirPath(get_DataSaveDirPath())+"/ASI_observation_mode/"+SelectedObserveModeFile_Name;
                    ui->lineEdit_ObserveModeFullPath->setText(SelectedObserveModeFile_FullPath);
                    set_ObserveModeTXTFullPath(SelectedObserveModeFile_FullPath);
                    qDebug() << "联动控制选择观测模式文件路径 = " << SelectedObserveModeFile_FullPath;

                    //2.代码控制点击“启动观测”
                    QMetaObject::invokeMethod(ui->btn_StartAutoObserve, "clicked", Qt::QueuedConnection);

                    //3.改写 on_control 为 on_observation
                    LinkageControlFile.seek(0);  // 将文件指针移回文件开头
                    LinkageControlFile.resize(0);  // 清空文件内容
                    QString Command = "on_observation";
                    LinkageControlFile.write(Command.toUtf8());
                }

                //二、如果写的结束观测
                else if(On_Off_Control_Observe == "off_control")
                {
                    //1.代码控制点击“结束观测”
                    QMetaObject::invokeMethod(ui->btn_TerminateAutoObserve, "clicked", Qt::QueuedConnection);

                    //2.改写 off_control 为 off_observation
                    LinkageControlFile.seek(0);  // 将文件指针移回文件开头
                    LinkageControlFile.resize(0);  // 清空文件内容
                    QString Command = "off_observation";
                    LinkageControlFile.write(Command.toUtf8());
                }

                //三、如果文件为空，则直接退出函数
                else if(On_Off_Control_Observe == "")
                {
                    return;
                }

                LinkageControlFile.close();  //用完及时关闭文件！！！
            }
            else  //若文件 ASI_command_observation_control.txt 打开失败
            {
                QString ErrorMessage = "打开文件"+ASI_command_observation_controlTXTFullPath +"失败！错误信息：" + LinkageControlFile.errorString();
                qDebug()<<ErrorMessage;
                QMessageBox::warning(this, "错误",ErrorMessage);
            }
        }

    }
    else   //若文件ASI_selected_observation_mode.txt不存在
    {
        QString ErrorMessage = "文件"+ ASI_command_observation_controlTXTFullPath + "不存在！请新建此文件后重试。";
        qDebug()<<ErrorMessage;
        //QMessageBox::warning(this, "错误",ErrorMessage);
    }





}



//重写closeEvent函数，在退出程序前执行某些特定操作
void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton button;
    button = QMessageBox::question(this,tr("退出程序"),tr("确定退出吗?"),QMessageBox::Yes|QMessageBox::No);
    if(button == QMessageBox::Yes)
    {
        //----- Do Something before Close APP: ------

        myCameraController->UnInitializeAndCloseCamera();   //关闭相机
        m_LastImageFigure->close();  //关闭所拍图片显示窗口

        //----------- Do Something End ------------
        event->accept();  //接受退出event
    }
    else
    {
        event->ignore();  //取消退出
    }
}


//slots: 更新UI上的当前滤波片index
void MainWindow::UI_Update_CurSlotIndex(int slotIndex)
{
    UI_CurSlotIndex = slotIndex;

    if(slotIndex == -1)
    {
        ui->lab_CurSlotIndex->setText("--");
        ui->lab_CurSlotIndex_2->setText("--");
    }
    else
    {
        ui->lab_CurSlotIndex->setText( QString::number(slotIndex+1) );
        ui->lab_CurSlotIndex_2->setText( QString::number(slotIndex+1) );
    }


    // switch (slotIndex) {
    // case 0:
    //     ui->comboBox_changeSlotTo->setCurrentIndex(index_750850_window);
    //     break;
    // case 1:
    //     ui->comboBox_changeSlotTo->setCurrentIndex(index_862_window);
    //     break;
    // case 2:
    //     ui->comboBox_changeSlotTo->setCurrentIndex(index_630_window);
    //     break;
    // case 3:
    //     ui->comboBox_changeSlotTo->setCurrentIndex(index_000_window);
    //     break;
    // default:
    //     ui->comboBox_changeSlotTo->setCurrentIndex(0);
    //     break;
    // }

}

//slots: 根据CC返回的相机是否连接信号，更新UI上的“连接”、“断开”按钮状态
void MainWindow::UI_Update_WhetherCameraIsConnected(bool IfConnected)
{
    if(IfConnected)
    {
        ui->comboBox_CameraLink->setEnabled(false);
        ui->btn_CameraConnect->setEnabled(false);
        ui->btn_CameraDisconnect->setEnabled(true);
        // ui->groupBox_Binning->setEnabled(true);
        // ui->groupBox_Gain->setEnabled(true);
        ui->tabWidget_CameraManual->setTabEnabled(0,true);         //相机手控卡片
    }
    else
    {
        ui->comboBox_CameraLink->setEnabled(true);
        ui->btn_CameraConnect->setEnabled(true);
        ui->btn_CameraDisconnect->setEnabled(false);
        // ui->groupBox_Binning->setEnabled(false);
        // ui->groupBox_Gain->setEnabled(false);
        ui->tabWidget_CameraManual->setTabEnabled(0,false);         //相机手控卡片
    }
}




//slots: 更新UI上相机传感器温度值
void MainWindow::UI_UpdateCurSensorTemperature(float NewTemperature)
{
    ui->lab_CurSensorTemperature->setText( QString::number(int(NewTemperature)) );
    ui->lcdNumber_SensorTemperature->display(int(NewTemperature));
    UI_CameraSensorTemperature = NewTemperature;
}

//更新UI上相机的当前曝光时间s
void MainWindow::UI_UpdateCurExposureTime(float NewExposureTime)
{
    ui->lab_CurExposureTime->setText(QString::number(NewExposureTime));
    ui->lab_CurExposureTime_2->setText(QString::number(NewExposureTime));
    UI_CameraExposureTime = NewExposureTime*1000;
    ui->progressBar_ShootProgress->setRange(0,UI_CameraExposureTime);
    ui->progressBar_ShootProgress_2->setRange(0,UI_CameraExposureTime);
}

//更新UI上相机的快门模式
void MainWindow::UI_UpdateCurShuttingTimeMode(int ShuttingTimeMode)
{
    UI_CameraShuttingTimeMode = ShuttingTimeMode ;
    if(ShuttingTimeMode==1)
    {
        ui->lab_CurShuttingTimeMode->setText("自动");
        ui->lab_CurShuttingTimeMode_2->setText("自动");
    }
    else if(ShuttingTimeMode==2)
    {
        ui->lab_CurShuttingTimeMode->setText("常闭");
        ui->lab_CurShuttingTimeMode_2->setText("常闭");
    }
    else if(ShuttingTimeMode==3)
    {
        ui->lab_CurShuttingTimeMode->setText("常开");
        ui->lab_CurShuttingTimeMode_2->setText("常开");
    }
    else
    {
        QMessageBox::warning(this, "相机报错", "MainWindow::UI_UpdateCurShuttingTimeMode()收到了不可识别的参数，无法更新UI上的快门模式！");
    }
}

//UI更新当前工作时段
void MainWindow::UI_Update_CurWorkingStartAndEndTime(QString StartWorkingTime1,QString EndWorkingTime1,QString StartWorkingTime2,QString EndWorkingTime2)
{
    ui->lab_CurWorkingPeriodBegin->setText(StartWorkingTime1);
    ui->lab_CurWorkingPeriodEnd->setText(EndWorkingTime1);
    ui->lab_NextWorkingPeriodBegin->setText(StartWorkingTime2);
    ui->lab_NextWorkingPeriodEnd->setText(EndWorkingTime2);
}

//AOC阻塞后，UI上自动观测模式的“暂停”btn要切换显示“继续”，
void MainWindow::UI_Update_PauseBtnAfterAOCpaused()
{
    AOCThread_pause = true;
    ui->btn_PauseAutoObserve->setText("继续");
    ui->btn_StartAutoObserve->setEnabled(false);
    ui->btn_TerminateAutoObserve->setEnabled(true);
}

//在UI上显示MC的错误信息
void MainWindow::UI_Show_FCError(QString ErrorInfo)
{
    QMessageBox::warning(this, "滤波轮报错", ErrorInfo);
}

//在UI上显示CC的错误信息
void MainWindow::UI_Show_CCError(QString ErrorInfo)
{
    QMessageBox::warning(this, "相机报错", ErrorInfo);
}

//在UI上显示CC的提示信息
void MainWindow::UI_Show_CCInfo(QString Info)
{
    // QMessageBox::information(this, "相机提示", Info);

    //弹出自动淡出窗口输出提示信息
    MessageTips One_MessageTips;
    One_MessageTips.showMessageTips(Info,this);
}


//UI在消息盒子上更新从AOC收到的信息
void MainWindow::UI_Show_MessageBoxOutput(QString OneMessage)
{
    ui->textEdit_MessageOutput->append(currentDateTime_UorL(get_UsedTimeType()).toString("yyyy-MM-dd hh:mm:ss") + ": " +OneMessage);
}



void MainWindow::on_pB_ParameterPreset_clicked()
{
    ui->pB_ParameterPreset->setEnabled(false);
    ui->pB_ObservModePerset->setEnabled(true);
    ui->pB_ManualOperate->setEnabled(true);
    ui->pB_Automatic->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(0);
}


void MainWindow::on_pB_ObservModePerset_clicked()
{
    ui->pB_ParameterPreset->setEnabled(true);
    ui->pB_ObservModePerset->setEnabled(false);
    ui->pB_ManualOperate->setEnabled(true);
    ui->pB_Automatic->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(1);
}


void MainWindow::on_pB_ManualOperate_clicked()
{
    ui->pB_ParameterPreset->setEnabled(true);
    ui->pB_ObservModePerset->setEnabled(true);
    ui->pB_ManualOperate->setEnabled(false);
    ui->pB_Automatic->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(2);
}


void MainWindow::on_pB_Automatic_clicked()
{
    ui->pB_ParameterPreset->setEnabled(true);
    ui->pB_ObservModePerset->setEnabled(true);
    ui->pB_ManualOperate->setEnabled(true);
    ui->pB_Automatic->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//                                            一：参数预设
///////////////////////////////////////////////////////////////////////////////////////////////////////

//电源 连接串口
void MainWindow::on_btn_PowerConnect_clicked()
{
    if(mySerialPort.isOpen())
    {
        QMessageBox::warning(this, "错误","此串口已被占用，无法连接！");
        return;
    }
    QList<QSerialPortInfo> comList = QSerialPortInfo::availablePorts();
    QSerialPortInfo portInfo = comList.at(ui->comboBox_powerLink->currentIndex());/////////////////////////////////////改成 用串口名，避免串口编号不连续时索引
    mySerialPort.setPort(portInfo); //设置使用当前下拉栏选中的串口
    mySerialPort.setBaudRate(QSerialPort::Baud57600);  //电源的波特率57600
    mySerialPort.setDataBits(QSerialPort::Data8);      //数据位8位
    mySerialPort.setStopBits(QSerialPort::OneStop);    //停止位1位
    mySerialPort.setParity(QSerialPort::NoParity);     //无奇偶校验
    mySerialPort.open(QIODevice::ReadWrite);           //以读写方式打开
    if(mySerialPort.isOpen())
    {
        ui->comboBox_powerLink->setEnabled(false);
        ui->btn_PowerConnect->setEnabled(false);
        ui->btn_PowerDisconnect->setEnabled(true);
        ui->tabWidget_SmartPowerManual->setTabEnabled(0,true);
        qDebug()<<"电源串口打开成功！";   
    }
    else
    {
        QSerialPort::SerialPortError error;
        QString  ERRORString ;
        switch (error)
        {
        case QSerialPort::NoError:
            ERRORString=  "No Error";
            break;
        case QSerialPort::DeviceNotFoundError:
            ERRORString= "Device Not Found";
            break;
        case QSerialPort::PermissionError:
            ERRORString= "Permission Denied";
            break;
        case QSerialPort::OpenError:
            ERRORString= "Open Error";
            break;
        // case QSerialPort::ParityError:
        //     ERRORString= "Parity Error";
        //     break;
        // case QSerialPort::FramingError:
        //     ERRORString= "Framing Error";
        //     break;
        // case QSerialPort::BreakConditionError:
        //     ERRORString= "Break Condition";
        //     break;
        case QSerialPort::WriteError:
            ERRORString= "Write Error";
            break;
        case QSerialPort::ReadError:
            ERRORString= "Read Error";
            break;
        case QSerialPort::ResourceError:
            ERRORString= "Resource Error";
            break;
        case QSerialPort::UnsupportedOperationError:
            ERRORString= "Unsupported Operation";
            break;
        case QSerialPort::UnknownError:
            ERRORString= "Unknown Error";
            break;
        case QSerialPort::TimeoutError:
            ERRORString= "Timeout Error";
            break;
        case QSerialPort::NotOpenError:
            ERRORString= "Not Open Error";
            break;
        default:
            ERRORString= "Other Error";
        }
        QString ErrorMessage = "电源串口未被占用，但无法打开！\n发生 SerialPortWorker::errorOccurred , 错误信息是：" + ERRORString;
        qDebug()<<ErrorMessage;
        QMessageBox::warning(this, "错误",ErrorMessage);

    }
}

//电源断开连接
void MainWindow::on_btn_PowerDisconnect_clicked()
{
    if(mySerialPort.isOpen())
    {
        mySerialPort.close();
        ui->comboBox_powerLink->setEnabled(true);
        ui->btn_PowerConnect->setEnabled(true);
        ui->btn_PowerDisconnect->setEnabled(false);
        ui->tabWidget_SmartPowerManual->setTabEnabled(0,false);
        qDebug()<<"电源串口已关闭！";
    }
    else
    {
        QMessageBox::warning(this, "提示","串口已关闭，请勿重复操作！");
    }
}

//slots: 电源串口接收信息处理
void MainWindow::do_com_readyRead()
{
    QByteArray alldata = mySerialPort.readAll();
    qDebug()<<"电源串口收到：alldata= "<< alldata;

    // 检查 alldata 中的特定字节来确定串口数据的内容
    const char* dataPtr = alldata.constData();

    if(alldata.size() == 8 && dataPtr[0] == 'H' && dataPtr[1] == '\x1A' && dataPtr[2] == '\x00' && dataPtr[7] == 'M')
    {
        if(dataPtr[3] == '\x04' ) //帧类型04：继电器全开/全关
        {
            switch (dataPtr[4]) {
            case '\x00':
                ui->label_realyFlag_1->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->label_realyFlag_2->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->label_realyFlag_3->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->label_realyFlag_4->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->label_realyFlag_5->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->label_realyFlag_6->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->label_realyFlag_7->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->label_realyFlag_8->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                ui->btn_off_all->setEnabled(false);
                ui->btn_on_all->setEnabled(true);
                ui->btn_off_1->setEnabled(false);
                ui->btn_off_2->setEnabled(false);
                ui->btn_off_3->setEnabled(false);
                ui->btn_off_4->setEnabled(false);
                ui->btn_off_5->setEnabled(false);
                ui->btn_off_6->setEnabled(false);
                ui->btn_off_7->setEnabled(false);
                ui->btn_off_8->setEnabled(false);
                ui->btn_on_1->setEnabled(true);
                ui->btn_on_2->setEnabled(true);
                ui->btn_on_3->setEnabled(true);
                ui->btn_on_4->setEnabled(true);
                ui->btn_on_5->setEnabled(true);
                ui->btn_on_6->setEnabled(true);
                ui->btn_on_7->setEnabled(true);
                ui->btn_on_8->setEnabled(true);
                relay_1_Open = 0;
                relay_2_Open = 0;
                relay_3_Open = 0;
                relay_4_Open = 0;
                relay_5_Open = 0;
                relay_6_Open = 0;
                relay_7_Open = 0;
                relay_8_Open = 0;
                break;
            case '\x01':
                ui->label_realyFlag_1->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->label_realyFlag_2->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->label_realyFlag_3->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->label_realyFlag_4->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->label_realyFlag_5->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->label_realyFlag_6->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->label_realyFlag_7->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->label_realyFlag_8->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                ui->btn_off_all->setEnabled(true);
                ui->btn_on_all->setEnabled(false);
                ui->btn_off_1->setEnabled(true);
                ui->btn_off_2->setEnabled(true);
                ui->btn_off_3->setEnabled(true);
                ui->btn_off_4->setEnabled(true);
                ui->btn_off_5->setEnabled(true);
                ui->btn_off_6->setEnabled(true);
                ui->btn_off_7->setEnabled(true);
                ui->btn_off_8->setEnabled(true);
                ui->btn_on_1->setEnabled(false);
                ui->btn_on_2->setEnabled(false);
                ui->btn_on_3->setEnabled(false);
                ui->btn_on_4->setEnabled(false);
                ui->btn_on_5->setEnabled(false);
                ui->btn_on_6->setEnabled(false);
                ui->btn_on_7->setEnabled(false);
                ui->btn_on_8->setEnabled(false);
                relay_1_Open = 1;
                relay_2_Open = 1;
                relay_3_Open = 1;
                relay_4_Open = 1;
                relay_5_Open = 1;
                relay_6_Open = 1;
                relay_7_Open = 1;
                relay_8_Open = 1;
                break;
            default:
                QMessageBox::warning(this, "错误","电源返回的数据帧在04字段后格式错误！");
                break;
            }
        }
        if(dataPtr[3] == '\x05'  ) //帧类型05：单个继电器开/关
        {
            switch (dataPtr[4]) {
            case '\x00':
                if(relay_1_Open==1) //如果继电器1之前是开的
                {
                    ui->label_realyFlag_1->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}") ;
                    ui->btn_off_1->setEnabled(false);
                    ui->btn_on_1->setEnabled(true);
                    relay_1_Open = 0;
                }
                else                //如果继电器1之前是关的
                {
                    ui->label_realyFlag_1->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_1->setEnabled(true);
                    ui->btn_on_1->setEnabled(false);
                    relay_1_Open = 1;
                }
                break;
            case '\x01':
                if(relay_2_Open==1)    //如果继电器2之前是开的
                {
                    ui->label_realyFlag_2->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                    ui->btn_off_2->setEnabled(false);
                    ui->btn_on_2->setEnabled(true);
                    relay_2_Open = 0;
                }
                else                    //如果继电器2之前是关的
                {
                    ui->label_realyFlag_2->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_2->setEnabled(true);
                    ui->btn_on_2->setEnabled(false);
                    relay_2_Open = 1;
                }
                break;
            case '\x02':
                if(relay_3_Open==1)     //如果继电器3之前是开的
                {
                    ui->label_realyFlag_3->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                    ui->btn_off_3->setEnabled(false);
                    ui->btn_on_3->setEnabled(true);
                    relay_3_Open = 0;
                }
                else
                {
                    ui->label_realyFlag_3->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_3->setEnabled(true);
                    ui->btn_on_3->setEnabled(false);
                    relay_3_Open = 1;
                }
                break;
            case '\x03':
                if(relay_4_Open==1)     //如果继电器4之前是开的
                {
                    ui->label_realyFlag_4->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                    ui->btn_off_4->setEnabled(false);
                    ui->btn_on_4->setEnabled(true);
                    relay_4_Open = 0;
                }
                else
                {
                    ui->label_realyFlag_4->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_4->setEnabled(true);
                    ui->btn_on_4->setEnabled(false);
                    relay_4_Open = 1;
                }
                break;
            case '\x04':
                if(relay_5_Open==1)     //如果继电器5之前是开的
                {
                    ui->label_realyFlag_5->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                    ui->btn_off_5->setEnabled(false);
                    ui->btn_on_5->setEnabled(true);
                    relay_5_Open = 0;
                }
                else
                {
                    ui->label_realyFlag_5->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_5->setEnabled(true);
                    ui->btn_on_5->setEnabled(false);
                    relay_5_Open = 1;
                }
                break;
            case '\x05':
                if(relay_6_Open==1)     //如果继电器6之前是开的
                {
                    ui->label_realyFlag_6->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                    ui->btn_off_6->setEnabled(false);
                    ui->btn_on_6->setEnabled(true);
                    relay_6_Open = 0;
                }
                else
                {
                    ui->label_realyFlag_6->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_6->setEnabled(true);
                    ui->btn_on_6->setEnabled(false);
                    relay_6_Open = 1;
                }
                break;
            case '\x06':
                if(relay_7_Open==1)     //如果继电器7之前是开的
                {
                    ui->label_realyFlag_7->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                    ui->btn_off_7->setEnabled(false);
                    ui->btn_on_7->setEnabled(true);
                    relay_7_Open = 0;
                }
                else
                {
                    ui->label_realyFlag_7->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_7->setEnabled(true);
                    ui->btn_on_7->setEnabled(false);
                    relay_7_Open = 1;
                }
                break;
            case '\x07':
                if(relay_8_Open==1)     //如果继电器8之前是开的
                {
                    ui->label_realyFlag_8->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(240,240,240);}");
                    ui->btn_off_8->setEnabled(false);
                    ui->btn_on_8->setEnabled(true);
                    relay_8_Open = 0;
                }
                else
                {
                    ui->label_realyFlag_8->setStyleSheet("QLabel{border:1px solid #000000;background-color:rgb(80,245,80);}");
                    ui->btn_off_8->setEnabled(true);
                    ui->btn_on_8->setEnabled(false);
                    relay_8_Open = 1;
                }
                break;
            default:
                QMessageBox::warning(this, "错误","电源返回的数据帧在字段05后格式错误！");
                break;
            }

            //把relay_1_Open改成char型之后，再用true_count判断开了几个通道就可能出错
            // int true_count = relay_1_Open + relay_2_Open + relay_3_Open + relay_4_Open +
            //                  relay_5_Open + relay_6_Open + relay_7_Open + relay_8_Open;

            //若8通道全开了
            if ((relay_1_Open==1) && (relay_2_Open==1) && (relay_3_Open==1) && (relay_4_Open==1) && (relay_5_Open==1) && (relay_6_Open==1) && (relay_7_Open==1) && (relay_8_Open==1) )
            {
                ui->btn_on_all->setEnabled(false);
                ui->btn_off_all->setEnabled(true);
            }
            else if ( (relay_1_Open==0) && (relay_2_Open==0) && (relay_3_Open==0) && (relay_4_Open==0) && (relay_5_Open==0) && (relay_6_Open==0) && (relay_7_Open==0) && (relay_8_Open==0) )  // 全为false
            {
                ui->btn_on_all->setEnabled(true);
                ui->btn_off_all->setEnabled(false);
            }
            else // 不全为true也不全为false
            {
                ui->btn_on_all->setEnabled(true);
                ui->btn_off_all->setEnabled(true);
            }

        }
    }
    else
    {
        //QMessageBox::warning(this, "错误", "返回的数据帧长度或格式错误！");  //有时会连续弹出多个提示框，省去
    }
}


//滤波轮-串口连接
void MainWindow::on_btn_MotorConnect_clicked()
{
    if(myFilterController->get_FilterIsConnected())
    {
        QMessageBox::warning(this, "错误","滤波轮已经连接！请勿重复操作。");
    }
    else
    {
        if(myFilterController->ConnectAndInitFilter())
        {
            ui->comboBox_MotorLink->setEnabled(false);
            ui->btn_MotorConnect->setEnabled(false);
            ui->btn_MotorDisconnect->setEnabled(true);
            ui->tabWidget_Filter->setTabEnabled(0,true);
            qDebug()<<"滤波轮连接成功！";
            //每次连接成功后，先把滤波轮转到1号孔位
            emit UI_Send_ChangeSlotTo(index_750850_window);
        }
    }
}

//滤波轮-断开连接
void MainWindow::on_btn_MotorDisconnect_clicked()
{
    if(myFilterController->get_FilterIsConnected())
    {
        myFilterController->DisConnectFilter();
        ui->comboBox_MotorLink->setEnabled(true);
        ui->btn_MotorConnect->setEnabled(true);
        ui->btn_MotorDisconnect->setEnabled(false);
        ui->tabWidget_Filter->setTabEnabled(0,false);
    }
    else
    {
        QMessageBox::warning(this, "错误","运动控制器已经断开连接！请勿重复操作。");
    }

}

//连接相机串口
void MainWindow::on_btn_CameraConnect_clicked()
{
    //emit btn_ConnectCamera(); //发射相机连接信号
}

//断开相机连接
void MainWindow::on_btn_CameraDisconnect_clicked()
{
    //emit btn_DisConnectCamera(); //发射断开相机连接的信号
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
//                                            二：模式预设
///////////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::on_btn_addMode_clicked()
{
    //添加行
    //int currentRow = ui->tableWidget_Mode->currentRow();
    int maxRow = ui->tableWidget_Mode->rowCount();
    ui->tableWidget_Mode->insertRow(maxRow);
}


void MainWindow::on_btn_deleteMode_clicked()
{
    int currentRow = ui->tableWidget_Mode->currentRow();
    ui->tableWidget_Mode->removeRow(currentRow);
}


void MainWindow::on_btn_UpMode_clicked()
{
    int selectedRow = ui->tableWidget_Mode->currentRow();
    //qDebug()<<"selectedRow="<<selectedRow;
    if (selectedRow >= 1) //当前行至少是第二行才进行交换操作
    {
        ui->tableWidget_Mode->insertRow(selectedRow+1); //当前行之后插入空白行
       // qDebug()<<"selectedRow="<<selectedRow;
        for (int column = 0; column < ui->tableWidget_Mode->columnCount(); ++column) //复制当前行的前一行到当前行下的下一行
        {
            QTableWidgetItem *sourceItem = ui->tableWidget_Mode->item(selectedRow-1, column);
            //qDebug()<<"selectedRow-1 ="<<selectedRow-1;
            if (sourceItem) //当前项不为空才复制
            {
                QTableWidgetItem *targetItem = new QTableWidgetItem(*sourceItem);
                ui->tableWidget_Mode->setItem((selectedRow+1), column, targetItem); // 复制数据到目标行
            }
        }
        ui->tableWidget_Mode->removeRow(selectedRow-1);//删除当前行之前的一行
    }
}


void MainWindow::on_btn_DownMode_clicked()
{
    int selectedRow = ui->tableWidget_Mode->currentRow();
    //qDebug()<<"selectedRow="<<selectedRow;
    if ( selectedRow < ui->tableWidget_Mode->rowCount()-1) //当前行至少是倒数第二行才进行交换操作
    {
        ui->tableWidget_Mode->insertRow(selectedRow); //当前行前插入空白行
        selectedRow = ui->tableWidget_Mode->currentRow();//更新selectedRow
        //qDebug()<<"selectedRow="<<selectedRow;
        //qDebug()<<ui->tableWidget_Mode->currentRow();
        for (int column = 0; column < ui->tableWidget_Mode->columnCount(); ++column) //复制当前行的下一行 到 当前行的上一行
        {
            QTableWidgetItem *sourceItem = ui->tableWidget_Mode->item(selectedRow+1, column);
            if (sourceItem) //当前项不为空才复制
            {
                QTableWidgetItem *targetItem = new QTableWidgetItem(*sourceItem);
                ui->tableWidget_Mode->setItem((selectedRow-1), column, targetItem); // 复制数据到目标行
            }
        }
        ui->tableWidget_Mode->removeRow(selectedRow+1);//删除当前行之后的一行
    }
}


void MainWindow::on_btn_ActionAdd_clicked()
{
    //表中添加行
    QTableWidget* currentTableWidget = nullptr;
    switch (ui->tabWidget_EditMode->currentIndex()) {
    case 0:
        currentTableWidget = ui->tableWidget_section1;
        break;
    case 1:
        currentTableWidget = ui->tableWidget_section2;
        break;
    case 2:
        currentTableWidget = ui->tableWidget_section3;
        break;
    default:
        qDebug()<<"未定义的页面";
        break;
    }
    int maxRow = currentTableWidget->rowCount();
    currentTableWidget->insertRow(maxRow);
    //打开ActionEdit子页
    ActionEdit* Bianjidongzuo = new ActionEdit();
    Bianjidongzuo->show();
}


void MainWindow::on_btn_ActionRemove_clicked()
{
    QTableWidget* currentTableWidget = nullptr;
    switch (ui->tabWidget_EditMode->currentIndex()) {
    case 0:
        currentTableWidget = ui->tableWidget_section1;
        break;
    case 1:
        currentTableWidget = ui->tableWidget_section2;
        break;
    case 2:
        currentTableWidget = ui->tableWidget_section3;
        break;
    default:
        qDebug()<<"未定义的页面";
        break;
    }
    int currentRow = currentTableWidget->currentRow();
    currentTableWidget->removeRow(currentRow);
}


void MainWindow::on_btn_ActionUp_clicked()
{
    QTableWidget* currentTableWidget = nullptr;
    switch (ui->tabWidget_EditMode->currentIndex()) {
    case 0:
        currentTableWidget = ui->tableWidget_section1;
        break;
    case 1:
        currentTableWidget = ui->tableWidget_section2;
        break;
    case 2:
        currentTableWidget = ui->tableWidget_section3;
        break;
    default:
        qDebug()<<"未定义的页面";
        break;
    }
    int selectedRow = currentTableWidget->currentRow();
    if (selectedRow >= 1) //当前行至少是第二行才进行交换操作
    {
        currentTableWidget->insertRow(selectedRow+1); //当前行之后插入空白行
        for (int column = 0; column < currentTableWidget->columnCount(); ++column) //复制当前行的前一行到当前行下的下一行
        {
            QTableWidgetItem *sourceItem = currentTableWidget->item(selectedRow-1, column);
            if (sourceItem) //当前项不为空才复制
            {
                QTableWidgetItem *targetItem = new QTableWidgetItem(*sourceItem);
                currentTableWidget->setItem((selectedRow+1), column, targetItem); // 复制数据到目标行
            }
        }
        currentTableWidget->removeRow(selectedRow-1);//删除当前行之前的一行
    }
}


void MainWindow::on_btn_ActionDown_clicked()
{
    QTableWidget* currentTableWidget = nullptr;
    switch (ui->tabWidget_EditMode->currentIndex()) {
    case 0:
        currentTableWidget = ui->tableWidget_section1;
        break;
    case 1:
        currentTableWidget = ui->tableWidget_section2;
        break;
    case 2:
        currentTableWidget = ui->tableWidget_section3;
        break;
    default:
        qDebug()<<"未定义的页面";
        break;
    }
    int selectedRow = currentTableWidget->currentRow();
    if ( selectedRow < currentTableWidget->rowCount()-1) //当前行至少是倒数第二行才进行交换操作
    {
        currentTableWidget->insertRow(selectedRow); //当前行前插入空白行
        selectedRow = currentTableWidget->currentRow();//更新selectedRow
        for (int column = 0; column < currentTableWidget->columnCount(); ++column) //复制当前行的下一行 到 当前行的上一行
        {
            QTableWidgetItem *sourceItem = currentTableWidget->item(selectedRow+1, column);
            if (sourceItem) //当前项不为空才复制
            {
                QTableWidgetItem *targetItem = new QTableWidgetItem(*sourceItem);
                currentTableWidget->setItem((selectedRow-1), column, targetItem); // 复制数据到目标行
            }
        }
        currentTableWidget->removeRow(selectedRow+1);//删除当前行之后的一行
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//                                            三：手动观测
///////////////////////////////////////////////////////////////////////////////////////////////////////

//电源-8通道全开
void MainWindow::on_btn_on_all_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_ALL ON BLANK BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    qDebug()<<"串口发送数据帧：继电器全开 "<<frames;
    Sleep(SLEEP_DURATION);
    // relay_1_Open = 1;
    // relay_2_Open = 1;
    // relay_3_Open = 1;
    // relay_4_Open = 1;
    // relay_5_Open = 1;
    // relay_6_Open = 1;
    // relay_7_Open = 1;
    // relay_8_Open = 1;
}

//电源-8通道全关
void MainWindow::on_btn_off_all_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_ALL OFF BLANK BLANK FRAMES_TAIL );
    mySerialPort.write(frames);  // "481A00040000004D" 帧类型04, 8通道继电器全关
    mySerialPort.waitForBytesWritten(); //等待写入信号，否则阻塞
    qDebug()<<"串口发送数据帧：继电器全关 "<<frames;
    Sleep(SLEEP_DURATION); //必须睡眠至少20ms，后一句指令才能被写入并执行
    // relay_1_Open = 0;
    // relay_2_Open = 0;
    // relay_3_Open = 0;
    // relay_4_Open = 0;
    // relay_5_Open = 0;
    // relay_6_Open = 0;
    // relay_7_Open = 0;
    // relay_8_Open = 0;
}

//电源-通道1打开
void MainWindow::on_btn_on_1_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_1 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_1_Open = 1;
    qDebug()<<"串口发送数据帧：通道1，开 "<<frames;
}

//电源-通道2打开
void MainWindow::on_btn_on_2_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_2 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_2_Open = 1;
    qDebug()<<"串口发送数据帧：通道2，开 "<<frames;
}

//电源-通道3打开
void MainWindow::on_btn_on_3_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_3 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_3_Open = 1;
    qDebug()<<"串口发送数据帧：通道3，开 "<<frames;
}

//电源-通道4打开
void MainWindow::on_btn_on_4_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_4 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_4_Open = 1;
    qDebug()<<"串口发送数据帧：通道4，开 "<<frames;
}

//电源-通道5打开
void MainWindow::on_btn_on_5_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_5 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_5_Open = 1;
    qDebug()<<"串口发送数据帧：通道5，开 "<<frames;
}

//电源-通道6打开
void MainWindow::on_btn_on_6_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_6 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_6_Open = 1;
    qDebug()<<"串口发送数据帧：通道6，开 "<<frames;
}

//电源-通道7打开
void MainWindow::on_btn_on_7_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_7 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_7_Open = 1;
    qDebug()<<"串口发送数据帧：通道7，开 "<<frames;
}

//电源-通道8全开
void MainWindow::on_btn_on_8_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_8 ON BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_8_Open = 1;
    qDebug()<<"串口发送数据帧：通道8，开 "<<frames;
}

//电源-通道1关闭
void MainWindow::on_btn_off_1_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_1 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_1_Open = 0;
    qDebug()<<"串口发送数据帧：通道1，关 "<<frames;
}

//电源-通道2关闭
void MainWindow::on_btn_off_2_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_2 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_2_Open = 0;
    qDebug()<<"串口发送数据帧：通道2，关 "<<frames;
}

//电源-通道3关闭
void MainWindow::on_btn_off_3_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_3 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_3_Open = 0;
    qDebug()<<"串口发送数据帧：通道3，关 "<<frames;
}

//电源-通道4关闭
void MainWindow::on_btn_off_4_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_4 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_4_Open = 0;
    qDebug()<<"串口发送数据帧：通道4，关 "<<frames;
}

//电源-通道5关闭
void MainWindow::on_btn_off_5_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_5 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_5_Open = 0;
    qDebug()<<"串口发送数据帧：通道5，关 "<<frames;
}

//电源-通道6关闭
void MainWindow::on_btn_off_6_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_6 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_6_Open = 0;
    qDebug()<<"串口发送数据帧：通道6，关 "<<frames;
}

//电源-通道7关闭
void MainWindow::on_btn_off_7_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_7 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_7_Open = 0;
    qDebug()<<"串口发送数据帧：通道7，关 "<<frames;
}

//电源-通道8关闭
void MainWindow::on_btn_off_8_clicked()
{
    QByteArray frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_8 OFF BLANK FRAMES_TAIL );
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);
    //relay_8_Open = 0;
    qDebug()<<"串口发送数据帧：通道8，关 "<<frames;
}





//目标温度输入框，按下enter后
void MainWindow::on_lineEdit_SetTargetTemperature_returnPressed()
{
    emit LineEditEnter_SetTargetTemperature(ui->lineEdit_SetTargetTemperature->text().toFloat());
}

//曝光时间输入框s，按下enter后
void MainWindow::on_lineEdit_SetExposureTime_returnPressed()
{
    emit LineEditEnter_SetExposureTime(ui->lineEdit_SetExposureTime->text().toFloat()*1000);
}

//快门模式选择栏，改变index后
// void MainWindow::on_comboBox_SetShutterTimingMode_currentIndexChanged(int index)
// {

// }


///////////////////////////////////////////////////////////////////////////////////////////////////////
//                                            四：自动观测
///////////////////////////////////////////////////////////////////////////////////////////////////////


// “自动观测-启动观测”btn按下
void MainWindow::on_btn_StartAutoObserve_clicked()
{
    subThread_AutoObserveController->start(); //***启动自动观测控制器子线程***
    emit UI_Report_StartAutoObserve();   //此信号连接到AutoObserveControllerMain(): (重新读取时间表和模式、开启定时器)
    ui->btn_StartAutoObserve->setEnabled(false);
    ui->btn_PauseAutoObserve->setEnabled(true);
    ui->btn_TerminateAutoObserve->setEnabled(true);
}

// “自动观测-暂停/继续”btn按下
void MainWindow::on_btn_PauseAutoObserve_clicked()
{
    if(AOCThread_pause)  //如果之前是 暂停 状态，那就继续
    {
        myAutoObserveController->ResumeAutoObserve();  //QWaitCondition::WakeAll()必须由不同于被阻塞线程的其他线程来发出
        AOCThread_pause = false;
        ui->btn_PauseAutoObserve->setText("暂停");
        ui->btn_StartAutoObserve->setEnabled(false);
        ui->btn_TerminateAutoObserve->setEnabled(true);
    }
    else                //如果之前是 继续 状态，那就暂停：
    {
        emit UI_Report_PauseAutoObserve();
        AOCThread_pause = true;
        ui->btn_PauseAutoObserve->setText("继续");
        ui->btn_StartAutoObserve->setEnabled(false);
        ui->btn_TerminateAutoObserve->setEnabled(true);
    }
    QThread::msleep(30);  //消抖，避免鼠标按键抖动导致连续点击
}

// “自动观测-结束观测”btn按下
void MainWindow::on_btn_TerminateAutoObserve_clicked()
{

    ui->btn_StartAutoObserve->setEnabled(true);
    ui->btn_PauseAutoObserve->setText("暂停");
    ui->btn_PauseAutoObserve->setEnabled(false);
    ui->btn_TerminateAutoObserve->setEnabled(false);
    ui->lab_CurWorkingPeriodBegin->setText("---");
    ui->lab_CurWorkingPeriodEnd->setText("---");
    ui->lab_NextWorkingPeriodBegin->setText("---");
    ui->lab_NextWorkingPeriodEnd->setText("---");

    myCameraController->StopAcquireImmediately();  //立即停止相机采集

    myAutoObserveController->TerminateAutoObserve();  //清空程序中存储的时间表和模式、有效工作时段List
    subThread_AutoObserveController->terminate();  //终止AOC线程运行
    subThread_AutoObserveController->wait();        //terminate()后必须跟wait()


}

 //slots: UI定时器超时处理函数
void MainWindow::UI_onTimeout()
{ 
    //更新"自动观测"界面的-当前波段
    if(UI_CurSlotIndex == index_750850_window)
    {
        ui->lab_CurObserveWaveBand->setText("750-850nm");
        ui->lab_CurSlotIndex->setText(QString::number(index_750850_window+1));
        ui->lab_CurSlotIndex_2->setText(QString::number(index_750850_window+1));
        ui->comboBox_changeSlotTo->setCurrentIndex(index_750850_window);
    }
    else if(UI_CurSlotIndex == index_862_window)
    {
        ui->lab_CurObserveWaveBand->setText("862nm");
        ui->lab_CurSlotIndex->setText(QString::number(index_862_window+1));
        ui->lab_CurSlotIndex_2->setText(QString::number(index_862_window+1));
        ui->comboBox_changeSlotTo->setCurrentIndex(index_862_window);
    }
    else if(UI_CurSlotIndex == index_630_window)
    {
        ui->lab_CurObserveWaveBand->setText("630nm");
        ui->lab_CurSlotIndex->setText(QString::number(index_630_window+1));
        ui->lab_CurSlotIndex_2->setText(QString::number(index_630_window+1));
        ui->comboBox_changeSlotTo->setCurrentIndex(index_630_window);
    }
    else if(UI_CurSlotIndex == index_000_window)
    {
        ui->lab_CurObserveWaveBand->setText("遮光-暗场");
        ui->lab_CurSlotIndex->setText(QString::number(index_000_window+1));
        ui->lab_CurSlotIndex_2->setText(QString::number(index_000_window+1));
        ui->comboBox_changeSlotTo->setCurrentIndex(index_000_window);
    }
    else
    {
        ui->lab_CurObserveWaveBand->setText("---");
        ui->lab_CurSlotIndex->setText("---");
        ui->lab_CurSlotIndex_2->setText("---");
    }


    //若LastImageDisplayWindow没有显示，则把“显示最近一张图片”取消勾选
    if(!m_LastImageFigure->isVisible())
    {
        ui->checkBox_IfShowLIDW->setCheckState(Qt::Unchecked);
        ui->checkBox_IfShowLIDW_2->setCheckState(Qt::Unchecked);
    }

    //更新UI上显示的时间
    ui->label_DisplayCurrentTime->setText(currentDateTime_UorL(get_UsedTimeType()).toString("hh:mm:ss\nyyyy年MM月dd日 "));

    //如果启动了联动控制，则开始检测联动控制文件
    if(UI_IfLinkageControlOpened)
    {
        if(++UI_LinkageControlFile_AlreadyTimeoutHowManyTimes > UI_LinkageControlFile_HowManyTimesReadOnce)
        {
            UI_LinkageControlFile_AlreadyTimeoutHowManyTimes = 0;
        }
        else
        {
            Do_LinkageControl();
        }
    }

    //如果 相机正在采集数据标志 被置1，则给拍摄进度条加数值
    if(UI_IfCameraIsAcquiring)
    {
        // int ProgressBar_NewValue = ui->progressBar_ShootProgress->value() + UI_Timerout_period;
        int ProgressBar_NewValue = m_AcquireProgressBar_StartTime.msecsTo(QDateTime::currentDateTime());
        int Max_Value = ui->progressBar_ShootProgress->maximum();

        if(ProgressBar_NewValue >= Max_Value)
        {
            ui->progressBar_ShootProgress->setValue(Max_Value);
            ui->progressBar_ShootProgress_2->setValue(Max_Value);

            QThread::msleep(700);

            ui->progressBar_ShootProgress->setValue(0);
            ui->progressBar_ShootProgress_2->setValue(0);
            ui->progressBar_ShootProgress->hide();
            ui->progressBar_ShootProgress_2->hide();
            UI_IfCameraIsAcquiring = false;
            ui->btn_ShootOneImage->setEnabled(true);  //相机结束采集后，使能“拍摄”按钮
        }
        else
        {
            ui->progressBar_ShootProgress->setValue(ProgressBar_NewValue);
            ui->progressBar_ShootProgress_2->setValue(ProgressBar_NewValue);
        }

    }

}


//slots: 在UI线程中唤醒AOC线程，从而执行后面的观测动作
void MainWindow::UI_Respond_AwakeSubThreadAOC()
{
    //myAutoObserveController->AwakeSubThreadAOC();
    myAutoObserveController->ResumeAutoObserve();
}

//AOC完成了所有的有效观测时段，通知UI更新“启动暂停结束btn” 并 退出AOC线程
void MainWindow::UI_Respond_FinishedAllWorkPeriod()
{
    myAutoObserveController->TerminateAutoObserve();  //清空程序中存储的时间表和模式、有效工作时段List
    subThread_AutoObserveController->terminate();  //终止AOC线程运行
    subThread_AutoObserveController->wait();        //terminate()后必须跟wait()

    ui->btn_StartAutoObserve->setEnabled(true);
    ui->btn_PauseAutoObserve->setText("暂停");
    ui->btn_PauseAutoObserve->setEnabled(false);
    ui->btn_TerminateAutoObserve->setEnabled(false);
    ui->lab_CurWorkingPeriodBegin->setText("---");
    ui->lab_CurWorkingPeriodEnd->setText("---");
    ui->lab_NextWorkingPeriodBegin->setText("---");
    ui->lab_NextWorkingPeriodEnd->setText("---");
    QMessageBox::information(this, "提示","所有观测时段的观测任务均已完成！如需继续观测请添加观测任务。");  //弹窗提示所有观测任务已完成
}

//slots: 在AOC控制下，操纵继电器开启（ONorOFF==1）或关闭（ONorOFF==0），RelayIndex==-1表示全部继电器);
void MainWindow::UI_PSC_Respond_OperatePowerSwitch(int RelayIndex, char ONorOFF)
{
    QByteArray frames;  //给电源的指令数据帧
    char* relayOpen_flag = nullptr;  //用于指向当前选择的那个继电器的是否开启flag
    QDateTime CommandSendTime = currentDateTime_UorL(get_UsedTimeType());

    switch (RelayIndex)
    {
    case -1:         //如果是全部继电器
        if(ONorOFF)  //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_ALL ON BLANK BLANK FRAMES_TAIL );
        else         //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_ALL OFF BLANK BLANK FRAMES_TAIL );
        break;
    case 1:             //如果是继电器1
        relayOpen_flag = &relay_1_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_1 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_1 OFF BLANK FRAMES_TAIL );
        break;
    case 2:             //如果是继电器2
        relayOpen_flag = &relay_2_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_2 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_2 OFF BLANK FRAMES_TAIL );
        break;
    case 3:             //如果是继电器3
        relayOpen_flag = &relay_3_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_3 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_3 OFF BLANK FRAMES_TAIL );
        break;
    case 4:             //如果是继电器4
        relayOpen_flag = &relay_4_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_4 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_4 OFF BLANK FRAMES_TAIL );
        break;
    case 5:             //如果是继电器5
        relayOpen_flag = &relay_5_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_5 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_5 OFF BLANK FRAMES_TAIL );
        break;
    case 6:             //如果是继电器6
        relayOpen_flag = &relay_6_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_6 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_6 OFF BLANK FRAMES_TAIL );
        break;
    case 7:             //如果是继电器7
        relayOpen_flag = &relay_7_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_7 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_7 OFF BLANK FRAMES_TAIL );
        break;
    case 8:             //如果是继电器8
        relayOpen_flag = &relay_8_Open;
        if(ONorOFF)     //如果开启
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_8 ON BLANK FRAMES_TAIL );
        else            //如果关闭
            frames = QByteArray::fromHex( FRAMES_HEADER FACILITY_TYPE FACILITY_ID RELAY_SINGAL RELAY_8 OFF BLANK FRAMES_TAIL );
        break;
    }
    //---------------------------------switch() End-------------------------------------------------------------//
    //向缓冲区写入数据
    mySerialPort.write(frames);
    mySerialPort.waitForBytesWritten();
    Sleep(SLEEP_DURATION);      //按经验最好等待一个时间

    //判断是否完成更改
    if(RelayIndex==-1)   //如果是操作所有继电器
    {
        while( !(relay_1_Open==ONorOFF && relay_2_Open==ONorOFF && relay_3_Open==ONorOFF && relay_4_Open==ONorOFF &&
                 relay_5_Open==ONorOFF && relay_6_Open==ONorOFF && relay_7_Open==ONorOFF && relay_8_Open==ONorOFF) )
        {
            QThread::msleep(50);
            QCoreApplication::processEvents();
            if( CommandSendTime.msecsTo(currentDateTime_UorL(get_UsedTimeType())) > 5000 )  //如果while等待时间超过5秒，就重发指令
            {
                mySerialPort.write(frames);  //二次发送指令
                mySerialPort.waitForBytesWritten();
                Sleep(SLEEP_DURATION);      //按经验最好等待一个时间
                QDateTime CommandSendTime2 = currentDateTime_UorL(get_UsedTimeType());    //二次发送的时间
                while( !(relay_1_Open==ONorOFF && relay_2_Open==ONorOFF && relay_3_Open==ONorOFF && relay_4_Open==ONorOFF &&
                         relay_5_Open==ONorOFF && relay_6_Open==ONorOFF && relay_7_Open==ONorOFF && relay_8_Open==ONorOFF) )
                {
                    QThread::msleep(50);
                    QCoreApplication::processEvents();
                    //如果二次等待也超过5秒，就通知AOC电源操作指令执行失败
                    if( CommandSendTime2.msecsTo(currentDateTime_UorL(get_UsedTimeType())) > 5000 )
                    {
                        emit UI_Report_PauseAutoObserve();      //通知AOC暂停、UI更新暂停按钮为“继续”
                        QString Message;
                        if(ONorOFF==1)  Message = "两次尝试设置所有继电器为ON状态均失败！请使用手动模式完成控制后再继续自动观测";
                        else    Message = "两次尝试设置所有继电器为OFF状态均失败！请使用手动模式完成控制后再继续自动观测";
                        QMessageBox::warning(this, "电源控制器报错",Message);
                        qDebug()<< Message ;
                        return;
                    }
                }
                //如果跳出二次等待，也是成功了
                emit UI_PSC_Report_OperatePowerSwitch_Finish();    //通知-在AOC指挥下控制电源 完成
                qDebug()<<"二次设置全部继电器为"<<ONorOFF<<"状态成功！";
                return;
            }
        }
        //如果跳出一次等待，就说明接收到电源反馈信号，电源设置成功
        emit UI_PSC_Report_OperatePowerSwitch_Finish();    //通知-在AOC指挥下控制电源 完成
        qDebug()<<"一次设置全部号继电器为"<<ONorOFF<<"状态成功！";
        return;
    }
    else        //如果是操作单个继电器
    {
        while(*relayOpen_flag!=ONorOFF)
        {
            QThread::msleep(50);
            QCoreApplication::processEvents();
            if( CommandSendTime.msecsTo(currentDateTime_UorL(get_UsedTimeType())) > 5000 )  //如果while等待时间超过5秒，就重发指令
            {
                mySerialPort.write(frames);  //二次发送指令
                mySerialPort.waitForBytesWritten();
                Sleep(SLEEP_DURATION);      //按经验最好等待一个时间
                QDateTime CommandSendTime2 = currentDateTime_UorL(get_UsedTimeType());    //二次发送的时间
                while( *relayOpen_flag!=ONorOFF )
                {
                    QThread::msleep(50);
                    QCoreApplication::processEvents();
                    //如果二次等待也超过5秒，就通知AOC电源操作指令执行失败
                    if( CommandSendTime2.msecsTo(currentDateTime_UorL(get_UsedTimeType())) > 5000 )
                    {
                        emit UI_Report_PauseAutoObserve();      //通知AOC暂停、UI更新暂停按钮为“继续”
                        QString Message;
                        if(ONorOFF==1)  Message = "两次尝试设置"+QString::number(RelayIndex)+"号继电器为ON状态均失败！请使用手动模式完成控制后再继续自动观测";
                        else    Message = "两次尝试设置"+QString::number(RelayIndex)+"号继电器为OFF状态均失败！请使用手动模式完成控制后再继续自动观测";
                        QMessageBox::warning(this, "电源控制器报错",Message);
                        qDebug()<< Message ;
                        return;
                    }
                }
                //如果跳出二次等待，也是成功了
                emit UI_PSC_Report_OperatePowerSwitch_Finish();    //通知-在AOC指挥下控制电源 完成
                qDebug()<<"二次设置"<<RelayIndex<<"号继电器为"<<ONorOFF<<"状态成功！";
                return;
            }
        }
        //如果跳出一次等待，就说明接收到电源反馈信号，电源设置成功
        emit UI_PSC_Report_OperatePowerSwitch_Finish();    //通知-在AOC指挥下控制电源 完成
        qDebug()<<"一次设置"<<RelayIndex<<"号继电器为"<<ONorOFF<<"状态成功！";
        return;
    }

}


//slots: CC通知UI在子窗口中加载最新一张拍到的图片
void MainWindow::UI_Respond_DisplayLastImage(QString TifFileFullPath)
{
    m_LastImageFigure->DisplayLastImage(TifFileFullPath);
}




//slots: UI控制手动拍摄一张照片
void MainWindow::on_btn_ShootOneImage_clicked()
{
    QString CurWaveBand;
    if(ui->comboBox_changeSlotTo->currentText()=="862")
        CurWaveBand = "X";
    else if(ui->comboBox_changeSlotTo->currentText()=="630")
        CurWaveBand = "Y";
    else if(ui->comboBox_changeSlotTo->currentText()=="750-850")
        CurWaveBand = "Z";
    else if(ui->comboBox_changeSlotTo->currentText()=="000")
        CurWaveBand = "0";
    else
        CurWaveBand = "UnKnowWaveBand";
    QString CurDateTime = currentDateTime_UorL(get_UsedTimeType()).toString("yyyyMMddhhmmss");
    emit UI_Send_ShootOneImage("VFAJ_ASA01_IA"+CurWaveBand+"_L11_STP_"+CurDateTime);
}


//选择数据文件保存路径
void MainWindow::on_pushButton_2_clicked()
{
    // QFileDialog dialog;
    // dialog.setFileMode(QFileDialog::AnyFile);
    // ui->lineEdit_DataStorePath->setText(dialog.directoryUrl().toString());
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setFileMode(QFileDialog::Directory);
    //fileDialog->setDirectory("……");    //设置当前目录
    fileDialog->exec();
    auto selectDir = fileDialog->selectedFiles();
    if (selectDir.size()>0)
    {
        ui->lineEdit_DataStorePath->setText(selectDir.at(0));
        set_DataSaveDirPath(selectDir.at(0));
        qDebug() << "数据文件存储路径:" << selectDir.at(0);

        LINKAGECONTROL_DIRPATH_window = PathUtils::ExtractParentDirPath(get_DataSaveDirPath())+"/ASI_command_observation_control";
        qDebug() << "联动控制文件所在文件夹路径 = " << LINKAGECONTROL_DIRPATH_window;
    }

}

//选择观测模式
void MainWindow::on_btn_SelectObserveMode_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setFileMode(QFileDialog::ExistingFile);  //已存在的单个文件
    fileDialog->setNameFilter(tr("ObserveMode*.txt"));     //开启过滤器，只通过文件名形如ObserveMode*.txt的文件
    fileDialog->exec();
    auto selectDir = fileDialog->selectedFiles();
    if (selectDir.size()>0)
    {
        ui->lineEdit_ObserveModeFullPath->setText(selectDir.at(0));
        set_ObserveModeTXTFullPath(selectDir.at(0));
        qDebug() << "所选择观测模式定义文件路径:" << selectDir.at(0);
    }
}

//选择时间表
void MainWindow::on_btn_SelectTimeTable_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setFileMode(QFileDialog::ExistingFile);  //已存在的单个文件
    fileDialog->setNameFilter(tr("*.txt"));     //开启过滤器，只通过txt格式的文件
    fileDialog->exec();
    auto selectDir = fileDialog->selectedFiles();
    if (selectDir.size()>0)
    {
        ui->lineEdit_TimeTableFullPath->setText(selectDir.at(0));
        TIMETABLE_FULLPATH_window = selectDir.at(0);
        emit UI_Report_TimeTableFullPath(TIMETABLE_FULLPATH_window);
        qDebug() << "选择时间表路径:" << selectDir.at(0);
    }
}







//“清空消息”btn按下
void MainWindow::on_btn_ClearAllMessage_clicked()
{
    ui->textEdit_MessageOutput->clear();
}

//切换滤波轮slot
void MainWindow::on_comboBox_changeSlotTo_textActivated(const QString &arg1)
{
    qDebug()<<"当前comboBox_changeSlotTo = "<< arg1 ;
    if(arg1=="750-850")
        emit UI_Send_ChangeSlotTo(index_750850_window);
    else if(arg1=="862")
        emit UI_Send_ChangeSlotTo(index_862_window);
    else if(arg1=="630")
        emit UI_Send_ChangeSlotTo(index_630_window);
    else if(arg1=="000")
        emit UI_Send_ChangeSlotTo(index_000_window);
    else
        emit UI_Send_ChangeSlotTo(-1);  //表示索引的字符串错误
}


void MainWindow::on_checkBox_IfShowLIDW_stateChanged(int arg1)  //勾选，arg1是2。不选，arg1是0
{
    qDebug()<<"on_checkBox_IfShowLIDW_stateChanged . int arg1= "<<arg1;
    if(arg1 == 2)
    {
        m_LastImageFigure->show();
        m_LastImageFigure->setFocus();
        ui->checkBox_IfShowLIDW_2->setCheckState(Qt::Checked);  //同步两个checkBox的状态
    }
    else if(arg1 == 0)
    {
        m_LastImageFigure->hide();
        ui->checkBox_IfShowLIDW_2->setCheckState(Qt::Unchecked);  //同步两个checkBox的状态
    }

    //如果点击了窗口右上方的×，似乎这个widget对象也没有被删除？
    //qDebug()<<"m_LastImageFigure->isVisible() = "<<m_LastImageFigure->isVisible();
}


void MainWindow::on_checkBox_IfShowLIDW_2_stateChanged(int arg1)  //勾选，arg1是2。不选，arg1是0
{
    if(arg1 == 2)
    {
        m_LastImageFigure->show();
        m_LastImageFigure->setFocus();
        ui->checkBox_IfShowLIDW->setCheckState(Qt::Checked);  //同步两个checkBox的状态
    }
    else if(arg1 == 0)
    {
        m_LastImageFigure->hide();
        ui->checkBox_IfShowLIDW->setCheckState(Qt::Unchecked);  //同步两个checkBox的状态
    }
}



//改变 “世界时”checkBox状态
void MainWindow::on_checkBox_UseUTC_stateChanged(int arg1)
{
    if(arg1 == 2)
        set_UsedTimeType(UTC_Time);
}

//改变 “地方时”checkBox状态
void MainWindow::on_checkBox_UseLocalTime_stateChanged(int arg1)
{
    if(arg1 == 2)
        set_UsedTimeType(Local_Time);
}



//RadioBtn Binning1x1 状态改变
void MainWindow::on_radioButton_Binning1x1_toggled(bool checked)
{
    if(checked)  //仅在按钮状态变为true时进行操作
    {
        if(ui->btn_CameraConnect->isEnabled()==false)  //相机已连接：修改Binning值 并 提交到硬件
        {
            set_XYbinning(1);
            myCameraController->CCSetCameraXYBinning(get_XYbinning());
            //弹出自动淡出窗口输出提示信息
            MessageTips One_MessageTips;
            One_MessageTips.showMessageTips("设置像素合并值成功！当前Binning = 1x1",this);
        }
        else        //相机未连接：修改Binning值 ，但不提交到硬件（后续连接的时候会由CC提交Binning值到硬件）
        {
            set_XYbinning(1);
        }
    }

}

//RadioBtn Binning2x2 状态改变
void MainWindow::on_radioButton_Binning2x2_toggled(bool checked)
{
    if(checked)  //仅在按钮状态变为true时进行操作
    {
        if(ui->btn_CameraConnect->isEnabled()==false)  //相机已连接：修改Binning值 并 提交到硬件
        {
            set_XYbinning(2);
            myCameraController->CCSetCameraXYBinning(get_XYbinning());
            //弹出自动淡出窗口输出提示信息
            MessageTips One_MessageTips;
            One_MessageTips.showMessageTips("设置像素合并值成功！当前Binning = 2x2",this);
        }
        else        //相机未连接：修改Binning值 ，但不提交到硬件（后续连接的时候会由CC提交Binning值到硬件）
        {
            set_XYbinning(2);
        }
    }
}

//RadioBtn Binning4x4 状态改变
void MainWindow::on_radioButton_Binning4x4_toggled(bool checked)
{
    if(checked)  //仅在按钮状态变为true时进行操作
    {
        if(ui->btn_CameraConnect->isEnabled()==false)  //相机已连接：修改Binning值 并 提交到硬件
        {
            set_XYbinning(4);
            myCameraController->CCSetCameraXYBinning(get_XYbinning());
            //弹出自动淡出窗口输出提示信息
            MessageTips One_MessageTips;
            One_MessageTips.showMessageTips("设置像素合并值成功！当前Binning = 4x4",this);
        }
        else        //相机未连接：修改Binning值 ，但不提交到硬件（后续连接的时候会由CC提交Binning值到硬件）
        {
            set_XYbinning(4);
        }
    }
}

//RadioBtn Binning8x8 状态改变
void MainWindow::on_radioButton_Binning8x8_toggled(bool checked)
{
    if(checked)  //仅在按钮状态变为true时进行操作
    {
        if(ui->btn_CameraConnect->isEnabled()==false)  //相机已连接：修改Binning值 并 提交到硬件
        {
            set_XYbinning(8);
            myCameraController->CCSetCameraXYBinning(get_XYbinning());
            //弹出自动淡出窗口输出提示信息
            MessageTips One_MessageTips;
            One_MessageTips.showMessageTips("设置像素合并值成功！当前Binning = 8x8",this);
        }
        else        //相机未连接：修改Binning值 ，但不提交到硬件（后续连接的时候会由CC提交Binning值到硬件）
        {
            set_XYbinning(8);
        }
    }
}


//快捷键被按下后，UI线程开启/关闭联动控制
void MainWindow::UI_Respond_LinkageControlONorOFF()
{
    if(UI_IfLinkageControlOpened)   //如果之前是开启的，就关闭
    {
        qDebug()<<"关闭联动控制！";
        UI_IfLinkageControlOpened = false;
        ui->checkBox_UseLocalTime->setStyleSheet("QCheckBox { font-weight: normal; }");
        ui->checkBox_UseUTC->setStyleSheet("QCheckBox { font-weight: normal; }");
    }
    else                            //如果之前是关闭的，就开启
    {
        qDebug()<<"开启联动控制！";
        UI_IfLinkageControlOpened = true;
        ui->checkBox_UseLocalTime->setStyleSheet("QCheckBox { font-weight: bold; }");
        ui->checkBox_UseUTC->setStyleSheet("QCheckBox { font-weight: bold; }");
    }
}

//slots: CC通知UI相机开始采集数据
void MainWindow::UI_Respond_StartAcquireData(QDateTime StartAcquireTime)
{
    m_AcquireProgressBar_StartTime = StartAcquireTime;  //更新进度条起始时间

    ui->progressBar_ShootProgress->show();  //拍摄进度条 恢复显示
    ui->progressBar_ShootProgress_2->show();  //拍摄进度条 恢复显示
    ui->progressBar_ShootProgress->setValue(0);  //拍摄进度条 置零
    ui->progressBar_ShootProgress_2->setValue(0);  //拍摄进度条 置零

    UI_IfCameraIsAcquiring = true;         //相机开始采集数据，更新UI中的标志位

    ui->btn_ShootOneImage->setEnabled(false);  //相机开始采集后，禁用“拍摄”按钮
}
