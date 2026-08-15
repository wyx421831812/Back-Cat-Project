QT += core gui widgets
QT += network

greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

CONFIG += c++17

TARGET = BackPet
TEMPLATE = app
DESTDIR = bin
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

# 源文件
SOURCES += \
    src/main.cpp \
    src/petwidget.cpp \
    src/petcanvas.cpp \
    src/appconfig.cpp \
    src/settingsdialog.cpp \
    src/components/clockwidget.cpp \
    src/components/quotewidget.cpp \
    src/components/todowidget.cpp \
    src/components/bongocatwidget.cpp

# 头文件
HEADERS += \
    src/petwidget.h \
    src/petcanvas.h \
    src/appconfig.h \
    src/settingsdialog.h \
    src/components/componentbase.h \
    src/components/clockwidget.h \
    src/components/quotewidget.h \
    src/components/todowidget.h \
    src/components/bongocatwidget.h

# 资源文件
RESOURCES += \
    resources.qrc

# Windows 特有配置
win32 {
    LIBS += -luser32 -lshell32
    exists(app.rc) {
        RC_FILE = app.rc
    }
}

# 头文件搜索路径 (让 components/ 下的文件能找到 src/ 下的头文件)
INCLUDEPATH += src \
               src/components

# 输出目录
DESTDIR = bin
