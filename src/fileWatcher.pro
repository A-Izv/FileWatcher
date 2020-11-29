#-------------------------------------------------
#
# Project created by QtCreator 2014-10-12T20:22:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fileWatcher
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
        workthread.cpp \
    simpleippfir2file.cpp \
    myfirtaps.cpp

HEADERS  += widget.h \
        workthread.h \
    simpleippfir2file.h \
    myfirtaps.h

FORMS    += widget.ui

RESOURCES += \
    qres.qrc

RC_FILE = fileWatcher.rc

include( ../../Intel_LIBS/ipp.pri)
