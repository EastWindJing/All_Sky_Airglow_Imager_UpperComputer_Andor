QT       += core gui
QT += sql
QT += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17



# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AutoObserveController.cpp \
    CameraController.cpp \
    FilterController.cpp \
    GlobalVariable.cpp \
    LastImageDisplayWindow.cpp \
    MessageTips.cpp \
    Serialport_listener.cpp \
    ActionEdit.cpp \
    main.cpp \
    mainwindow.cpp \
    PathUtils.cpp

HEADERS += \
    AutoObserveController.h \
    CameraController.h \
    FilterController.h \
    GlobalVariable.h \
    LastImageDisplayWindow.h \
    MessageTips.h \
    Serialport_listener.h \
    ActionEdit.h \
    lib_64_release/EFW_filter.h \
    lib_64_release/opencv2/core.hpp \
    lib_64_release/opencv2/imgcodecs.hpp \
    lib_64_release/opencv2/opencv.hpp \
    lib_64_release/picam.h \
    lib_64_release/picam_advanced.h \
    lib_64_release/pil_platform.h \
    mainwindow.h \
    PathUtils.h

FORMS += \
    ActionEdit.ui \
    LastImageDisplayWindow.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    AllSkyAirglowImager_UpperComputer_Andor.qrc

# #第三方库的头文件目录
# INCLUDEPATH += $$PWD/lib
# DEPENDPATH += $$PWD/lib

# # 第三方静态链接库
#  LIBS += -L$$PWD/lib_32_release/ -lAdhon_PMC_Lib

# # win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./release/ -lAdhon_PMC_Lib
# # else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./debug/ -lAdhon_PMC_Lib
# # else:unix:!macx: LIBS += -L$$PWD/./ -lAdhon_PMC_Lib

# INCLUDEPATH += $$PWD/lib_32_release
# DEPENDPATH += $$PWD/lib_32_release


LIBS += -L$$PWD/lib_64_release/ -lPicam

INCLUDEPATH += $$PWD/lib_64_release
DEPENDPATH += $$PWD/lib_64_release

DISTFILES += \
    lib_64_release/EFW_filter.dll \
    lib_64_release/EFW_filter.lib \
    lib_64_release/Picam.dll \
    lib_64_release/Picam.lib



LIBS += -L$$PWD/lib_64_release/ -lopencv_world4100

INCLUDEPATH += $$PWD/lib_64_release
DEPENDPATH += $$PWD/lib_64_release


LIBS += -L$$PWD/lib_64_release/ -lEFW_filter

INCLUDEPATH += $$PWD/lib_64_release
DEPENDPATH += $$PWD/lib_64_release
