cmake_minimum_required(VERSION 3.30)
project(lr6 LANGUAGES C CXX)

set(CURSES_NEED_NCURSES 1)
set(CURSES_NEED_WIDE 1)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})
add_compile_options(${CURSES_CFLAGS})

find_package(Qt6 COMPONENTS Core Network REQUIRED)
set(CMAKE_AUTOMOC On)

add_subdirectory(imtui)

add_executable(task1 
    src/task1.cpp
    src/serializer.cpp
    src/metamagic.cpp
    src/qlastic.cpp
    src/metamagic.cpp
    src/task1_mainwindow.cpp

    src/include/metamagic.h
    src/include/qlastic.h
    src/include/task1_mainwindow.h
    src/include/task1part.h
    src/include/window.h
)
set_target_properties(task1 PROPERTIES
    CXX_STANDARD 20)
target_include_directories(task1 PRIVATE src/include)
target_link_libraries(task1 PRIVATE
    imtui
    ${CURSES_LIBRARIES}
    Qt6::Core Qt6::Network
)
