#-------------------------------------------------
#
# Project created by QtCreator 2021-08-19T14:54:18
#
#-------------------------------------------------

QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OverloadNewZK
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix {
SOURCES += main.cpp\
    custom_button.cpp \
    custom_time_control.cpp \
    resultMode.cpp \
    qsingleapplication.cpp \
    getconfig.cpp \
    picshow.cpp

HEADERS  += \
    CriticalSection.h \
    custom_button.h \
    custom_time_control.h \
    resultMode.h \
    qsingleapplication.h \
    getconfig.h \
    picshow.h
}

win32{
SOURCES += main.cpp\
    custom_button.cpp \
    custom_time_control.cpp \
    resultMode.cpp \
    qsingleapplication.cpp \
    picshow.cpp

HEADERS  += \
    CriticalSection.h \
    custom_button.h \
    custom_time_control.h \
    resultMode.h \
    qsingleapplication.h \
    picshow.h
}

FORMS    += overwidget.ui \
    customtimecontrol.ui

win32:LIBS += -L$$PWD/lib/ -lLog
win32:LIBS += -L$$PWD/lib/ -lpthreadVC2
win32:LIBS += -ldbghelp
win32:LIBS += -L$$PWD/lib/ -lPlayCtrl

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

RESOURCES += \
    over.qrc

RC_FILE = myico.rc

/////////////////////////////////////////////////////////////
unix:!macx: LIBS += -L$$PWD/lib/ -lhcnetsdk

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include


unix:!macx: LIBS += -L$$PWD/lib/ -lhblog

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

unix:!macx: PRE_TARGETDEPS += $$PWD/lib/libhblog.a

unix:!mac:QMAKE_LFLAGS += -Wl,--rpath=./

unix:!macx: LIBS += -L$$PWD/lib/ -lPlayCtrl

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

HEADERS += \
    wtnetcontrol.h \
    wtdevcontrol.h \
    wtprocontrol.h \
    wtcamcontrol.h \
    wtledcontrol.h \
    wtcarcontrol.h \
    wtdbcontrol.h \
    carresult.h \
    common_utils.h \
    picpoll.h \
    overwidget.h \
    realtimewindow.h \
    hiswindow.h

SOURCES += \
    carresult.cpp \
    picpoll.cpp \
    overwidget.cpp \
    realtimewindow.cpp \
    hiswindow.cpp
