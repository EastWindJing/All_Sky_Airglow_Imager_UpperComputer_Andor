#pragma execution_character_set("utf-8")

#ifndef SERIALPORT_LISTENER_H
#define SERIALPORT_LISTENER_H

#include <QWidget>
#include <windows.h>
#include <QAbstractNativeEventFilter>
#include <dbt.h>

class SerialPort_Listener : public QWidget, public QAbstractNativeEventFilter  //QAbstractNativeEventFilter 类为接收本地事件（如 MSG 或 XCB 事件结构）提供了一个接口
{
    Q_OBJECT
public:
    explicit SerialPort_Listener(QWidget *parent = nullptr);

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);

signals:
    void DeviceChangeCbk();
    void DevicePlugIn();
    void DevicePlugOut();

};

#endif // SERIALPORT_LISTENER_H
