#-------------------------------------------------
#
# Project created by QtCreator 2019-07-23T22:36:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = xArchiveManager
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += \
        AboutDialog.cpp \
        ArchiveManager.cpp \
        ArchiveManagerWindow.cpp \
        NewArchiveDialog.cpp \
        main.cpp

HEADERS += \
        AboutDialog.h \
        ArchiveManager.h \
        ArchiveManagerWindow.h \
        NewArchiveDialog.h

FORMS += \
        AboutDialog.ui \
        ArchiveManagerWindow.ui \
        NewArchiveDialog.ui

INCLUDEPATH += \
        ../xArchive

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../Bin/x86/release/ -lxarchive
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../Bin/x86/debug/ -lxarchive

INCLUDEPATH += \
        $$PWD/../Bin/x86/Release \
        $$PWD/../Bin/x86/Debug
DEPENDPATH += \
        $$PWD/../Bin/x86/Release \
        $$PWD/../Bin/x86/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../Bin/x86/release/libxarchive.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../Bin/x86/debug/libxarchive.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../Bin/x86/release/xarchive.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../Bin/x86/debug/xarchive.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../packages/zlib_native.1.2.11/build/native/lib/Win32/Release/ -lzlib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../packages/zlib_native.1.2.11/build/native/lib/Win32/Debug/ -lzlibd

INCLUDEPATH += \
        $$PWD/../packages/zlib_native.1.2.11/build/native/lib/Win32/Release \
        $$PWD/../packages/zlib_native.1.2.11/build/native/lib/Win32/Debug
DEPENDPATH += \
        $$PWD/../packages/zlib_native.1.2.11/build/native/lib/Win32/Release \
        $$PWD/../packages/zlib_native.1.2.11/build/native/lib/Win32/Debug
