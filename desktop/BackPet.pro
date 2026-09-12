QT += core gui widgets
QT += network
QT += httpserver

# Live2D支持 - 多种方案自动检测
# 方案1: Qt WebEngine (标准方式, MSVC only on Windows)
# 方案2: Qt WebView (QML, MinGW compatible)
# 方案3: 均不可用

has_webengine = false
has_webview = false

# 检查WebEngine (MSVC only on Windows) - 必须同时有头文件和库
exists($$[QT_INSTALL_HEADERS]/QtWebEngineWidgets/QWebEngineView) {
    exists($$[QT_INSTALL_LIBS]/Qt6WebEngineWidgets.lib)|exists($$[QT_INSTALL_LIBS]/libQt6WebEngineWidgets.a) {
        has_webengine = true
    }
}

# 检查WebView (QML模块) - 必须同时有 QtQuickWidgets 头文件 (QQuickWidget) 和 WebView 库
exists($$[QT_INSTALL_HEADERS]/QtQuickWidgets/QQuickWidget) {
    exists($$[QT_INSTALL_HEADERS]/QtWebView/QtWebView) {
        exists($$[QT_INSTALL_LIBS]/Qt6WebView.lib)|exists($$[QT_INSTALL_LIBS]/libQt6WebView.a) {
            has_webview = true
        }
    }
}

# 优先使用WebEngine, 否则使用WebView
has_live2d = false
equals(has_webengine, true) {
    QT += webenginewidgets webchannel
    DEFINES += HAS_LIVE2D_SUPPORT
    DEFINES += USE_QT_WEBENGINE
    has_live2d = true
    message("=== Live2D support: Qt WebEngine ENABLED ===")
} else:equals(has_webview, true) {
    QT += quick quickwidgets webview
    DEFINES += HAS_LIVE2D_SUPPORT
    DEFINES += USE_QT_WEBVIEW
    has_live2d = true
    message("=== Live2D support: Qt WebView (QML) ENABLED ===")
} else {
    message("=== Live2D support NOT available ===")
    message("=== Install Qt WebEngine (MSVC) or Qt WebView + QtQuickWidgets via Maintenance Tool ===")
}

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
    src/components/bongocatwidget.cpp \
    src/components/bongomodelmanager.cpp \
    src/components/localfileserver.cpp

# Live2D源文件 (需要Live2D支持 - WebEngine或WebView均可)
equals(has_live2d, true) {
    SOURCES += src/components/live2dwidget.cpp
}

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
    src/components/bongocatwidget.h \
    src/components/bongomodelmanager.h \
    src/components/localfileserver.h

# Live2D头文件
equals(has_live2d, true) {
    HEADERS += src/components/live2dwidget.h
}

# 资源文件
RESOURCES += \
    resources.qrc

# Windows 特有配置
win32 {
    # XInput 用于手柄轮询 (L-104)，链接 xinput.lib 或使用动态加载 (我们代码用动态加载，不用链接)
    LIBS += -luser32 -lshell32
    exists(app.rc) {
        RC_FILE = app.rc
    }
}

# 头文件搜索路径
INCLUDEPATH += src \
               src/components

# 输出目录
DESTDIR = bin
