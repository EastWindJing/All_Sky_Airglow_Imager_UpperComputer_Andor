#pragma execution_character_set("utf-8")

#ifndef LASTIMAGEDISPLAYWINDOW_H
#define LASTIMAGEDISPLAYWINDOW_H
//#include "ui_LastImageDisplayWindow.h"

#include <QWidget>
#include <QMessageBox>
#include <QPainter>
#include <QFileInfo>
#include <QDebug>
#include <opencv2/core.hpp>         //提供cv::Mat等
#include <opencv2/imgcodecs.hpp>    //提供cv::imread等
#include <opencv2/opencv.hpp>
#include <QDir>
#include <QKeyEvent>


namespace Ui {
class LastImageDisplayWindow;
}


class LastImageDisplayWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LastImageDisplayWindow(QWidget *parent = nullptr);
    ~LastImageDisplayWindow();

    void DisplayLastImage(const QString &filePath);  //显示拍摄的最近一张图片

protected:
    void paintEvent(QPaintEvent *event) override;  //一个虚函数，用于处理窗口部件的绘制事件。
        //将其声明为protected类型是为了遵循Qt的设计模式，并确保代码的封装性和安全性。
    void keyPressEvent(QKeyEvent *event) override; // 重载keyPressEvent，从而用左右方向键切换当前显示的照片
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::LastImageDisplayWindow *ui;

    cv::Mat m_image; // 用于存储OpenCV图片
    QImage m_qImage; // 用于显示的QImage

    QString m_currentFileName;
    QString m_currentFilePath;  //当前显示的tif文件路径
    QStringList m_imageFiles;
    int m_currentIndex = -1;  //当前显示的图片在文件夹中的index

    double m_scaleFactor = 1.0; //窗口缩放因子

    QPoint m_MouseWindowPosDif; //鼠标点击点 与 图片显示窗口 左上角之间的坐标差。QPoint：使用整数精度来定义平面中的一个点

    void adjustWindowSize();  //根据图片大小自适应调整图片显示窗大小

    void LoadAllImageFiles(const QString &directoryPath);  //加载当前目录下的所有符合命名规则的tif文件，并保存文件名列表。确定当前图片在文件列表中的索引。

signals:
    void LIDW_Report_HideLastImageDisplayWindow();  //通知MainWindow,LastImageDisplayWindow被隐藏了,要及时把“显示最近一张图片”取消勾选

private slots:
    void on_btn_ShowPreviousImage_clicked();    //切换到前一张图片显示
    void on_btn_ShowNextImage_clicked();        //切换到后一张图片显示

};

#endif // LASTIMAGEDISPLAYWINDOW_H
