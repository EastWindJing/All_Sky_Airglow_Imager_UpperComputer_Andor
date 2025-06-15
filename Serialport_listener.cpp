//#pragma execution_character_set("utf-8")

#include "Serialport_listener.h"
#include <windows.h>
//#include <QApplication>


SerialPort_Listener::SerialPort_Listener(QWidget *parent)
    : QWidget{parent}, QAbstractNativeEventFilter{}
{ }

bool SerialPort_Listener::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = reinterpret_cast<MSG*>(message);
    unsigned int msgType = msg->message;
    if(msgType == WM_DEVICECHANGE)
    {
        emit DeviceChangeCbk();
        if(msg->wParam == DBT_DEVICEARRIVAL)
        {
            qDebug("usb in");
            emit DevicePlugIn();         //触发信号
        }
        if(msg->wParam == DBT_DEVICEREMOVECOMPLETE)
        {
            qDebug("usb out");
            emit DevicePlugOut();         //触发信号
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}


