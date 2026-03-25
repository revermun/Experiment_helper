QT       += core gui
QT       += xml
QT       += serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    connectionSettings.cpp \
    dataandgraphsdialog.cpp \
    deviceconfigurationsdialog.cpp \
    experimentconfigurationdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    notesdialog.cpp \
    startstopactionsdialog.cpp \
    ubloxparser.cpp

HEADERS += \
    connectionSettings.h \
    dataandgraphsdialog.h \
    deviceconfigurationsdialog.h \
    enums.h \
    experimentconfigurationdialog.h \
    mainwindow.h \
    notesdialog.h \
    qchecklist.h \
    qvaluefield.h \
    startstopactionsdialog.h \
    ublox.h \
    ubloxparser.h

FORMS += \
    connectionSettings.ui \
    dataandgraphsdialog.ui \
    deviceconfigurationsdialog.ui \
    experimentconfigurationdialog.ui \
    mainwindow.ui \
    notesdialog.ui \
    startstopactionsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images.qrc


