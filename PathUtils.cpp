//#pragma execution_character_set("utf-8")

#include "pathutils.h"

// 获取项目目录的路径
QString PathUtils::getRootPath()
{
    QDir* dir = new QDir(QDir::currentPath());
    if (!dir->cdUp()) {
        return QDir::currentPath();
    }
    //return dir->absolutePath(); //返回项目目录的上一级路径
    return (dir->absolutePath()) + "/All_Sky_Airglow_Imager_UpperComputer_Andor";
}

//从一个文件路径或者文件夹路径中提取出它的上一级路径
QString PathUtils::ExtractParentDirPath(QString FileOrDirPath)
{
    //输出原始路径
    //qDebug() << "Origin Path = " << FileOrDirPath;

    //1.将 FileOrDirPath 路径按符号“/”分割成一段一段的，存储到一个QStringList中
    QStringList pathList = FileOrDirPath.split('/');

    //2.删去路径中最后一个“/”及其之后的内容
    if(!pathList.isEmpty())
        pathList.removeLast();

    //3.将剩下的字段按原顺序重新拼接成一个QString对象
    QString newPath = pathList.join('/');

    // 输出重新拼接后的路径字符串
    //qDebug() << "New Path:" << newPath;

    return newPath;
}


// QString PathUtils::getRootPath()
// {
//     return QDir::currentPath();  //返回当前工作目录的路径（与项目目录不同）
// }
