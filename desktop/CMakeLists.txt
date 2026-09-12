cmake_minimum_required(VERSION 3.16)
project(BackPet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

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
    WebEngineWidgets
)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/petwidget.cpp
    src/petcanvas.cpp
    src/appconfig.cpp
    src/settingsdialog.cpp
    src/components/clockwidget.cpp
    src/components/quotewidget.cpp
    src/components/todowidget.cpp
    src/components/bongocatwidget.cpp
    src/components/bongomodelmanager.cpp
    src/components/localfileserver.cpp
    src/components/live2dwidget.cpp
)

set(HEADERS
    src/petwidget.h
    src/petcanvas.h
    src/appconfig.h
    src/settingsdialog.h
    src/components/componentbase.h
    src/components/clockwidget.h
    src/components/quotewidget.h
    src/components/todowidget.h
    src/components/bongocatwidget.h
    src/components/bongomodelmanager.h
    src/components/localfileserver.h
    src/components/live2dwidget.h
)

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

target_compile_definitions(BackPet PRIVATE
    HAS_LIVE2D_SUPPORT
    USE_QT_WEBENGINE
)

target_link_libraries(BackPet PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::HttpServer
    Qt6::WebEngineWidgets
)

if(WIN32)
    target_link_libraries(BackPet PRIVATE user32 shell32)
endif()
