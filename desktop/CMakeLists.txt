cmake_minimum_required(VERSION 3.16)
project(BackPet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出目录 (与 BackPet.pro 的 DESTDIR=bin 一致)
# 可通过 -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=... 覆盖
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY OR CMAKE_RUNTIME_OUTPUT_DIRECTORY STREQUAL "")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif()

# 自动处理 MOC/RCC/UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    HttpServer
    Multimedia
    Svg
)

qt_standard_project_setup()

# ============================================================
# Live2D 支持自动检测 (与 BackPet.pro 逻辑一致)
#   方案1: Qt WebEngine (标准方式, MSVC only on Windows)
#   方案2: Qt WebView (QML, MinGW compatible)
#   方案3: 均不可用 (静态图片回退模式)
# ============================================================
set(HAS_LIVE2D OFF)
set(LIVE2D_BACKEND "")

# 方案1: WebEngine
find_package(Qt6 QUIET COMPONENTS WebEngineWidgets WebChannel)
if(Qt6WebEngineWidgets_FOUND)
    set(HAS_LIVE2D ON)
    set(LIVE2D_BACKEND "QT_WEBENGINE")
    set(LIVE2D_LIBS Qt6::WebEngineWidgets Qt6::WebChannel)
    message(STATUS "=== Live2D support: Qt WebEngine ENABLED ===")
else()
    # 方案2: WebView (QML + QQuickWidget)
    find_package(Qt6 QUIET COMPONENTS Quick QuickWidgets WebView)
    if(Qt6Quick_FOUND AND Qt6QuickWidgets_FOUND AND Qt6WebView_FOUND)
        set(HAS_LIVE2D ON)
        set(LIVE2D_BACKEND "QT_WEBVIEW")
        set(LIVE2D_LIBS Qt6::Quick Qt6::QuickWidgets Qt6::WebView)
        message(STATUS "=== Live2D support: Qt WebView (QML) ENABLED ===")
    else()
        message(WARNING
            "=== Live2D support NOT available ===\n"
            "Install Qt WebEngine (MSVC) or Qt WebView + QtQuickWidgets via Maintenance Tool.\n"
            "BongoCat 将退回静态图片模式")
    endif()
endif()

set(SOURCES
    src/main.cpp
    src/petwidget.cpp
    src/petcanvas.cpp
    src/appconfig.cpp
    src/components/clockwidget.cpp
    src/components/quotewidget.cpp
    src/components/todowidget.cpp
    src/components/bongocatwidget.cpp
    src/components/bongomodelmanager.cpp
    src/components/petstage.cpp
    src/components/localfileserver.cpp
    src/preference/thememanager.cpp
    src/preference/iconkit.cpp
    src/preference/preferencewindow.cpp
    src/preference/widgets/switchbutton.cpp
    src/preference/widgets/prolist.cpp
    src/preference/widgets/prolistitem.cpp
    src/preference/widgets/navitem.cpp
    src/preference/pages/catpage.cpp
)

set(HEADERS
    src/petwidget.h
    src/petcanvas.h
    src/appconfig.h
    src/components/componentbase.h
    src/components/clockwidget.h
    src/components/quotewidget.h
    src/components/todowidget.h
    src/components/bongocatwidget.h
    src/components/bongomodelmanager.h
    src/components/petstage.h
    src/components/localfileserver.h
    src/preference/thememanager.h
    src/preference/iconkit.h
    src/preference/preferencewindow.h
    src/preference/widgets/switchbutton.h
    src/preference/widgets/prolist.h
    src/preference/widgets/prolistitem.h
    src/preference/widgets/navitem.h
    src/preference/pages/catpage.h
)

# Live2D 源文件 (需要 Live2D 支持 - WebEngine 或 WebView 均可)
if(HAS_LIVE2D)
    list(APPEND SOURCES src/components/live2dwidget.cpp)
    list(APPEND HEADERS src/components/live2dwidget.h)
endif()

set(RESOURCES
    resources.qrc
)

qt_add_executable(BackPet WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(BackPet PRIVATE
    src
    src/components
)

# Windows 特有配置 (与 BackPet.pro win32 块一致)
if(WIN32)
    target_link_libraries(BackPet PRIVATE user32 shell32)
    # 版本资源文件 (可选, 与 BackPet.pro 的 RC_FILE 一致)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)
        target_sources(BackPet PRIVATE app.rc)
    endif()
endif()

target_compile_definitions(BackPet PRIVATE
    QT_DEPRECATED_WARNINGS
)

if(HAS_LIVE2D)
    target_compile_definitions(BackPet PRIVATE
        HAS_LIVE2D_SUPPORT
        # USE_QT_WEBENGINE=MSVC 后端 / USE_QT_WEBVIEW=MinGW 后端
        $<IF:$<STREQUAL:${LIVE2D_BACKEND},QT_WEBENGINE>,USE_QT_WEBENGINE,USE_QT_WEBVIEW>
    )
    target_link_libraries(BackPet PRIVATE ${LIVE2D_LIBS})
endif()

target_link_libraries(BackPet PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::HttpServer
    Qt6::Multimedia
    Qt6::Svg
)
