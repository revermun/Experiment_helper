QT       += core gui
QT       += xml
QT       += serialport
QT       += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += YAML_CPP_STATIC_DEFINE
INCLUDEPATH += "C:/Users/rever/Documents/cpp-libs/yaml-cpp-master/include"

# Условное подключение библиотек в зависимости от конфигурации
CONFIG(debug, debug|release) {
    # Debug конфигурация
    LIBS += -L"C:/Users/rever/Documents/cpp-libs/yaml-cpp-master/build-static-x86/Debug"
    LIBS += -lyaml-cppd
    message("Building in DEBUG mode with yaml-cppd.lib")
} else {
    # Release конфигурация
    LIBS += -L"C:/Users/rever/Documents/cpp-libs/yaml-cpp-master/build-static-x86/Release"
    LIBS += -lyaml-cpp
    message("Building in RELEASE mode with yaml-cpp.lib")
}


SOURCES += \
    connectionSettings.cpp \
    dataandgraphsdialog.cpp \
    deviceconfigurationsdialog.cpp \
    eventeditdialog.cpp \
    eventsettingsdialog.cpp \
    experimentconfigurationdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    notesdialog.cpp \
    pickdirectorydialog.cpp \
    presetsettingsdialog.cpp \
    startstopactionsdialog.cpp \
    ubloxparser.cpp \
    unicoreparser.cpp

HEADERS += \
    connectionSettings.h \
    convertors.h \
    dataandgraphsdialog.h \
    deviceconfigurationsdialog.h \
    enums.h \
    eventeditdialog.h \
    eventsettingsdialog.h \
    experimentconfigurationdialog.h \
    groupboxareawidget.h \
    mainwindow.h \
    notesdialog.h \
    pickdirectorydialog.h \
    presetsettingsdialog.h \
    qchecklist.h \
    serialtotcpbridge.h \
    startstopactionsdialog.h \
    structs.h \
    ublox.h \
    ubloxparser.h \
    unicore.h \
    unicoreparser.h

FORMS += \
    connectionSettings.ui \
    dataandgraphsdialog.ui \
    deviceconfigurationsdialog.ui \
    eventeditdialog.ui \
    eventsettingsdialog.ui \
    experimentconfigurationdialog.ui \
    mainwindow.ui \
    notesdialog.ui \
    pickdirectorydialog.ui \
    presetsettingsdialog.ui \
    startstopactionsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images.qrc


