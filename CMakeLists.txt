cmake_minimum_required(VERSION 3.15)
project(DistributedImagingServices VERSION 1.0.0 LANGUAGES CXX)

# Author: Haobo (Brian) Liu
# Email: h349liu@gmail.com
# Project: Distributed Imaging Services for Voyis Interview

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable testing
enable_testing()

# Find required packages
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Common include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

# Common library
add_library(common STATIC
    src/common/message_protocol.cpp
    src/common/logger.cpp
)

target_link_libraries(common
    ${ZMQ_LIBRARIES}
)

# App 1: Image Generator
add_executable(image_generator
    src/image_generator/main.cpp
    src/image_generator/image_publisher.cpp
)

target_link_libraries(image_generator
    common
    ${OpenCV_LIBS}
    ${ZMQ_LIBRARIES}
    pthread
)

# App 2: Feature Extractor
add_executable(feature_extractor
    src/feature_extractor/main.cpp
    src/feature_extractor/sift_processor.cpp
)

target_link_libraries(feature_extractor
    common
    ${OpenCV_LIBS}
    ${ZMQ_LIBRARIES}
    pthread
)

# App 3: Data Logger
add_executable(data_logger
    src/data_logger/main.cpp
    src/data_logger/database_manager.cpp
)

target_link_libraries(data_logger
    common
    ${OpenCV_LIBS}
    ${ZMQ_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    pthread
)

# Unit Tests
add_executable(test_message_protocol
    tests/test_message_protocol.cpp
)

target_link_libraries(test_message_protocol
    common
)

add_executable(test_database
    tests/test_database.cpp
    src/data_logger/database_manager.cpp
)

target_link_libraries(test_database
    common
    ${SQLITE3_LIBRARIES}
)

# Register tests with CTest
add_test(NAME MessageProtocolTests COMMAND test_message_protocol)
add_test(NAME DatabaseTests COMMAND test_database)

# Installation
install(TARGETS image_generator feature_extractor data_logger
    RUNTIME DESTINATION bin
)

# Custom target to run tests and generate report
add_custom_target(test_report
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Running Unit Tests - Distributed Imaging Services"
    COMMAND ${CMAKE_COMMAND} -E echo "Author: Haobo Brian Liu"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Test Report Generated Successfully"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    DEPENDS test_message_protocol test_database
)
