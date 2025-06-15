#include "LastImageDisplayWindow.h"
#include "ui_LastImageDisplayWindow.h"

LastImageDisplayWindow::LastImageDisplayWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LastImageDisplayWindow)
{
    ui->setupUi(this);

    //设置窗口始终位于最顶层
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    //确保窗口可以接收键盘事件
    setFocusPolicy(Qt::StrongFocus);  //窗口部件通过点击和Tab都可以被Focus
}

LastImageDisplayWindow::~LastImageDisplayWindow()
{
    delete ui;
}

//显示最近拍摄的一张图片
void LastImageDisplayWindow::DisplayLastImage(const QString &filePath)
{
    m_currentFilePath = filePath;

    // 读取tif图片
    m_image = cv::imread(filePath.toStdString(), cv::IMREAD_UNCHANGED);

    if (m_image.empty())
    {
        QString ErrorMessage = "读取文件"+ filePath +"失败！因为该文件为空。";
        QMessageBox::warning(this, "图片显示窗报错", ErrorMessage);
        qDebug()<<ErrorMessage;
        return;
    }
    if (m_image.type() != CV_16UC1)
    {
        QString ErrorMessage = "读取文件"+ filePath +"失败！因为该文件不是CV_16UC1类型的。";
        QMessageBox::warning(this, "图片显示窗报错", ErrorMessage);
        qDebug()<<ErrorMessage;
        return;
    }

    //每次显示新图片时重置缩放因子
    //m_scaleFactor = 1.0;

    // image转换成QImage，使用QImage::Format_Grayscale16
    m_qImage = QImage(reinterpret_cast<const uchar*>(m_image.data), m_image.cols, m_image.rows, m_image.step, QImage::Format_Grayscale16);

    //把m_image根据缩放因子调整大小后转换成QImage，使用QImage::Format_Grayscale16
    // cv::Mat resizedImage;    //装调整大小之后的新图像数据
    // cv::resize(m_image, resizedImage, cv::Size(), m_scaleFactor, m_scaleFactor); // XY轴都按照缩放因子scaleFactor调整图片大小
    // m_qImage = QImage(reinterpret_cast<const uchar*>(resizedImage.data), resizedImage.cols, resizedImage.rows, resizedImage.step, QImage::Format_Grayscale16);

    // 调整窗口大小
    adjustWindowSize();

    //设置窗口标题为文件名
    QFileInfo fileInfo(filePath);
    m_currentFileName = fileInfo.fileName();
    setWindowTitle(m_currentFileName);

    // // 设置窗口标题为文件名（带路径）
    // setWindowTitle(filePath);

    //加载当前目录下的图片文件列表
    LoadAllImageFiles(fileInfo.absolutePath());

    //触发绘制事件
    update();
}

//在窗口中绘制图片
void LastImageDisplayWindow::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (!m_qImage.isNull())
    {
        QPainter painter(this);

        cv::Mat resizedImage;
        cv::resize(m_image, resizedImage, cv::Size(), m_scaleFactor, m_scaleFactor);
        QImage qImage(reinterpret_cast<const uchar*>(resizedImage.data), resizedImage.cols, resizedImage.rows, resizedImage.step, QImage::Format_Grayscale16);
        painter.drawImage(0, 0, qImage);
    }
}

//重载keyPressEvent，从而用左右方向键切换当前显示的照片
void LastImageDisplayWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Left:
        if (m_currentIndex > 0)
        {
            DisplayLastImage(QFileInfo(m_currentFilePath).absolutePath() + "/" + m_imageFiles.at(m_currentIndex - 1));
        }
        break;
    case Qt::Key_Right:
        if (m_currentIndex  < m_imageFiles.size() - 1)
        {
            DisplayLastImage(QFileInfo(m_currentFilePath).absolutePath() + "/" + m_imageFiles.at(m_currentIndex + 1));
        }
        break;
    case Qt::Key_Up:        // 按 上方向键，长宽变原来的2倍
        m_scaleFactor *= 2.0;
        adjustWindowSize();
        update();
        break;
    case Qt::Key_Down:      // 按 下方向键，长宽变原来的0.5倍
        m_scaleFactor *= 0.5;
        adjustWindowSize();
        update();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

//按下鼠标
void LastImageDisplayWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)  //如果按下鼠标左键
    {
        QPoint startPos = event->globalPos();
        m_MouseWindowPosDif = startPos - geometry().topLeft();
        setCursor(Qt::ClosedHandCursor); //鼠标切换成握紧的手
    }
    QWidget::mousePressEvent(event);
}

//移动鼠标
void LastImageDisplayWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton)
    {
        QPoint endPos = event->globalPos();
        move(endPos - m_MouseWindowPosDif);
    }
    QWidget::mouseMoveEvent(event);
}

//松开鼠标
void LastImageDisplayWindow::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    QWidget::mouseReleaseEvent(event);  //鼠标切换成张开的手
}

//根据图片大小调整窗口大小
void LastImageDisplayWindow::adjustWindowSize()
{
    if (!m_qImage.isNull())
    {
        //setFixedSize(m_qImage.size());
        resize(m_image.cols * m_scaleFactor, m_image.rows * m_scaleFactor);
    }
}

//1.加载当前目录下的所有符合命名规则的tif文件，并保存文件名列表。2.确定当前图片在文件列表中的索引。
void LastImageDisplayWindow::LoadAllImageFiles(const QString &CurDirectoryPath)
{
    QDir TifFileDir(CurDirectoryPath);
    QStringList nameFilters;
    nameFilters << "VFAJ_ASA01_I*.tif";
    m_imageFiles = TifFileDir.entryList(nameFilters, QDir::Files, QDir::Name);  //过滤出所有名字为"VFAJ_ASA01_I*.tif"的文件
    m_currentIndex = m_imageFiles.indexOf(QFileInfo(m_currentFilePath).fileName());
}

//切换到前一张图片显示
void LastImageDisplayWindow::on_btn_ShowPreviousImage_clicked()
{
    if (m_currentIndex > 0)
    {
        DisplayLastImage(QFileInfo(m_currentFilePath).absolutePath() + "/" + m_imageFiles.at(m_currentIndex - 1));
    }
}

//切换到后一张图片显示
void LastImageDisplayWindow::on_btn_ShowNextImage_clicked()
{
    if (m_currentIndex  < m_imageFiles.size() - 1)
    {
        DisplayLastImage(QFileInfo(m_currentFilePath).absolutePath() + "/" + m_imageFiles.at(m_currentIndex + 1));
    }
}









