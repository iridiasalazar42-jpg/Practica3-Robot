cmake_minimum_required(VERSION 3.19)
project(prueba_qmysql LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Sql)

qt_standard_project_setup()

qt_add_executable(prueba_qmysql
    main.cpp
)

target_link_libraries(prueba_qmysql
    PRIVATE
        Qt::Core
        Qt::Sql


    target_link_libraries(tienda_qt PRIVATE Qt6::Core Qt6::Gui Qt6::Sql)
)