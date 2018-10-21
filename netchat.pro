QT       += core gui macextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

TARGET = Netchat
TEMPLATE = app

SOURCES += src/main.cpp \
           src/netchat.cpp \
           src/server.cpp \
           src/options.cpp

HEADERS  += src/netchat.h \
            src/server.h \
            src/options.h

FORMS += netchat.ui

macx: ICON = res/icon.icns
win32: RC_FILE = res/icon.rc

RESOURCES += resources.qrc

macx: LIBS += -L$$PWD/../../../../../../usr/local/lib/ -lbotan-2

INCLUDEPATH += $$PWD/../../../../../../usr/local/include/botan-2
DEPENDPATH += $$PWD/../../../../../../usr/local/include/botan-2
