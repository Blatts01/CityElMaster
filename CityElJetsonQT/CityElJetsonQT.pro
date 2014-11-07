# Add more folders to ship with the application, here
folder_01.source = qml/CityElJetsonQT
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
        ../caninterface.cpp \
        ../can_drv.c

HEADERS += ../caninterface.h \
           ../can_drv.h
# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include(qtquick1applicationviewer/qtquick1applicationviewer.pri)
qtcAddDeployment()

unix:!macx: LIBS += -L$$PWD/../ -lmhstcan
LIBS+= -L/lib/arm-linux-gnueabihf/ -ldl


INCLUDEPATH += $$PWD/../
DEPENDPATH += $$PWD/../
