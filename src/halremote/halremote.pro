TEMPLATE = lib
QT += qml quick network

uri = Machinekit.HalRemote
include(../plugin.pri)

include(../zeromq.pri)
include(../../3rdparty/machinetalk-protobuf-qt/machinetalk-protobuf-lib.pri)
include(../common/common.pri)

# Input
SOURCES += \
    plugin.cpp \
    qhalpin.cpp \
    qhalremotecomponent.cpp \
    halremotecomponentbase.cpp \
    qhalgroup.cpp \
    qhalsignal.cpp \
    halremotecomponent.cpp

HEADERS += \
    plugin.h \
    qhalpin.h \
    qhalremotecomponent.h \
    halremotecomponentbase.h \
    qhalgroup.h \
    qhalsignal.h \
    debughelper.h \
    halremotecomponent.h

QML_INFRA_FILES = \
    qmldir

include(../deployment.pri)
