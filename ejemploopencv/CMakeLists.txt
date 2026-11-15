cmake_minimum_required(VERSION 3.19)
project(ejemploopencv LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

find_package(OpenCV REQUIRED COMPONENTS core imgproc)

qt_add_executable(ejemploopencv
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(ejemploopencv
    PRIVATE
        Qt::Core
        Qt::Widgets
        ${OpenCV_LIBS}
)

install(TARGETS ejemploopencv
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ejemploopencv
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
