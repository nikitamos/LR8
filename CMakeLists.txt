cmake_minimum_required(VERSION 3.30)
project(lr8 LANGUAGES C CXX)

set(CURSES_NEED_NCURSES 1)
set(CURSES_NEED_WIDE 1)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})
add_compile_options(${CURSES_CFLAGS})

find_package(Qt6 COMPONENTS Core Network REQUIRED)

add_subdirectory(imtui)

add_library(lr8core SHARED
    src/metamagic.cpp
    src/qlastic.cpp
    src/metamagic.cpp
    src/serializer.cpp

    src/include/metamagic.h
    src/include/qlastic.h
    src/include/serializer.h
    src/include/window.h
)
target_include_directories(lr8core PRIVATE src/include)
target_link_libraries(lr8core imtui Qt6::Core Qt6::Network)

add_executable(task1 
    src/task1.cpp
    src/task1_mainwindow.cpp

    src/include/task1_mainwindow.h
    src/include/task1part.h
)
add_executable(task2
    src/task2.cpp
    src/task2_window.cpp

    src/include/task2_window.h
    src/include/task2book.h
)

set_target_properties(task1 task2 lr8core PROPERTIES
    CXX_STANDARD 20
    AUTOMOC ON
)
target_include_directories(task1 PRIVATE src/include)
target_link_libraries(task1 PRIVATE
    imtui
    ${CURSES_LIBRARIES}
    Qt6::Core Qt6::Network
    lr8core
)

target_include_directories(task2 PRIVATE src/include)
target_link_libraries(task2 PRIVATE
    imtui
    ${CURSES_LIBRARIES}
    Qt6::Core Qt6::Network
    lr8core
)
