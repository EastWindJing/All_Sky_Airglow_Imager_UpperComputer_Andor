#include "AutoObserveController.h"
#include "GlobalVariable.h"

//构造函数
AutoObserveController::AutoObserveController(QObject *parent)
    : QObject{parent}
{

}

//读取时间表TimeTable.txt，TimeTableTXTFullPath 包含文件名
bool AutoObserveController::ReadTimeTableTXT(QString TimeTableTXTFullPath)
{
    QFile TimeTable_file(TimeTableTXTFullPath);

    // 打开文件，使用只读模式和文本模式
    if (!TimeTable_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString ErrorMessage = "无法打开TimeTable.txt，读取时间表失败！\n错误信息：" + TimeTable_file.errorString();
        emit AOC_Report_Error(ErrorMessage);
        emit AOC_Send_MessageBoxOutput(ErrorMessage);  //AOC向UI的MessageBox发送消息
        qDebug() << ErrorMessage;
        return false;
    }

    //Qt中读写文本文件通常使用QFile与QTextStream 结合，因为QTextStream 可以逐行读取，处理数据要相对方便一些.
    QTextStream FileStream(&TimeTable_file); //将QFile对象与QTextStream对象关联起来.

    // 逐行读取文件内容
    while (!FileStream.atEnd())
    {
        //QTextStream::readLine()从流中读取一行文本，并将其作为QString返回
        QString OneLine = FileStream.readLine();
        //split("-")函数将OneLine字符串按照“-”字符拆分成两个子字符串，并将它们存储在QStringList对象TwoParts中。
        QStringList TwoParts = OneLine.split("-");
        // 检查读取结果，确保每行都包含一个开始时间和一个结束时间
        if (TwoParts.size() != 2) continue; //如果当前行缺少了开始时间或结束时间，直接跳过，去读取下一行

        // // 解析开始时间和结束时间，格式为"yyyy MM dd HH mm ss"
        // QDateTime StartDateTime = QDateTime::fromString(TwoParts[0], "yyyy MM dd HH mm ss");
        // QDateTime EndDateTime = QDateTime::fromString(TwoParts[1], "yyyy MM dd HH mm ss");

        // 根据 g_UsedTimeType 读取时间
        QDateTime StartDateTime, EndDateTime;
        if (get_UsedTimeType() == UTC_Time)
        {
            StartDateTime = QDateTime::fromString(TwoParts[0], "yyyy MM dd HH mm ss");
            EndDateTime = QDateTime::fromString(TwoParts[1], "yyyy MM dd HH mm ss");
            StartDateTime.setTimeSpec(Qt::UTC);  // 设置时间规范为UTC
            EndDateTime.setTimeSpec(Qt::UTC);  // 设置时间规范为UTC
        }
        else if (get_UsedTimeType() == Local_Time)
        {
            StartDateTime = QDateTime::fromString(TwoParts[0], "yyyy MM dd HH mm ss").toLocalTime();
            EndDateTime = QDateTime::fromString(TwoParts[1], "yyyy MM dd HH mm ss").toLocalTime();
            StartDateTime.setTimeSpec(Qt::LocalTime);  // 设置时间规范为UTC
            EndDateTime.setTimeSpec(Qt::LocalTime);  // 设置时间规范为UTC
        }

        // 将解析出的 <开始-结束>时间对 添加到m_TimeTable
        m_TimeTable.append(qMakePair(StartDateTime, EndDateTime));
    }

    //检查读取结果是否为空
    if(m_TimeTable.isEmpty())   //如果m_TimeTable是空的，则读取失败
    {
        QString ErrorMessage = "m_TimeTable为空，时间表读取失败！";
        emit AOC_Report_Error(ErrorMessage);
        emit AOC_Send_MessageBoxOutput(ErrorMessage);  //AOC向UI的MessageBox发送消息
        qDebug()<< ErrorMessage;
        return false;
    }
    else                        //如果读取m_TimeTable成功
    {
        QString Message = "时间表文件"+ TimeTableTXTFullPath +"读取成功！m_TimeTable内有 "
                          + QString::number(m_TimeTable.size()) + " 个<StartDateTime, EndDateTime>对。";
        emit AOC_Send_MessageBoxOutput(Message);  //AOC向UI的MessageBox发送消息
        qDebug().noquote()<< Message;
        return true;
    }

}

//从时间表中提取出所有起始时间大于currentTime的时段，它们就是剩余工作时段
void AutoObserveController::ExtractRemainWorkPeriodList()
{
    QDateTime currentDateTime = currentDateTime_UorL(get_UsedTimeType());

    //遍历m_TimeTable，找出所有结束时间>currentTime，并且开始时间<结束时间的时段
    for (int i = 0; i < m_TimeTable.size(); ++i)
    {
        const auto &period = m_TimeTable[i];
        if ( (currentDateTime < period.second) && (period.first<period.second) )  // 如果当前时间在m_TimeTable的第i个工作时段内
        {
            m_RemainWorkPeriodList.append(period);
        }
    }
    std::sort(m_RemainWorkPeriodList.begin(), m_RemainWorkPeriodList.end());  //std::sort默认升序排列
    if(m_RemainWorkPeriodList.size()<1)
    {
        QString Message = "时间表中有效工作时段数量<1, 无法开启自动观测！请更改时间表内容后重试。";
        emit AOC_Report_Error(Message);
        emit AOC_Send_MessageBoxOutput(Message);
        qDebug()<<Message;
        emit AOC_Report_PauseAutoObserve();   //通知UI更新暂停按钮为“继续”
        PauseAutoObserve();
        return;
    }

    m_RemainWorkPeriodNum = m_RemainWorkPeriodList.size();

    for (int i = 0; i < m_RemainWorkPeriodList.size(); ++i)
    {
        QString WorkPeriod_i = "有效工作时段[" + QString::number(i+1) + "] = " +
                                m_RemainWorkPeriodList[i].first.toString("yyyy年 MM月 dd日 HH:mm:ss") + "--" +
                                m_RemainWorkPeriodList[i].second.toString("yyyy年 MM月 dd日 HH:mm:ss");
        emit AOC_Send_MessageBoxOutput(WorkPeriod_i);
        qDebug()<<WorkPeriod_i;
    }

}


//读取观测模式文件ObserveMode_index.txt，ObserveModeTXTPath 含文件名
bool AutoObserveController::ReadObserveModeTXT(QString ObserveModeTXTFullPath)
{

    QFile ObserveMode_file(ObserveModeTXTFullPath);

    // 打开文件，使用只读模式和文本模式
    if (!ObserveMode_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString ErrorMessage = "无法打开观测模式定义文件！\n错误信息：" + ObserveMode_file.errorString();
        emit AOC_Report_Error(ErrorMessage);
        emit AOC_Send_MessageBoxOutput(ErrorMessage);  //AOC向UI的MessageBox发送消息
        qDebug().noquote() << ErrorMessage;
        return false;
    }

    QTextStream FileStream(&ObserveMode_file); //将QFile对象与QTextStream对象关联起来.

    // 逐行读取文件内容
    while (!FileStream.atEnd())
    {
        //QTextStream::readLine()从流中读取一行文本，并将其作为QString返回
        QString OneLine = FileStream.readLine();
        //split(">")函数将OneLine字符串按照“>”字符拆分成两个子字符串，并将它们存储在QStringList对象OperatedObject_and_OperateValue中。
        QStringList OperatedObject_and_OperateValue = OneLine.split(">");   // ModeSectionOne 格式：操作对象 > 操作值
        // 检查读取结果属于那个Section
        if (OperatedObject_and_OperateValue.size() == 2)     //属于ModeSectionOne
        {
            m_ModeSectionOne.append( qMakePair(OperatedObject_and_OperateValue[0], OperatedObject_and_OperateValue[1]) );
        }
        else  //不属于 ModeSectionOne
        {
            OperatedObject_and_OperateValue = OneLine.split("=");           // ModeSectionTwo 格式：操作对象 = 操作值
            if(OperatedObject_and_OperateValue.size() == 2)  //属于ModeSectionTwo
            {
                m_ModeSectionTwo.append( qMakePair(OperatedObject_and_OperateValue[0], OperatedObject_and_OperateValue[1]) );
            }
            else    //不属于ModeSectionTwo
            {
                OperatedObject_and_OperateValue = OneLine.split(":");       // ModeSectionThree 格式：操作对象 : 操作值
                if(OperatedObject_and_OperateValue.size() == 2)  //属于ModeSectionThree
                {
                    m_ModeSectionThree.append( qMakePair(OperatedObject_and_OperateValue[0], OperatedObject_and_OperateValue[1]) );
                }
                else
                {
                    OperatedObject_and_OperateValue = OneLine.split("#");       // m_SectionRepeatTimes 格式：REPEAT_TIMES # 重复次数
                    if( (OperatedObject_and_OperateValue.size() == 2) && (OperatedObject_and_OperateValue[0]=="REPEAT_TIMES") )
                    {                                           //属于 m_SectionRepeatTimes
                        m_SectionRepeatTimes = OperatedObject_and_OperateValue[1].toInt();
                    }
                    else
                    {
                        continue;  //不属于任何一个Section，直接结束，去读下一行
                    }
                }
            }
        }
    }
    // --- while() end ---

    //检查读取结果是否为空
    if( m_ModeSectionOne.isEmpty() || m_ModeSectionTwo.isEmpty() || m_ModeSectionThree.isEmpty() )  //如果任何一个Section是空的，则读取失败
    {
        QString ErrorMessage;
        if( m_ModeSectionOne.isEmpty() )
            ErrorMessage = "读取到ModeSectionOne 为空，此观测模式不完整！请检查模式定义文件：" + ObserveModeTXTFullPath;
        else if( m_ModeSectionTwo.isEmpty() )
            ErrorMessage = "读取到ModeSectionTwo 为空，此观测模式不完整！请检查模式定义文件：" + ObserveModeTXTFullPath;
        else    // m_ModeSectionThree.isEmpty()
            ErrorMessage = "读取到ModeSectionThree 为空，此观测模式不完整！请检查模式定义文件：" + ObserveModeTXTFullPath;

        emit AOC_Report_Error(ErrorMessage);
        emit AOC_Send_MessageBoxOutput(ErrorMessage);  //AOC向UI的MessageBox发送消息
        qDebug().noquote()<< ErrorMessage;
        return false;
    }
    else  //如果三个Section都不为空，则认为ObserveMode_index.txt读取成功
    {
        QString Message = "观测模式定义文件 " + ObserveModeTXTFullPath + " 读取成功!\n第一节有 "+
                          QString::number(m_ModeSectionOne.size()) +" 个<操作对象，操作值>对；\n"+
                          "第二节有 "+ QString::number(m_ModeSectionTwo.size()) + " 个<操作对象，操作值>对；\n第三节有 "+
                          QString::number(m_ModeSectionThree.size())+ "个<操作对象，操作值>对.";
        emit AOC_Send_MessageBoxOutput(Message);  //AOC向UI的MessageBox发送消息
        qDebug().noquote()<< Message;
        return true;
    }

}


//执行当前所选观测模式的一个section
void AutoObserveController::ExecuteCurObserveModeOneSection(int SectionNum)
{
    int SectionRepeatTimes = 1;  //第一节和第二节默认重复一次
    int CurRepeatTimes = 0;  //当前是第几次重复
    QVector<QPair<QString,QString>>* SelectedSection = nullptr;
    switch (SectionNum)
    {
    case 1:
        SelectedSection = &m_ModeSectionOne;
        break;
    case 2:
        SelectedSection = &m_ModeSectionTwo;
        SectionRepeatTimes = m_SectionRepeatTimes;  //如果是第二节，就把读取到的重复次数设置生效
        break;
    case 3:
        SelectedSection = &m_ModeSectionThree;
        break;
    // default:
    //     QString ErrorMessage = "AutoObserveController::ExecuteCurObserveModeOneSection(int SectionNum)参数选择错误！请保证SectionNum=1,2,3.";
    //     emit AOC_Report_Error(ErrorMessage);
    //     qDebug()<<ErrorMessage;
    //     break;
    }



    QString str1 ="********************** 观测模式"+ QString::number(m_CurMode_index) +"的Section"+QString::number(SectionNum) +"开始执行：**********************";
    qDebug()<<str1;
    emit AOC_Send_MessageBoxOutput(str1);

    if(SectionRepeatTimes == -1)
    {
        QString str2 ="*************************** 本节一共计划重复 ∞ 次 ***************************";
        emit AOC_Send_MessageBoxOutput(str2);
        qDebug()<<str2;
    }
    else
    {
        QString str3 ="*************************** 本节一共计划重复"+ QString::number(SectionRepeatTimes) +"次 ***************************";
        emit AOC_Send_MessageBoxOutput(str3);
        qDebug()<<str3;
    }

    m_WhetherSectionContinueRepeat = true; //确保能进循环

    while(m_WhetherSectionContinueRepeat)
    {
        ++CurRepeatTimes;
        QString str4 = "***************************** 以下是第 "+QString::number(CurRepeatTimes) +" 次重复 ************************";
        emit AOC_Send_MessageBoxOutput(str4);
        qDebug()<<str4;

        //遍历Section容器, 把当前Section中的指令按顺序全部执行一遍
        for (const auto& OperatePair : *SelectedSection)
        {
            //没执行一个动作之前都检查当前时间是否在工作时段
            QDateTime CurDateTime = currentDateTime_UorL(get_UsedTimeType());
            bool InWorkPeriod = (CurDateTime>m_CurWorkingStartAndEndTime.first) && (CurDateTime<m_CurWorkingStartAndEndTime.second);
            if( (!InWorkPeriod)&&(SectionNum==1 ||SectionNum==2) )  //只有当：不在工作时段 && 执行的是第1或2节 时，需要中途自动退出
            {
                m_WhetherSectionContinueRepeat = false;
                break;
            }
            const QString& OperatedObject = OperatePair.first;  //被操作对象的const引用
            const QString& OperateValue = OperatePair.second;   //操作值的const引用

            //-------------------------------------------------------------------------------------------//
            if (OperatedObject == "POWER")
            {
                if (OperateValue == "ALL_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(-1,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "1_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(1,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "2_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(2,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "3_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(3,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "4_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(4,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "5_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(5,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "6_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(6,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "7_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(7,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "8_ON")
                {
                    emit AOC_Send_OperatePowerSwitch(8,1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "ALL_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(-1,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "1_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(1,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "2_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(2,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "3_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(3,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "4_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(4,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "5_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(5,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "6_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(6,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "7_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(7,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
                else if (OperateValue == "8_OFF")
                {
                    emit AOC_Send_OperatePowerSwitch(8,0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                    while(!m_IfOperatePowerSwitch_Finished)
                    {
                        QThread::msleep(20);
                        QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                    }
                    m_IfOperatePowerSwitch_Finished = false;
                }
            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "FILTER")
            {
                if (OperateValue == "862")
                {
                    emit AOC_Send_OperateFilter("862");
                    m_FileNamingInfo.CurWaveBand = "X";  //更新命名信息
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                else if (OperateValue == "630")
                {
                    emit AOC_Send_OperateFilter("630");
                    m_FileNamingInfo.CurWaveBand = "Y";  //更新命名信息
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                else if (OperateValue == "750-850")
                {
                    emit AOC_Send_OperateFilter("750-850");
                    m_FileNamingInfo.CurWaveBand = "Z";  //更新命名信息
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                else if (OperateValue == "000")
                {
                    emit AOC_Send_OperateFilter("000");
                    m_FileNamingInfo.CurWaveBand = "0";  //更新命名信息
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                while(!m_IfOperateFilter_Finished)
                {
                    QThread::msleep(20);
                    QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                }
                m_IfOperateFilter_Finished = false;
                m_CurFilter = OperateValue;
            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "CAMERA_TEMPERATURE")
            {
                emit AOC_Send_OperateCameraTemperature(OperateValue.toFloat());
                emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                //PauseAutoObserve();  //阻塞AOC线程
                while(!m_IfOperateCameraTemperature_Finished)
                {
                    QThread::msleep(20);
                    QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                }
                m_IfOperateCameraTemperature_Finished = false;
            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "CAMERA_SHUTTERTIMINGMODE")
            {
                if (OperateValue == "Auto")
                {
                    emit AOC_Send_OperateCameraShutterTimingMode(0);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                else if (OperateValue == "AlwaysClose")
                {
                    emit AOC_Send_OperateCameraShutterTimingMode(1);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                else if (OperateValue == "AlwaysOpen")
                {
                    emit AOC_Send_OperateCameraShutterTimingMode(2);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                while(!m_IfOperateCameraShutterTimingMode_Finished)
                {
                    QThread::msleep(20);
                    QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                }
                m_IfOperateCameraShutterTimingMode_Finished = false;
                m_CurCameraShuttingTimingMode = OperateValue;
            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "CAMERA_EXPOSURETIME")
            {
                emit AOC_Send_OperateCameraExposureTime(OperateValue.toFloat()*1000);
                emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;

                while(!m_IfOperateCameraExposureTime_Finished)
                {
                    QThread::msleep(20);
                    QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                }
                m_IfOperateCameraExposureTime_Finished = false;
                m_CurExposureTime = OperateValue.toFloat()*1000;  //更新当前曝光时间，后续传给AOC_Send_OperateCameraAcquireAndSave()
            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "CAMERA_ACQUIRE")
            {
                m_ShoottingDateTime = currentDateTime_UorL(get_UsedTimeType());  //获取开始拍摄的时刻
                QString DataFileName = m_FileNamingInfo.VFAJ_ASA01_IA + m_FileNamingInfo.CurWaveBand + m_FileNamingInfo._L11_STP_ + m_ShoottingDateTime.toString("yyyyMMddhhmmss");
                emit AOC_Send_OperateCameraAcquireAndSave( DataFileName, m_CurExposureTime );
                emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue);
                qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                while(!m_IfOperateCameraAcquireAndSave_Finished)
                {
                    QThread::msleep(20);
                    QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                }
                m_IfOperateCameraAcquireAndSave_Finished = false;

                if(SectionNum == 1)
                    m_AutoObserveState = "Start";
                else if(SectionNum == 2)
                    m_AutoObserveState = "Working";
                else if(SectionNum == 3)
                    m_AutoObserveState = "End";

                m_LatestLogEntry = m_ShoottingDateTime.toString("yyyy-MM-dd , hh:mm:ss") + " , " + DataFileName +".tif"+" , " +
                                      m_CurFilter +" , " + QString::number(m_CurExposureTime/1000)+ " , " + m_AutoObserveState;

                WriteObservLogTXT(get_DataSaveDirPath(), m_LatestLogEntry);  //每拍摄完一张照片就添加一条日志记录
            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "CAMERA_CONNECT")
            {
                if (OperateValue == "Connect")
                {
                    emit AOC_Send_OperateCameraConnect(true);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }
                else if (OperateValue == "Disconnect")
                {
                    emit AOC_Send_OperateCameraConnect(false);
                    emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                    qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                }

                while(!m_IfOperateCameraConnect_Finished)
                {
                    QThread::msleep(20);
                    QCoreApplication::processEvents(); //让线程去响应其他事件(MC、CC通知AOC更新 模式动作Finish标志)
                }
                m_IfOperateCameraConnect_Finished = false;

            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "SLEEP")
            {
                emit AOC_Send_MessageBoxOutput( "操作对象：" + OperatedObject + ", 操作值：" + OperateValue );
                qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
                QThread::msleep(OperateValue.toInt() * 1000);
            }
            //-------------------------------------------------------------------------------------------//
            else if(OperatedObject == "REPEAT_TIMES")
            {
                m_SectionRepeatTimes = OperateValue.toInt();
                qDebug().noquote()<< "操作对象：" + OperatedObject + ", 操作值：" + OperateValue;
            }
            //-------------------------------------------------------------------------------------------//
            //让AOC线程处理来自UI的信号和其他待处理事件
            QCoreApplication::processEvents(); //该函数的作用是让程序处理那些还没有处理的事件，然后再把使用权返回给调用者。
        }
        //-------------------- for() end ------------------------


        if( --SectionRepeatTimes == 0 )       //如果重复次数用完，就退出循环(只有在从1->0时才会退出，如果一开始就是-1则会一直循环)
            m_WhetherSectionContinueRepeat = false;
        //（另一种可能是：当前时间出了观测时段，m_WhetherSectionContinueRepeat被置为false）

        //让AOC线程处理来自UI的信号和其他待处理事件
        QCoreApplication::processEvents(); //该函数的作用是让程序处理那些还没有处理的事件，然后再把使用权返回给调用者。
    }
    //---------------------- while() end------------------------
    QString str5 = "********* 观测模式" + QString::number(m_CurMode_index) + "的Section" + QString::number(SectionNum) +"执行完毕！*********";
    emit AOC_Send_MessageBoxOutput(str5);
    qDebug()<<str5;
    // if(SectionNum ==3)
    //     emit AOC_Send_CurWorkingStartAndEndTime("--","--");  //AOC向UI通告当前工作时段

}



//在存数据的文件夹中，每天书写一个日志文件ObservLog.TXT
void AutoObserveController::WriteObservLogTXT(QString DataSavePath,QString LogEntry)
{
    //自动观测时，无论是否跨天，数据文件一律存在开始日期对应的文件夹内，日志也要对应上。
    QString CurrentDay;
    if(get_isWorkingUnderAOC())
        CurrentDay = get_CurWorkingPeriodStartDate().toString("yyyyMMdd");    //获取当前工作时段的起始日期
    else
        CurrentDay = currentDateTime_UorL(get_UsedTimeType()).date().toString("yyyyMMdd");  //获取当前日期

    //QString targetDir = DataSavePath + "/" + CurrentYear + "/" + CurrentMonth + "/" + CurrentDay;
    QString targetDir = DataSavePath + "/" + CurrentDay;

    // 检查目标目录是否存在，若没有就新建
    QDir dir;
    if (!dir.exists(targetDir))  //如果不存在
    {
        if (!dir.mkpath(targetDir))
        {
            QString ErrorMessage = "函数AutoObserveController::WriteObservLogTXT()自动新建文件夹:" + targetDir + "失败！";
                                   emit AOC_Report_Error(ErrorMessage);
            qDebug()<< ErrorMessage;
            return;
        }
    }

    // 获取当天文件夹下所有以".txt"结尾的文件
    QStringList FileFilters;
    FileFilters << "*.txt";     //设置文件过滤器以获取所有以.txt结尾的文件。
    QDir CurDaydDir(targetDir);
    CurDaydDir.setNameFilters(FileFilters);
    QFileInfoList TXTFileList = CurDaydDir.entryInfoList();

    // 检查是否有以"VFAJ_ASA01_DLG_L11_STP_"开头的txt文件
    bool LogFileExists = false;
    QString LogFileName = "";
    foreach (const QFileInfo &fileInfo, TXTFileList)
    {
        if (fileInfo.fileName().startsWith(m_ObserveLogTXTPrefixName))
        {
            LogFileExists = true;
            LogFileName =fileInfo.fileName();
            break;
        }
    }

    // 根据日志文件是否存在采取不同的行为
    if(LogFileExists == true)    // 如果日志文件存在
    {
        QString LogFileFullPath = targetDir + "/" + LogFileName;
        QFile file(LogFileFullPath);
        //则以追加方式打开
        if (!file.open(QIODevice::Append | QIODevice::Text))  //如果打开log文件失败
        {
            QString ErrorMessage = "无法打开日志文件追加日志条目！\n详细错误信息：" + file.errorString();
            emit AOC_Report_Error(ErrorMessage);
            qDebug()<<ErrorMessage;
            return;
        }
        // 使用 QTextStream 写入文本
        QTextStream out(&file);
        out.setCodec("UTF-8"); // 设置编码格式为 UTF-8
        out << LogEntry << Qt::endl;
        file.close();
        QString Message = "成功添加了一条日志记录！";
        qDebug()<<Message;
    }
    else        // 如果日志文件不存在
    {
        QString LogFileFullPath = targetDir + "/" + m_ObserveLogTXTPrefixName +
                                  currentDateTime_UorL(get_UsedTimeType()).toString("yyyyMMddhhmmss") +".txt";
        QFile file(LogFileFullPath);
        //创建新文件并打开
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QString ErrorMessage = "无法创建日志文件并添加条目！\n详细错误信息：" + file.errorString();
            emit AOC_Report_Error(ErrorMessage);
            qDebug()<<ErrorMessage;
            return;
        }
        // 使用 QTextStream 写入文本
        QTextStream out(&file);
        out << "AcquireTime , DateFileName , CurWaveBand , ExposureTime , AutoObserveState"<< Qt::endl;
        out << LogEntry << Qt::endl;
        file.close();
        QString Message = "成功添加了一条日志记录！";
        qDebug()<<Message;
    }


    // // 检查日志文件是否存在
    // QFile file(LogFileFullPath);
    // if (file.exists())  // 如果文件存在
    // {
    //     //则以追加方式打开
    //     if (!file.open(QIODevice::Append | QIODevice::Text))  //如果打开log文件失败
    //     {
    //         QString ErrorMessage = "无法打开日志文件追加日志条目！\n详细错误信息：" + file.errorString();
    //         emit AOC_Report_Error(ErrorMessage);
    //         qDebug()<<ErrorMessage;
    //         return;
    //     }
    //     // 使用 QTextStream 写入文本
    //     QTextStream out(&file);
    //     out << LogEntry << Qt::endl;
    //     file.close();
    //     QString Message = "成功添加了一条日志记录！";
    //     qDebug()<<Message;

    // }
    // else  //如果日志文件不存在
    // {
    //     //创建新文件并打开
    //     if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    //     {
    //         QString ErrorMessage = "无法创建日志文件并添加条目！\n详细错误信息：" + file.errorString();
    //         emit AOC_Report_Error(ErrorMessage);
    //         qDebug()<<ErrorMessage;
    //         return;
    //     }
    //     // 使用 QTextStream 写入文本
    //     QTextStream out(&file);
    //     out << "AcquireTime , DateFileName , CurWaveBand , ExposureTime , AutoObserveState"<< Qt::endl;
    //     out << LogEntry << Qt::endl;
    //     file.close();
    //     QString Message = "成功添加了一条日志记录！";
    //     qDebug()<<Message;
    // }

}


//*************************************************************************
//******* AOC主工作函数，UI上的“开始自动观测btn_clicked” connect到此函数 *******
//*************************************************************************
void AutoObserveController::AutoObserveControllerMain()
{
    //检查相机、滤波轮、电源是否都已连接
    //if(m_IfCameraConnected && m_IfMotorControllerConnedted  && m_IfPowerConnedted)
    if(1)
    {
        //子线程的事件循环默认是开启的
        qDebug() << "进入AutoObserveController子线程，子线程对象地址: " << QThread::currentThread();
        emit AOC_Send_MessageBoxOutput(" ！！！--- 开始自动观测 --- ！！！");


        //先清空程序中存储的上次读取的时间表和模式、有效工作时段
        QVector<QPair<QDateTime, QDateTime>>().swap(m_TimeTable);
        QVector<QPair<QDateTime, QDateTime>>().swap(m_RemainWorkPeriodList);
        QVector<QPair<QString,QString>>().swap(m_ModeSectionOne);
        QVector<QPair<QString,QString>>().swap(m_ModeSectionTwo);
        QVector<QPair<QString,QString>>().swap(m_ModeSectionThree);

        QThread::msleep(50);

        //读取时间表
        ReadTimeTableTXT(m_TimeTableTXTFullPath);
        emit AOC_Send_MessageBoxOutput("加载时间表……");
        //从时间表中提取有效工作时段、更新剩余工作时段数量
        ExtractRemainWorkPeriodList();

        //读取选定的观测模式
        ReadObserveModeTXT(get_ObserveModeTXTFullPath());
        emit AOC_Send_MessageBoxOutput("加载观测模式……");

        m_pause = false;   //确保 线程暂停标志 为false
        //m_stopped = false;  //确保 线程停止标志 为false

        //创建并开启定时器
        int timeoutPeriod = 200;  //定时器超时周期（ms）
        m_TimerPtr = new QTimer; //创建AOC专属定时器
        m_TimerPtr->setInterval(timeoutPeriod);//每timeoutPeriod ms超时一次
        connect(m_TimerPtr, SIGNAL(timeout()), this, SLOT(onTimeout()));
        m_TimerPtr->start(); //启动定时器
        qDebug()<<"AOC专属定时器启动，超时周期 = " + QString::number(timeoutPeriod) + "ms";

        //？？？？？？？？？？？？？？？？？？？？？？？？？？？？
        //？？？这个循环会占用AOC线程，连定时器也无法工作？？？
        //？？？？？？？？？？？？？？？？？？？？？？？？？？？？
        //？？把这部分工作移到超时函数中？？？
        // while(1)
        // {
        //     // 检查是否需要暂停
        //    //m_mutex.lock(); // 锁住互斥量
        //     //if (m_pause)    //如果m_pause==1，那就阻塞AOC线程
        //         //m_pauseCondition.wait(&m_mutex);
        //         //m_mutex将被解锁，并且调用wait()函数的线程将会阻塞，直到 另一个线程使用wakeAll()传输给它 才会唤醒阻塞的线程，并unlockedMutex.
        //         //QWaitCondition::wait()释放lockedMutex, 然后等待条件进行解锁。lockedMutex最初必须由调用线程锁定。
        //     //m_mutex.unlock();// 解锁互斥量

        //     // Do Something……
        //     //添加QCoreApplication::processEvents保证能够及时处理事件
        //     QCoreApplication::processEvents(); //该函数的作用是让程序处理那些还没有处理的事件，然后再把使用权返回给调用者。
        // }
    }
    else     //如果相机、滤波轮、电源有任何一个未连接，则报错
    {

        QString ErrorMessage;
        if(!m_IfCameraConnected)
            ErrorMessage = "相机未连接，无法开启自动观测！请连接后再试。";
        else if(!m_IfFilterControllerConnedted)
            ErrorMessage = "滤波轮未连接，无法开启自动观测！请连接后再试。";
        else    //m_IfPowerConnected == 0
            ErrorMessage = "电源未连接，无法开启自动观测！请连接后再试。";

        emit AOC_Report_Error(ErrorMessage);
        emit AOC_Send_MessageBoxOutput(ErrorMessage);
        qDebug().noquote() << ErrorMessage;
    }
}


//暂停自动观测（阻塞AOC线程）
void AutoObserveController::PauseAutoObserve()
{
    m_mutex.lock();  //lock互斥量
    m_pause = true; // 暂停标志置1
    emit AOC_Send_MessageBoxOutput("暂停自动观测！");
    qDebug()<<"暂停自动观测！";
    m_pauseCondition.wait(&m_mutex);
    m_mutex.unlock();  //释放互斥量
}

//继续自动观测（唤醒AOC线程）
void AutoObserveController::ResumeAutoObserve()
{
    m_mutex.lock();  //锁定互斥量
    m_pause = false; // 暂停标志置0
    emit AOC_Send_MessageBoxOutput("继续自动观测！");
    qDebug()<<"继续自动观测！";
    m_pauseCondition.wakeAll(); // 唤醒所有被m_mutex/QWaitCondition::wait()阻塞的线程（AOC线程）
    m_mutex.unlock();  //释放互斥量

}

//结束自动观测
void AutoObserveController::TerminateAutoObserve()
{
    //清空程序中存储的时间表和模式
    QVector<QPair<QDateTime, QDateTime>>().swap(m_TimeTable);
    QVector<QPair<QDateTime, QDateTime>>().swap(m_RemainWorkPeriodList);
    QVector<QPair<QString,QString>>().swap(m_ModeSectionOne);
    QVector<QPair<QString,QString>>().swap(m_ModeSectionTwo);
    QVector<QPair<QString,QString>>().swap(m_ModeSectionThree);

    m_WhetherWorkingLastTimeout = false;  //保证下次点“启动”能从section1开始执行
    m_SectionNumLastTimeout = -1;  //-1:未执行任何一个section
    m_CurMode_index = 1;  //所选模式序号恢复默认
    m_CurWorkingPeriodIndex = -1;  //-1: 未执行任何一个工作时段

    //把AOC下各动作的是否完成标志都置否
    m_IfOperatePowerSwitch_Finished = false;
    m_IfOperateFilter_Finished = false;
    m_IfOperateCameraTemperature_Finished = false;
    m_IfOperateCameraShutterTimingMode_Finished = false;
    m_IfOperateCameraExposureTime_Finished = false;
    m_IfOperateCameraAcquireAndSave_Finished = false;

    emit AOC_Send_MessageBoxOutput("结束自动观测！");
    qDebug()<<"结束自动观测！";
}

//重写文件ASI_CurWorkPeriodStartDate.txt
void AutoObserveController::ReWrite_ASI_CurWorkPeriodStartDateTXT()
{
    QString ASI_CurWorkPeriodStartDateTXTFullPath = PathUtils::ExtractParentDirPath(get_DataSaveDirPath()) +
                                                    "/ASI_command_observation_control/ASI_CurWorkPeriodStartDate.txt";
    QFile LinkageControlFile(ASI_CurWorkPeriodStartDateTXTFullPath);

    QDateTime StartTime = QDateTime::currentDateTime();
    while(true)
    {
        QThread::msleep(50);

        if(LinkageControlFile.open(QIODevice::WriteOnly | QIODevice::Text))  //尝试打开文件
        {
            break;
        }

        if(StartTime.secsTo(QDateTime::currentDateTime()) > 10)
        {
            QString ErrorMessage = "尝试打开文件"+ASI_CurWorkPeriodStartDateTXTFullPath+"超时，打开失败！";
                                       emit AOC_Report_Error(ErrorMessage);
            return;    //如果尝试超过10s, 则报错提示，然后直接退出函数
        }
    }

    QString StartWorkPeriodDate = get_CurWorkingPeriodStartDate().toString("yyyyMMdd");
    LinkageControlFile.write(StartWorkPeriodDate.toUtf8());

    LinkageControlFile.close();


}


//***供UI线程调用从而消除AOC线程的阻塞***
// void AutoObserveController::AwakeSubThreadAOC()
// {
//     m_mutex_Auto.lock();  //锁定互斥量
//     m_autoCondition.wakeAll(); // 唤醒所有被m_mutex_Auto\QWaitCondition::wait()阻塞的线程（AOC线程）
//     m_mutex_Auto.unlock();  //释放互斥量
// }



//slots: AOC Timer超时处理函数
void AutoObserveController::onTimeout()
{
    qDebug()<<"！！！AOC Timer超时1次 ！！！";

    // 获取当前时间
    QDateTime currentTime = currentDateTime_UorL(get_UsedTimeType());

    //遍历工作时段List的每个条目
    for (int i = 0; i < m_RemainWorkPeriodList.size(); ++i)
    {
        const auto &period = m_RemainWorkPeriodList[i];
        if ( (currentTime >= period.first) && (currentTime < period.second))  // 如果当前时间在 m_RemainWorkPeriodList 的第i个工作时段内
        {
            m_CurWorkingStartAndEndTime = period;    //在进Section之前就要更新当前工作时段的 起始时间

            set_CurWorkingPeriodStartDate(m_CurWorkingStartAndEndTime.first.date());  //更新全局对象: 当前工作时段的起始日期

            ReWrite_ASI_CurWorkPeriodStartDateTXT();    //把最新的 当前工作时段起始日期 写入文件 ASI_CurWorkPeriodStartDate.txt

            QString StartWorkTime1 = m_CurWorkingStartAndEndTime.first.toString("yyyy年MM月dd日 hh:mm:ss");
            QString EndWorkTime1  = m_CurWorkingStartAndEndTime.second.toString("yyyy年MM月dd日 hh:mm:ss");
            QString StartWorkTime2, EndWorkTime2;
            if( m_RemainWorkPeriodNum >= 2 )  //如果包含当前进入的这个时段，剩余工作时段>=2, 就可以显示下一工作时段
            {
                StartWorkTime2 = m_RemainWorkPeriodList[i+1].first.toString("yyyy年MM月dd日 hh:mm:ss");
                EndWorkTime2 = m_RemainWorkPeriodList[i+1].second.toString("yyyy年MM月dd日 hh:mm:ss");
            }
            else
            {
                StartWorkTime2 = "---";
                EndWorkTime2 = "---";
            }
            emit AOC_Send_CurWorkingStartAndEndTime(StartWorkTime1,EndWorkTime1,StartWorkTime2,EndWorkTime2);  //AOC向UI通告当前工作时段
            emit AOC_Send_MessageBoxOutput("当前工作时段: "+StartWorkTime1+"至"+ EndWorkTime1);
            emit AOC_Send_MessageBoxOutput("下一工作时段: "+StartWorkTime2+"至"+ EndWorkTime2);
            qDebug()<<"当前工作时段: "<<StartWorkTime1<<"至"<<EndWorkTime1;
            qDebug()<<"下一工作时段: "<<StartWorkTime2<<"至"<<EndWorkTime2;

            if ( (!m_WhetherWorkingLastTimeout) || (m_CurWorkingPeriodIndex != i) ) // 如果设备之前不在工作状态（新进一个观测时段），则执行第一节，然后执行第二节
            {
                m_WhetherWorkingLastTimeout = true;
                ExecuteCurObserveModeOneSection(1); //执行第一节toString("yyyy-MM-dd hh:mm:ss");
                m_SectionNumLastTimeout = 1;               
                m_CurWorkingPeriodIndex = i;
            }
            else if(m_WhetherWorkingLastTimeout && (m_CurWorkingPeriodIndex == i) && m_SectionNumLastTimeout == 1)  //如果 上一次Timer超时的时候已经在工作了，并且还在同一个工作时段内
            {
                ExecuteCurObserveModeOneSection(2); //执行第二节
                m_SectionNumLastTimeout = 2;
                m_WhetherWorkingLastTimeout = true;
                m_CurWorkingPeriodIndex = i;
            }
            //return;
        }
        else if (m_WhetherWorkingLastTimeout && (currentTime >= period.second) && (m_SectionNumLastTimeout == 2) && (m_CurWorkingPeriodIndex == i))
        {
            // 如果当前时间刚好到达工作时段结束时间，执行DoSectionThree

            // //执行Section3之后，给日志文件新加一行，重复上一行的内容，但把Working改成End
            // QStringList LatestLogEntry_List = m_LatestLogEntry.split(',');   //将m_LatestLogEntry按照 "," 分隔成一个QStringList
            // if (!LatestLogEntry_List.isEmpty() && LatestLogEntry_List.last() == " Working")
            // {
            //     LatestLogEntry_List.last() = " End";  // 将最后一个字段修改为" End"
            //     m_LatestLogEntry = LatestLogEntry_List.join(',');    // 使用 join() 函数将 QStringList 合并回一个 QString
            // }
            // WriteObservLogTXT(get_DataSaveDirPath(), m_LatestLogEntry);

            ExecuteCurObserveModeOneSection(3); //执行第三节
            //emit AOC_Send_CurWorkingStartAndEndTime("--","--");  //AOC向UI通告当前工作时段
            m_SectionNumLastTimeout = 3;
            m_WhetherWorkingLastTimeout = false;
            m_CurWorkingPeriodIndex = -1;
            ;  //每执行一次section3结束，剩余工作时段就少一个
            if(--m_RemainWorkPeriodNum <= 0)
            {
                emit AOC_Report_FinishedAllWorkPeriod();   //完成了所有的有效观测时段，通知UI更新“启动暂停结束btn” 并 退出AOC线程
            }
            //return;
        }

    }
    //-------------------------------------for() end----------------------------------------------


    //??????????????????????????????????
    //这一段还有存在的必要吗？？？？？
    // 如果当前时间不在任何一个工作时段内
    // if (m_WhetherWorkingLastTimeout)
    // {
    //     //emit AOC_Send_CurWorkingStartAndEndTime("--","--");  //AOC向UI通告当前工作时段
    //     ExecuteCurObserveModeOneSection(3); //执行第三节
    //     m_SectionNumLastTimeout = -1;
    //     m_WhetherWorkingLastTimeout = false;
    //     m_CurWorkingPeriodIndex = -1;
    // }
    //?????????????????????????????????

}


//private slots: 执行当前所选观测模式的指定Section（待使用）
// void AutoObserveController::AOC_RespondPrivateSignal_ExecuteCurObserveModeOneSection(int WhitchSection)
// {
//     ExecuteCurObserveModeOneSection(WhitchSection);
// }
