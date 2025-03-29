cmake_minimum_required(VERSION 3.10)
project(log_polar_vision)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Include directories
include_directories(include)

# Add library
add_library(lpx_image SHARED
    src/lpx_globals.cpp
    src/lpx_image.cpp
    src/lpx_renderer.cpp
    src/mt_lpx_image.cpp
    src/lpx_logging.cpp
)

# Link OpenCV to the library
target_link_libraries(lpx_image ${OpenCV_LIBS})

# Add test executables
add_executable(test_lpx src/test_lpx.cpp)
target_link_libraries(test_lpx lpx_image ${OpenCV_LIBS})

# Add multithreaded test executable
add_executable(test_mt_lpx src/test_mt_lpx.cpp)
target_link_libraries(test_mt_lpx lpx_image ${OpenCV_LIBS})

# Add web server executable
add_executable(lpx_web_server src/lpx_web_server.cpp)
target_link_libraries(lpx_web_server lpx_image ${OpenCV_LIBS})

# Installation
install(TARGETS lpx_image DESTINATION lib)
install(FILES 
    include/lpx_image.h
    include/lpx_renderer.h
    include/lpx_mt.h
    DESTINATION include)
