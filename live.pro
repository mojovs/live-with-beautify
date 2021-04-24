QT       += core gui opengl multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += console


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.~!
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# ffmpeg
INCLUDEPATH += $$PWD/../ffmpeg-4.3.2/include
DEPENDPATH += $$PWD/../ffmpeg-4.3.2/include
LIBS += -L$$PWD/../ffmpeg-4.3.2/lib -lavformat \
                -lavcodec \
                -lavfilter \
                -lavdevice \
                -lavutil \
                -lpostproc \
                -lswresample \
                -lswscale

#opencv
INCLUDEPATH += E:/ENV/opencv344/build/include   \
                E:/ENV/opencv344/build/include/opencv \
                E:/ENV/opencv344/build/include/opencv2 \
DEPENDPATH += E:/ENV/opencv344/build/include    \
              E:/ENV/opencv344/build/include/opencv \
              E:/ENV/opencv344/build/include/opencv2 \
LIBS += -LE:/ENV/opencv344/build/x64/vc14/lib \
-lopencv_world344d

LIBS += -LE:/ENV/opencv344/build/x64/vc15/lib \
-lopencv_world344d

