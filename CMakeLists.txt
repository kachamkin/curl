cmake_minimum_required(VERSION 3.5)

project(curl VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        http.cpp
        mainwindow.h
        mainwindow.ui
        resource.h
)

include_directories("C:/Users/kacha/.nuget/boost_1_80_0")
include_directories("C:/Program Files/OpenSSL-Win64/include")
link_directories("C:/Program Files/OpenSSL-Win64/lib/VC")
set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/Project1.rc")
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(curl
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES} main.cpp ${app_icon_resource_windows}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET curl APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(curl SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(curl
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(curl PRIVATE libcrypto64MD libssl64MD Qt${QT_VERSION_MAJOR}::Widgets)
add_custom_command(TARGET curl POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "C:/Program Files/OpenSSL-Win64/bin/libssl-3-x64.dll" $<TARGET_FILE_DIR:curl>)
add_custom_command(TARGET curl POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "C:/Program Files/OpenSSL-Win64/bin/libcrypto-3-x64.dll" $<TARGET_FILE_DIR:curl>)

set_target_properties(curl PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS curl
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(curl)
endif()
