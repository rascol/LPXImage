# Find Python
find_package(Python 3.6 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Found Python: ${Python_VERSION}")

# Set minimum CMake version for pybind11
set(PYBIND11_MINIMUM_CMAKE_VERSION 3.15)

# Add pybind11
# First try to use local pybind11 directory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pybind11/CMakeLists.txt)
    message(STATUS "Using local pybind11 directory")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pybind11)
else()
    # Fall back to FetchContent
    message(STATUS "Using FetchContent to get pybind11")
    include(FetchContent)
    FetchContent_Declare(
      pybind11
      GIT_REPOSITORY https://github.com/pybind/pybind11.git
      GIT_TAG        v2.12.0  # Using a more recent version
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# Create the Python module
pybind11_add_module(lpximage lpx_bindings.cpp)

# Link against our lpx_image library
target_link_libraries(lpximage PRIVATE lpx_image ${OpenCV_LIBS})

# Add dependency to ensure C++ library is built first
add_dependencies(lpximage lpx_image)

# Install the module
install(TARGETS lpximage DESTINATION ${Python_SITEARCH})
