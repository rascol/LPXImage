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

# RPATH settings for robust library loading
if(APPLE)
    # macOS RPATH settings
    set(CMAKE_INSTALL_RPATH "@loader_path;@loader_path/../lib;@loader_path/../../lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_MACOSX_RPATH ON)
elseif(UNIX AND NOT APPLE)
    # Linux RPATH settings
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:$ORIGIN/../../lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
elseif(WIN32)
    # Windows doesn't use RPATH but rather relies on PATH or DLL placement
    # Make sure DLLs are copied to the output directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

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
    src/optimized_scan.cpp   # Added optimized scanning
    src/lpx_logging.cpp
    src/lpx_webcam_server.cpp
    src/lpx_file_server.cpp  # Added file server implementation
    src/lpx_version.cpp      # Added version tracking
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


# Add the webcam server executable
add_executable(main_webcam_server 
    src/main_webcam_server.cpp
)

# Add the file server executable
add_executable(main_file_server 
    src/main_file_server.cpp
)

# Add the debug renderer executable
add_executable(main_debug_renderer 
    src/main_debug_renderer.cpp
)

target_link_libraries(main_webcam_server 
    lpx_image
    ${OpenCV_LIBS}
    pthread
)

# Link libraries for file server
target_link_libraries(main_file_server
    lpx_image
    ${OpenCV_LIBS}
    pthread
)

# Link libraries for debug renderer
target_link_libraries(main_debug_renderer
    lpx_image
    ${OpenCV_LIBS}
    pthread
)


# Option to build Python bindings
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

if(BUILD_PYTHON_BINDINGS)
    # Add the python subdirectory
    add_subdirectory(python)
endif()

# Pre-install cleanup to forcefully remove old libraries
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pre_install_cleanup.cmake)")

# Configure enhanced clean script that also uninstalls
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clean.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_clean.cmake"
  IMMEDIATE @ONLY)

# Add a custom target that runs after 'make clean' to also uninstall
add_custom_target(cleanall
  COMMAND ${CMAKE_MAKE_PROGRAM} clean
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_clean.cmake
  COMMENT "Cleaning build and installed files...")

# Also provide a separate uninstall target for convenience
add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_clean.cmake
  COMMENT "Uninstalling files...")

# Installation - support both system and local installation
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Custom install prefix was provided (e.g., for Python builds)
    message(STATUS "Installing to custom prefix: ${CMAKE_INSTALL_PREFIX}")
else()
    # Default to local installation in project directory
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Installation directory" FORCE)
    message(STATUS "Installing locally to: ${CMAKE_INSTALL_PREFIX}")
endif()

install(TARGETS lpx_image 
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(FILES 
    include/lpx_image.h
    include/lpx_renderer.h
    include/lpx_mt.h
    include/lpx_webcam_server.h
    include/lpx_file_server.h
    DESTINATION include
)
