# Custom clean script to remove both build artifacts and installed files

# First uninstall the installed files
if(EXISTS "${CMAKE_BINARY_DIR}/install_manifest.txt")
  file(READ "${CMAKE_BINARY_DIR}/install_manifest.txt" files)
  string(REGEX REPLACE "\n" ";" files "${files}")
  foreach(file ${files})
    message(STATUS "Uninstalling $ENV{DESTDIR}${file}")
    if(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E remove "$ENV{DESTDIR}${file}"
        RESULT_VARIABLE rm_retval
        OUTPUT_VARIABLE rm_out
        ERROR_VARIABLE rm_err
      )
      if(NOT "${rm_retval}" STREQUAL "0")
        message(WARNING "Problem when removing $ENV{DESTDIR}${file}: ${rm_err}")
      endif()
    else()
      message(STATUS "File $ENV{DESTDIR}${file} does not exist.")
    endif()
  endforeach()
else()
  message(STATUS "No install manifest found. No files will be uninstalled.")
endif()

# Also handle Python modules which might not be in the install manifest
find_package(Python QUIET)
if(Python_FOUND)
  execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(EXISTS "${PYTHON_SITE_PACKAGES}/lpximage")
    message(STATUS "Removing Python module: ${PYTHON_SITE_PACKAGES}/lpximage")
    file(REMOVE_RECURSE "${PYTHON_SITE_PACKAGES}/lpximage")
  endif()
  
  if(EXISTS "${PYTHON_SITE_PACKAGES}/lpximage.so")
    message(STATUS "Removing Python module: ${PYTHON_SITE_PACKAGES}/lpximage.so")
    file(REMOVE "${PYTHON_SITE_PACKAGES}/lpximage.so")
  endif()
  
  # Also check user site packages
  execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import site; print(site.getusersitepackages())"
    OUTPUT_VARIABLE PYTHON_USER_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(EXISTS "${PYTHON_USER_SITE_PACKAGES}/lpximage")
    message(STATUS "Removing Python module: ${PYTHON_USER_SITE_PACKAGES}/lpximage")
    file(REMOVE_RECURSE "${PYTHON_USER_SITE_PACKAGES}/lpximage")
  endif()
  
  if(EXISTS "${PYTHON_USER_SITE_PACKAGES}/lpximage.so")
    message(STATUS "Removing Python module: ${PYTHON_USER_SITE_PACKAGES}/lpximage.so")
    file(REMOVE "${PYTHON_USER_SITE_PACKAGES}/lpximage.so")
  endif()
endif()

message(STATUS "Clean completed. All installed files have been removed.")
