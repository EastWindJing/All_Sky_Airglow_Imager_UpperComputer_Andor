#include "MessageTips.h"
#include <QDesktopWidget>
#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QDebug>

MessageTips::MessageTips(QString showStr, QWidget* parent)
    : QWidget(parent),m_showStr(showStr)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    this->setAttribute(Qt::WA_TranslucentBackground); // ****这里很重要****
    this->setAttribute(Qt::WA_TransparentForMouseEvents, true);// 禁止鼠标事件
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setFixedHeight(50);
    QFont font("Arial", m_textSize, QFont::Bold);
    QFontMetrics fontMetrics(font);
    int tw = fontMetrics.width(m_showStr);
    if (tw > 200)
        this->setFixedWidth(tw + 50);
    else
        this->setFixedWidth(200);
}

void MessageTips::prepare()
{
    this->setWindowOpacity(m_opacity);
    connect(this,SIGNAL(finished()),this,SLOT(onFinished()));
    QTimer* mtimer = new QTimer(this);//隐藏的定时器
    mtimer->setTimerType(Qt::PreciseTimer);
    connect(mtimer, &QTimer::timeout, this, [=]() {
        m_opacity = m_opacity - m_closeSpeed;
        if (m_opacity <= 0) {
            mtimer->stop();
            this->close();
            return;
        }
        else
            this->setWindowOpacity(m_opacity);
    });

    //support for multi-screens.
    int scrIndex = 0;
    QDesktopWidget* desktop = QApplication::desktop();
    QWidget* acWgt = QApplication::activeWindow();
    if (acWgt)
    {
        QPoint checkPoint(acWgt->pos().x() + acWgt->width() / 2, acWgt->pos().y() + acWgt->height() / 2);
        for (int i = 0; i < desktop->screenCount(); i++)
        {
            if (desktop->screenGeometry(i).contains(checkPoint))
            {
                scrIndex = i;
                break;
            }
        }
    }

    QRect scrRect = desktop->screenGeometry(scrIndex);
    //设置屏幕居中显示
    this->move(scrRect.left() + (scrRect.width() - this->width()) / 2,
               scrRect.top() + (scrRect.height() - this->height()) / 2);
    QTimer::singleShot(m_showTime, [=]() {mtimer->start(m_closeTime); });//执行延时自动关闭
}

void MessageTips::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setBrush(QBrush(m_bgColor));//窗体的背景色
    painter.setPen(QPen(m_frameColor, m_frameSize));//窗体边框的颜色和笔画大小
    QRectF rect(0, 0, this->width(), this->height());
    painter.drawRoundedRect(rect, 10, 10); // round rect
        //文字显示居中，设置字体，大小，颜色
    QFont font("Arial", m_textSize, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QPen(m_textColor, m_frameSize));
    painter.drawText(rect,Qt::AlignHCenter | Qt::AlignVCenter,m_showStr);
}

//设置关闭的时间和速度，speed大小限定0~1
void MessageTips::setCloseTimeSpeed(int closeTime, double closeSpeed)
{
    if (closeSpeed > 0 && closeSpeed <= 1)
        m_closeSpeed = closeSpeed;
    m_closeTime = closeTime;
}

void MessageTips::showMessageTips(QString showStr, QWidget* parent)
{
    MessageTips* mMessageTips = new MessageTips(showStr, parent);
    mMessageTips->setShowTime(1500);
    mMessageTips->prepare();
    mMessageTips->show();
}
