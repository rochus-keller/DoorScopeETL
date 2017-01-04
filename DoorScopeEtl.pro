
TEMPLATE = app
TARGET = DoorScopeEtl
QT += network

INCLUDEPATH += ./.. ../../Libraries

	DESTDIR = ./tmp
	OBJECTS_DIR = ./tmp
	RCC_DIR = ./tmp
	MOC_DIR = ./tmp
	CONFIG(debug, debug|release) {
		DESTDIR = ./tmp-dbg
		OBJECTS_DIR = ./tmp-dbg
		RCC_DIR = ./tmp-dbg
		MOC_DIR = ./tmp-dbg
		DEFINES += _DEBUG
	}

win32 {
	INCLUDEPATH += $$[QT_INSTALL_PREFIX]/include/Qt
	RC_FILE = DoorScopeEtl.rc
	DEFINES -= UNICODE
	CONFIG(debug, debug|release) {
		LIBS += -lqjpegd -lqgifd
		DEFINES += _DEBUG
	 } else {
		LIBS += -lqjpeg -lqgif
	 }
 }else {
	INCLUDEPATH += $$(HOME)/Programme/Qt-4.4.3/include/Qt
	CONFIG(debug, debug|release)
	{
		DEFINES += _DEBUG
	}
	LIBS += -lqjpeg -lqgif
	QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-parameter
 }

HEADERS += ./DoorScopeEtl.h \
	./HtmlImporter.h \
	./IpcProtocol.h \
	./StreamAgent.h

#Source files
SOURCES += ./DoorScopeEtl.cpp \
	./HtmlImporter.cpp \
	./IpcProtocol.cpp \
	./main.cpp \
	./StreamAgent.cpp


#Include file(s)
include(../Stream/Stream.pri)



