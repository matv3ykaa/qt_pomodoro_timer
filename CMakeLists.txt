cmake_minimum_required(VERSION 3.19)
project(Antiprocrastinator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

add_executable(antiprocrastinator
    src/main.cpp
    src/headers/antiprocrastinator.h
    src/app/antiprocrastinator.cpp
    src/headers/quotesdialog.h
    src/app/quotesdialog.cpp

    .env
    quotes/quotes.txt
    .env
)

target_link_libraries(antiprocrastinator PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)

# Копируем ресурсы в директорию сборки
configure_file(quotes/quotes.txt ${CMAKE_CURRENT_BINARY_DIR}/quotes/quotes.txt COPYONLY)
configure_file(.env ${CMAKE_CURRENT_BINARY_DIR}/.env COPYONLY)
