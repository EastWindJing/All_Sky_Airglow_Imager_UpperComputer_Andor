#pragma execution_character_set("utf-8")

#ifndef PATHUTILS_H
#define PATHUTILS_H

#include <qstring.h>
#include <qdir.h>
#include <QDebug>

class PathUtils
{
public:
    static QString getRootPath(); // 获取程序运行的根目录
    static QString ExtractParentDirPath(QString FileOrDirPath);    //从一个文件路径或者文件夹路径中提取出它的上一级路径
};

#endif // PATHUTILS_H
