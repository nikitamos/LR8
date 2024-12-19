cmake_minimum_required(VERSION 3.30)
project(lr6 LANGUAGES C CXX)

add_subdirectory(corrosion)
corrosion_import_crate(MANIFEST_PATH Cargo.toml)

set(CURSES_NEED_NCURSES 1)
set(CURSES_NEED_WIDE 1)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})
add_compile_options(${CURSES_CFLAGS})

add_subdirectory(imtui)

add_executable(task1 src/frontend/task1.cpp)
target_link_libraries(task1 PRIVATE lr8backend imtui ${CURSES_LIBRARIES} ${lr8backend_LIBRARIES})
