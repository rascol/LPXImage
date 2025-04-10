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

# Determine the correct Python site-packages directory
execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import site; print(site.getsitepackages()[0])"
  OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE SITE_PACKAGES_RESULT
)

# If the above method fails, try another approach
if(NOT PYTHON_SITE_PACKAGES OR NOT "${SITE_PACKAGES_RESULT}" EQUAL 0)
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

message(STATUS "Python site-packages directory: ${PYTHON_SITE_PACKAGES}")

# Make sure we found a valid directory
if(NOT PYTHON_SITE_PACKAGES)
  message(FATAL_ERROR "Could not determine Python site-packages directory")
endif()

# Install the module to the correct site-packages directory
install(TARGETS lpximage DESTINATION "${PYTHON_SITE_PACKAGES}")
