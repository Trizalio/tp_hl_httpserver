#-------------------------------------------------
#
# Project created by QtCreator 2014-11-28T12:11:45
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = httpServer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app
QMAKE_CXXFLAGS += -std=c++11

LIBS += -levent

SOURCES += main.cpp \
    httplogic.cpp

HEADERS += \
    httplogic.h \
    SConfig.h
