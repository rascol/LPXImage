# Pre-install cleanup script to forcefully remove old libraries
# This ensures we're not mixing old and new library versions

message(STATUS "=== PRE-INSTALL CLEANUP ===")
message(STATUS "Forcefully removing old LPX libraries before installing new ones...")

# Define common library locations
set(USR_LOCAL_LIB "/usr/local/lib")
set(USR_LOCAL_INCLUDE "/usr/local/include")
set(HOMEBREW_LIB "/opt/homebrew/lib")

# Find all Python site-packages directories
file(GLOB PYTHON_SITE_PACKAGES_DIRS "/opt/homebrew/lib/python*/site-packages")
file(GLOB USER_PYTHON_SITE_PACKAGES_DIRS "/Users/ray/Library/Python/*/lib/python/site-packages")

# Initialize list of files to remove
set(OLD_LIBRARIES)

# Add system-wide C++ libraries
list(APPEND OLD_LIBRARIES
    "${USR_LOCAL_LIB}/liblpx_image.1.0.0.dylib"
    "${USR_LOCAL_LIB}/liblpx_image.1.dylib"
    "${USR_LOCAL_LIB}/liblpx_image.dylib"
    "${USR_LOCAL_INCLUDE}/lpx_image.h"
    "${USR_LOCAL_INCLUDE}/lpx_renderer.h"
    "${USR_LOCAL_INCLUDE}/lpx_mt.h"
    "${USR_LOCAL_INCLUDE}/lpx_webcam_server.h"
    "${USR_LOCAL_INCLUDE}/lpx_file_server.h"
    "${HOMEBREW_LIB}/liblpx_image.1.0.0.dylib"
    "${HOMEBREW_LIB}/liblpx_image.1.dylib"
    "${HOMEBREW_LIB}/liblpx_image.dylib"
)

# Add Python libraries from ALL Python versions (3.11, 3.12, 3.13, etc.)
foreach(python_dir ${PYTHON_SITE_PACKAGES_DIRS})
    if(EXISTS "${python_dir}")
        # Standard library files
        file(GLOB python_libs "${python_dir}/lpximage.cpython-*.so")
        list(APPEND OLD_LIBRARIES ${python_libs})
        
        list(APPEND OLD_LIBRARIES
            "${python_dir}/liblpx_image.1.0.0.dylib"
            "${python_dir}/liblpx_image.1.dylib"
            "${python_dir}/liblpx_image.dylib"
        )
        
        # Editable install artifacts (from development installs)
        file(GLOB editable_pth "${python_dir}/__editable__.lpximage-*.pth")
        file(GLOB editable_finder "${python_dir}/__editable___lpximage_*_finder.py")
        file(GLOB dist_info "${python_dir}/lpximage-*.dist-info")
        file(GLOB pycache_editable "${python_dir}/__pycache__/__editable___lpximage_*")
        
        list(APPEND OLD_LIBRARIES ${editable_pth} ${editable_finder} ${dist_info} ${pycache_editable})
    endif()
endforeach()

# Add user-specific Python libraries
foreach(user_python_dir ${USER_PYTHON_SITE_PACKAGES_DIRS})
    if(EXISTS "${user_python_dir}")
        file(GLOB user_python_libs "${user_python_dir}/lpximage.cpython-*.so")
        file(GLOB user_lib_files "${user_python_dir}/liblpx_image.*")
        file(GLOB user_dist_info "${user_python_dir}/lpximage-*.dist-info")
        
        list(APPEND OLD_LIBRARIES ${user_python_libs} ${user_lib_files} ${user_dist_info})
    endif()
endforeach()

# Also remove any files in the project root that might be old (but NOT build directory)
set(ROOT_OLD_FILES
    "${CMAKE_SOURCE_DIR}/liblpx_image.1.0.0.dylib"
    "${CMAKE_SOURCE_DIR}/liblpx_image.1.dylib"
    "${CMAKE_SOURCE_DIR}/liblpx_image.dylib"
    "${CMAKE_SOURCE_DIR}/lpximage.cpython-313-darwin.so"
)

# Only add root files if they exist and are not in build directory
foreach(root_file ${ROOT_OLD_FILES})
    if(NOT "${root_file}" MATCHES "${CMAKE_BINARY_DIR}")
        list(APPEND OLD_LIBRARIES "${root_file}")
    endif()
endforeach()

# Remove each file with detailed logging
foreach(lib_file ${OLD_LIBRARIES})
    if(EXISTS "${lib_file}")
        message(STATUS "REMOVING: ${lib_file}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E rm -f "${lib_file}"
            RESULT_VARIABLE rm_result
            OUTPUT_VARIABLE rm_output
            ERROR_VARIABLE rm_error
        )
        if(NOT rm_result EQUAL 0)
            message(WARNING "Failed to remove ${lib_file}: ${rm_error}")
        else()
            message(STATUS "  ✅ Successfully removed: ${lib_file}")
        endif()
    else()
        message(STATUS "  ℹ️  Not found (OK): ${lib_file}")
    endif()
endforeach()

# Verify removal - check that critical files are gone
message(STATUS "=== VERIFICATION ===")

# Check current Python version's site-packages
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(PYTHON_SITE_PACKAGES "${Python3_SITELIB}" ABSOLUTE)

set(CRITICAL_FILES
    "${PYTHON_SITE_PACKAGES}/lpximage.cpython-313-darwin.so"
    "${USR_LOCAL_LIB}/liblpx_image.1.0.0.dylib"
)

set(CLEANUP_SUCCESS TRUE)
foreach(critical_file ${CRITICAL_FILES})
    if(EXISTS "${critical_file}")
        message(FATAL_ERROR "❌ CLEANUP FAILED: ${critical_file} still exists!")
        set(CLEANUP_SUCCESS FALSE)
    else()
        message(STATUS "  ✅ Verified removed: ${critical_file}")
    endif()
endforeach()

if(CLEANUP_SUCCESS)
    message(STATUS "=== PRE-INSTALL CLEANUP COMPLETED SUCCESSFULLY ===")
else()
    message(FATAL_ERROR "Pre-install cleanup failed - aborting install")
endif()
