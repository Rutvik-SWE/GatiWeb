cmake_minimum_required(VERSION 3.16)

project(GatiWeb VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)

set(SOURCES
    src/main.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

add_executable(GatiWeb ${SOURCES})

target_include_directories(GatiWeb PRIVATE src)

# ---> OPTIMIZATION: Hardware-level Compiler Flags for Max Execution Speed <---
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(GatiWeb PRIVATE -O3 -flto)
    target_link_options(GatiWeb PRIVATE -flto)
elseif(MSVC)
    target_compile_options(GatiWeb PRIVATE /O2 /Oi /GL /Gy)
    target_link_options(GatiWeb PRIVATE /LTCG)
endif()

target_link_libraries(GatiWeb PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)