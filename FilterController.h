#pragma execution_character_set("utf-8")

#ifndef FILTERCONTROLLER_H
#define FILTERCONTROLLER_H

#include <QObject>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>
#include <QMessageBox>
#include <QTimer>
#include "EFW_filter.h"
#include "pathutils.h"

class FilterController : public QObject
{
    Q_OBJECT
public:
    explicit FilterController(QObject *parent = nullptr);

    bool get_FilterIsConnected(){return m_FilterIsConnected;}
    void set_FilterIsConnected(bool IfConnected){m_FilterIsConnected = IfConnected;}

    void read_FilterPreSetParam();  //从FilterParam.txt中读取各波段滤波片对应的孔位index

    //操作滤波轮
    bool ConnectAndInitFilter();    //尝试连接滤波轮
    bool DisConnectFilter();        //断开滤波轮的连接
    bool Get_CurFilterInfo();       //获取当前滤波轮的信息（ID、name、slot总数）
    int  Get_CurSlotsIndex();       //获取当前滤波轮通孔处的槽编号，该值介于0到M-1之间，M为通孔数量.-1: 滤轮在移动;-2:获取失败 。
    bool MoveToOneSlot(int TargetSlotIndex);   //将指定Index（0~M-1）的槽转到通孔处

    //把ErrorCode枚举类型的值转为QString
    QString TransEFWErrorCodeToQString(EFW_ERROR_CODE ErrorCode);

    //###################################
    void FilterControllerMain();  //FC主工作函数，UI上的连接滤波轮btn connect到此函数
    //###################################

private:

    bool m_FilterIsConnected = false;  //滤波轮是否连接

    int m_AccessibleEFWFilterNum = 0;   //此主机当前可以访问的EFW滤波轮总数n
    int m_CurFilter_Index = 0;              //决定访问的滤波轮在所有可用滤波轮中的index(0 ~ n-1)
    int m_CurFilter_SlotsNum = 0;         //滤波轮上的通孔总数
    int m_CurFilter_ID = 0;          //当前使用的滤波轮的ID
    EFW_INFO m_CurFilter_Info;       //EFW滤波轮的信息（ID、name、通孔数量）
    bool m_CurFilterUniDirectional = false; //是否单方向旋转标记。如果设置为true，滤波轮将始终沿同一个方向旋转
    int m_CurSlotIndex = 0;             //当前处于通孔处的slot编号，该值介于0到M-1之间，M为通孔数量，如果滤轮在移动，该值为-1。
    int m_TargetSlotIndex = 0;          //希望挪到通孔处的slot的Index

    int m_750850index = 0;      //从FilterParam.txt读取的750-850nm滤波片的slot_index
    int m_862index = 1;         //从FilterParam.txt读取的862nm滤波片的slot_index
    int m_630index = 2;         //从FilterParam.txt读取的630nm滤波片的slot_index
    int m_000index = 3;         //遮光片的slot_index

    QString m_FilterParamTXT_FullPath = PathUtils::getRootPath()+"/Setting/FilterParam.txt";  //滤波轮设置文件FilterParam.txt的完整路径（含文件名）

    QTimer* m_TimerPtr; //FC的专属定时器的指针

signals:
    void FC_Report_Error(QString ErrorInfo); //FC向MainWindow通告错误信息,引发弹窗

    void FC_Report_NewCurSlotIndex(int CurIndex);  //FC向UI通知滤波轮的当前slot编号

    void FC_Report_PauseAutoObserve();  //FC通知AOC阻塞自身，停止自动观测
    void FC_Report_OperateFilter_Finish(); //FC通知AOC滤波轮操作已完成


public slots:

    //手动控制槽函数
    void FC_Respond_ChangeSlotTo(int index);    //slots：手动控制切换滤波片


    //AOC专用槽函数
    void FC_Respond_OperateFilter(QString CurWaveBand);  //AOC指挥FC将滤波轮转到指定位置

private slots:
    void onTimeout(); //FC Timer超时后执行

};

#endif // FILTERCONTROLLER_H
