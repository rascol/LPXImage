# Pre-install cleanup script to forcefully remove old libraries
# This ensures we're not mixing old and new library versions

message(STATUS "=== PRE-INSTALL CLEANUP ===")
message(STATUS "Forcefully removing old LPX libraries before installing new ones...")

# Define common library locations
set(HOMEBREW_SITE_PACKAGES "/opt/homebrew/lib/python3.13/site-packages")
set(USR_LOCAL_LIB "/usr/local/lib")
set(USR_LOCAL_INCLUDE "/usr/local/include")

# List of files to remove
set(OLD_LIBRARIES
    "${HOMEBREW_SITE_PACKAGES}/lpximage.cpython-313-darwin.so"
    "${HOMEBREW_SITE_PACKAGES}/liblpx_image.1.0.0.dylib"
    "${HOMEBREW_SITE_PACKAGES}/liblpx_image.1.dylib" 
    "${HOMEBREW_SITE_PACKAGES}/liblpx_image.dylib"
    "${USR_LOCAL_LIB}/liblpx_image.1.0.0.dylib"
    "${USR_LOCAL_LIB}/liblpx_image.1.dylib"
    "${USR_LOCAL_LIB}/liblpx_image.dylib"
    "${USR_LOCAL_INCLUDE}/lpx_image.h"
    "${USR_LOCAL_INCLUDE}/lpx_renderer.h"
    "${USR_LOCAL_INCLUDE}/lpx_mt.h"
    "${USR_LOCAL_INCLUDE}/lpx_webcam_server.h"
)

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
set(CRITICAL_FILES
    "${HOMEBREW_SITE_PACKAGES}/lpximage.cpython-313-darwin.so"
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
