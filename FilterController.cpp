#include "FilterController.h"
#include "GlobalVariable.h"

FilterController::FilterController(QObject *parent)
    : QObject{parent}
{
    read_FilterPreSetParam();

}


//从FilterParam.txt中读取各波段滤波片对应的孔位index
void FilterController::read_FilterPreSetParam()
{
    QFile file(m_FilterParamTXT_FullPath); //创建QFile对象用于操作文件
    QFileInfo fileinfo(m_FilterParamTXT_FullPath);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))  //以读写和文本方式打开文件
    {
        QMessageBox::warning(nullptr, "错误",("无法打开文件：" + m_FilterParamTXT_FullPath +"\n错误信息：" + file.errorString()));
    }
    else
    {
        if(fileinfo.size() == 0)
        {
            QMessageBox::warning(nullptr, "错误", "文件： " + m_FilterParamTXT_FullPath + " 为空！请重新创建此文件！" );
        }
        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            QString str(line);
            QStringList strList = str.split(" ");
            if (str.startsWith("750-850", Qt::CaseInsensitive))
                m_750850index =  strList.at(1).toInt()-1;
            else if (str.startsWith("862", Qt::CaseInsensitive))
                m_862index =  strList.at(1).toInt()-1;
            else if (str.startsWith("630", Qt::CaseInsensitive))
                m_630index =  strList.at(1).toInt()-1;
            else if (str.startsWith("000", Qt::CaseInsensitive))
                m_000index =  strList.at(1).toInt()-1;
        }
        file.close();
    }

}


//尝试连接滤波轮
bool FilterController::ConnectAndInitFilter()
{
    m_AccessibleEFWFilterNum = EFWGetNum();  //检查当前有多少可用EFW滤波轮

    if( m_AccessibleEFWFilterNum >0 )  //如果当前可访问的EFW滤波轮数量大于0
    {
        //获取第一个滤波轮的ID
        //EFWGetID(int index, int* ID) 如果过滤轮未打开，则ID为负，否则ID为0到(EFW_ID_MAX - 1)之间的唯一整数，
        //打开后所有操作都基于此ID,在过滤轮关闭前ID不会改变
        EFW_ERROR_CODE ErrorCode_GetID = EFWGetID(m_CurFilter_Index, &m_CurFilter_ID );

        if(ErrorCode_GetID != EFW_SUCCESS)
        {
            QString Message = "连接滤波轮失败，具体是执行EFWGetID()失败，EFW_ERROR_CODE = "+ TransEFWErrorCodeToQString(ErrorCode_GetID);
            emit FC_Report_Error(Message);
            qDebug()<< Message;
            return false;
        }

        //打开第一个滤波轮
        EFW_ERROR_CODE ErrorCode_OpenFilter = EFWOpen(m_CurFilter_ID);
        if(ErrorCode_OpenFilter != EFW_SUCCESS)
        {
            QString Message = "连接滤波轮失败，具体是执行EFWOpen()失败，EFW_ERROR_CODE = "+TransEFWErrorCodeToQString(ErrorCode_OpenFilter);
            emit FC_Report_Error(Message);
            qDebug()<<Message;
            return false;
        }

        //获取滤波轮的信息（ID、name、通孔数量）
        if( Get_CurFilterInfo() )
        {
            qDebug()<<"第一个可用的EFWFilter_Info.ID = "<<m_CurFilter_Info.ID;
            qDebug()<<"第一个可用的EFWFilter_Info.Name = "<<m_CurFilter_Info.Name;
            qDebug()<<"第一个可用的EFWFilter_Info.slotNum = "<<m_CurFilter_Info.slotNum;
        }
        else
        {
            return false;  //错误信息Get_CurFilterInfo()中会输出
        }

        //设置-滤波轮是否只能朝一个方向转动
        EFW_ERROR_CODE ErrorCode_SetDirection = EFWSetDirection(m_CurFilter_ID, m_CurFilterUniDirectional);
        if(ErrorCode_SetDirection != EFW_SUCCESS)
        {
            QString Message = "连接滤波轮失败，具体是执行EFWSetDirection()失败，EFW_ERROR_CODE = "+ TransEFWErrorCodeToQString(ErrorCode_SetDirection);
            emit FC_Report_Error(Message);
            qDebug()<<Message;
            return false;
        }

        MoveToOneSlot(0);  //连接成功就转到0号位
        m_FilterIsConnected = true;
        return true;        //如果每一步都通过了，就认为连接成功
    }
    else
    {
        QString Message = "当前可用的EFW滤波轮数量 = " + QString::number(m_AccessibleEFWFilterNum) + "，无法连接到任何一个滤波轮！";
        emit FC_Report_Error(Message);
        qDebug()<<Message;
        return false;
    }

}

//断开滤波轮的连接
bool FilterController::DisConnectFilter()
{
    //关闭当前打开的滤波轮
    EFW_ERROR_CODE ErrorCode_Close = EFWClose(m_CurFilter_ID);
    if(ErrorCode_Close == EFW_SUCCESS)
    {
        m_FilterIsConnected = false;
        return true;
    }
    else
    {
        QString Message = "断开滤波轮连接失败，具体是执行EFWClose()失败，EFW_ERROR_CODE = "+ TransEFWErrorCodeToQString(ErrorCode_Close);
        emit FC_Report_Error(Message);
        qDebug()<<Message;
        return false;
    }
}


//获取当前滤波轮的信息（ID、name、slot总数）
bool FilterController::Get_CurFilterInfo()
{
    EFW_ERROR_CODE ErrorCode_GetProperty = EFWGetProperty(m_CurFilter_ID, &m_CurFilter_Info);
    if(ErrorCode_GetProperty == EFW_SUCCESS)
    {
        return true;
    }
    else
    {
        QString Message = "获取当前滤波轮的信息失败，具体是执行FilterController::Get_CurFilterInfo()失败，EFW_ERROR_CODE = "+
                          TransEFWErrorCodeToQString(ErrorCode_GetProperty);
        emit FC_Report_Error(Message);
        qDebug()<<Message;
        return false;
    }
}

//获取当前滤波轮通孔处的槽编号，该值介于0到M-1之间，M为通孔数量，如果滤轮在移动，该值为-1。
int FilterController::Get_CurSlotsIndex()
{
    EFW_ERROR_CODE ErrorCode_GetPosition = EFWGetPosition(m_CurFilter_ID, &m_CurSlotIndex);  //看看当前处于几号孔
    if(ErrorCode_GetPosition == EFW_SUCCESS)
    {
        return m_CurSlotIndex;
    }
    else
    {
        qDebug()<<"FilterController::Get_CurSlotsIndex()失败，EFW_ERROR_CODE = "<< TransEFWErrorCodeToQString(ErrorCode_GetPosition);
        return -2;
    }
}

//将指定编号TargetSlotIndex（0~M-1）的槽转到通孔处
bool FilterController::MoveToOneSlot(int TargetSlotIndex)
{
    m_TargetSlotIndex = TargetSlotIndex;

    EFW_ERROR_CODE ErrorCode_SetPosition = EFWSetPosition(m_CurFilter_ID,m_TargetSlotIndex);
    if(ErrorCode_SetPosition == EFW_SUCCESS)
    {
        QDateTime StartMoveTime = currentDateTime_UorL(get_UsedTimeType());
        while(Get_CurSlotsIndex() != m_TargetSlotIndex)          //首次等待
        {
            QThread::msleep(100);
            QCoreApplication::processEvents();
            if( StartMoveTime.msecsTo(currentDateTime_UorL(get_UsedTimeType())) > 10000 )
            {
                //超过10s未到达就认为是转动失败了
                QString Message = "尝试将index = "+QString::number(TargetSlotIndex)+
                                  "的slot移至滤波轮通孔处失败！当前所在slot的index = "+
                                  QString::number(Get_CurSlotsIndex());
                emit FC_Report_Error(Message);
                qDebug()<< Message ;
                return false;
            }
        }
        qDebug()<<"成功到达第"+QString::number(m_TargetSlotIndex)+"号slot！";
        return true;
    }
    else
    {
        QString ErrorMessage = "尝试将index = "+QString::number(TargetSlotIndex)+"的slot移至滤波轮通孔处失败！"+
                               "具体是在FilterController::MoveToOneSlot()时出错，EFW_ERROR_CODE =" +
                               TransEFWErrorCodeToQString(ErrorCode_SetPosition)+
                               ".当前所在slot的index = "+ QString::number(Get_CurSlotsIndex());
        emit FC_Report_Error(ErrorMessage);
        qDebug()<< ErrorMessage;
        return false;
    }
}


//把ErrorCode枚举类型的值转为QString
QString FilterController::TransEFWErrorCodeToQString(EFW_ERROR_CODE ErrorCode)
{
    switch (ErrorCode)
    {
        case 0:
            return "EFW_SUCCESS";
            break;
        case 1:
            return "INVALID_INDEX";
            break;
        case 2:
            return "INVALID_ID";
            break;
        case 3:
            return "INVALID_VALUE";
            break;
        case 4:
            return "REMOVED";
            break;
        case 5:
            return "MOVING";
            break;
        case 6:
            return "ERROR_STATE";
            break;
        case 7:
            return "GENERAL_ERROR";
            break;
        case 8:
            return "NOT_SUPPORTED";
            break;
        case 9:
            return "CLOSED";
            break;
        case -1:
            return "END";
            break;
        default:
            return "UnknownError!";
            break;
        }
}

//**************************************************
//***FC主工作函数，UI上的连接滤波轮btn connect到此函数***
//**************************************************
void FilterController::FilterControllerMain()
{
    //创建并开启定时器
    int timeoutPeriod = 200;  //定时器超时周期（ms）
    m_TimerPtr = new QTimer; //创建FC专属定时器
    m_TimerPtr->setInterval(timeoutPeriod);//每timeoutPeriod ms超时一次
    connect(m_TimerPtr, SIGNAL(timeout()), this, SLOT(onTimeout()));
    m_TimerPtr->start(); //启动定时器


}

//slots：手动控制切换滤波片
void FilterController::FC_Respond_ChangeSlotTo(int index)
{
    if(!get_FilterIsConnected())
    {
        QString ErrorMessage = "滤波轮未连接！请重新连接后再试。";
        emit FC_Report_Error(ErrorMessage);
        qDebug()<<ErrorMessage;
        return;
    }
    MoveToOneSlot(index);
}
//************FilterControllerMain() end************


//AOC指挥FC将滤波轮转到指定位置
void FilterController::FC_Respond_OperateFilter(QString CurWaveBand)
{
    //根据波段获取slot index
    int TargetSlotIndex;
    if(CurWaveBand == "750-850")
        TargetSlotIndex = m_750850index;
    else if(CurWaveBand == "862")
        TargetSlotIndex = m_862index;
    else if(CurWaveBand == "630")
        TargetSlotIndex = m_630index;
    else if(CurWaveBand == "000")
        TargetSlotIndex = m_000index;
    else
    {
        QString ErrorMessage = "FC_Respond_OperateFilter()接收的参数不可识别！请检查模式定义文件。";
        emit FC_Report_Error(ErrorMessage);
        qDebug()<< ErrorMessage;
        emit FC_Report_PauseAutoObserve();      //通知AOC阻塞、UI更新暂停按钮为“继续”
        return;
    }

    if(!MoveToOneSlot(TargetSlotIndex))  //第一次尝试转动滤波轮
    {
        //如果第一次尝试转动不成功就再试一次
        if( !MoveToOneSlot(TargetSlotIndex) )
        {
            //如果两次尝试都失败了
            emit FC_Report_PauseAutoObserve();      //通知AOC暂停、UI更新暂停按钮为“继续”
            return;
        }
        emit FC_Report_OperateFilter_Finish(); //FC通知AOC滤波轮操作已完成
        qDebug()<<"滤波轮成功转至第"+QString::number(m_TargetSlotIndex)+"号slot！";
    }
    emit FC_Report_OperateFilter_Finish(); //FC通知AOC滤波轮操作已完成
    qDebug()<<"滤波轮成功转至第"+QString::number(m_TargetSlotIndex)+"号slot！";
}




//FC Timer超时后执行
void FilterController::onTimeout()
{
    if(get_FilterIsConnected())
        emit FC_Report_NewCurSlotIndex(Get_CurSlotsIndex());  //获取当前在通孔处的槽编号，通知给UI
    else
        emit FC_Report_NewCurSlotIndex(-1);
    //qDebug()<<"FC Timer超时一次！";
}
