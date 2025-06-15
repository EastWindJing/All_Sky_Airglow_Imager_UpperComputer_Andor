#ifndef MESSAGETIPS_H
#define MESSAGETIPS_H

#include <QObject>
#include <QWidget>

class MessageTips : public QWidget
{
    Q_OBJECT

public:
    MessageTips(QString showStr = "", QWidget* parent = nullptr);

    double getOpacityValue() { return m_opacity; };
    void setOpacityValue(double value) { m_opacity = value; };

    int getTextSize() { return m_textSize; };
    void setTextSize(int value) { m_textSize = value; };

    QColor getTextColor() { return m_textColor; };
    void setTextColor(const QColor& value) { m_textColor = value; };

    QColor getBackgroundColor() { return m_bgColor; };
    void setBackgroundColor(const QColor& value) { m_bgColor = value; };

    QColor getFrameColor() { return m_frameColor; };
    void setFrameColor(const QColor& value) { m_frameColor = value; };

    int getFrameSize() { return m_frameSize; };
    void setFrameSize(int value) { m_frameSize = value; };

    int getShowTime() { return m_showTime; };
    void setShowTime(int value) { m_showTime = value; };

    void setCloseTimeSpeed(int closeTime = 100, double closeSpeed = 0.1);
    void prepare();
    static void showMessageTips(QString showStr, QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_showStr;
    double m_opacity = 0.8;
    int     m_textSize = 18;//显示字体大小
    QColor  m_textColor = QColor(255, 255, 255);//字体颜色
    QColor  m_bgColor = QColor(192, 192, 192);//窗体的背景色
    QColor  m_frameColor = QColor(211, 211, 211);//边框颜色
    int     m_frameSize = 2;//边框粗细大小
    // int     m_showTime = 3500;//显示时间
    // int     m_closeTime = 100;//关闭需要时间
    int     m_showTime = 3500;//显示时间
    int     m_closeTime = 175;//关闭需要时间
    double  m_closeSpeed = 0.05;//窗体消失的平滑度，大小0~1
    // double  m_closeSpeed = 0.1;//窗体消失的平滑度，大小0~1
};


#endif // MESSAGETIPS_H
