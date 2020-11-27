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
    workthread.cpp

HEADERS  += widget.h \
    workthread.h

FORMS    += widget.ui

RESOURCES += \
    qres.qrc

RC_FILE = fileWatcher.rc
