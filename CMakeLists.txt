cmake_minimum_required(VERSION 3.10)
project(log_polar_vision)

if(APPLE)
  # Check for Homebrew OpenCV first
  if(EXISTS /opt/homebrew/lib/cmake/opencv4)
    set(OpenCV_DIR /opt/homebrew/lib/cmake/opencv4)
  elseif(EXISTS /usr/local/lib/cmake/opencv4)
    set(OpenCV_DIR /usr/local/lib/cmake/opencv4)
  endif()
endif()

set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(PROJECT_BUILD_DIR ${CMAKE_BINARY_DIR})

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV package
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs)

message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")

include_directories(${OpenCV_INCLUDE_DIRS})

# Include directories
include_directories(include)

# Add library
add_library(lpx_image SHARED
    src/lpx_globals.cpp
    src/mt_lpx_renderer.cpp
    src/mt_lpx_image.cpp
    src/lpx_logging.cpp
)

# This ensures that executable targets can see the symbols from the lpx_image library
set_target_properties(lpx_image PROPERTIES
    # Comment out the visibility settings as they may be causing issues
    # CXX_VISIBILITY_PRESET hidden
    # VISIBILITY_INLINES_HIDDEN YES
    SOVERSION 1
    VERSION 1.0.0
)

target_include_directories(lpx_image PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)
# target_include_directories(lpx_image PRIVATE ${OpenCV_INCLUDE_DIRS})

# Link with PUBLIC visibility so executables using the library can also see OpenCV
target_link_libraries(lpx_image PUBLIC ${OpenCV_LIBS})

# Add multithreaded test executable
add_executable(test_mt_lpx src/test_mt_lpx.cpp)
target_link_libraries(test_mt_lpx PRIVATE lpx_image ${OpenCV_LIBS})

# Add the webcam server executable
add_executable(main_webcam_server 
    src/main_webcam_server.cpp 
    src/lpx_webcam_server.cpp
    src/mt_lpx_image.cpp
    src/mt_lpx_renderer.cpp
    src/lpx_globals.cpp
    src/lpx_logging.cpp
)

# Add the debug renderer executable
add_executable(main_debug_renderer 
    src/main_debug_renderer.cpp
    src/lpx_webcam_server.cpp
    src/mt_lpx_image.cpp
    src/mt_lpx_renderer.cpp
    src/lpx_globals.cpp
    src/lpx_logging.cpp
)

target_link_libraries(main_webcam_server 
    ${OpenCV_LIBS}
    pthread
)

# Link libraries for debug renderer
target_link_libraries(main_debug_renderer
    ${OpenCV_LIBS}
    pthread
)

# Installation
install(TARGETS lpx_image DESTINATION lib)
install(FILES 
    include/lpx_image.h
    include/lpx_renderer.h
    include/lpx_mt.h
    include/lpx_webcam_server.h
    DESTINATION include)