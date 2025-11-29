cmake_minimum_required(VERSION 3.10)
project(wcapd C)



add_executable(wcapd src/main.c) 

add_library(sqlite3 include/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC include/sqlite3)

target_include_directories(wcapd PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include/miniaudio
)

target_link_libraries(wcapd PRIVATE sqlite3)
