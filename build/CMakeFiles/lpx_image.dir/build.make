# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.30.3/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.30.3/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/ray/Desktop/LPXImage

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/ray/Desktop/LPXImage/build

# Include any dependencies generated for this target.
include CMakeFiles/lpx_image.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/lpx_image.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/lpx_image.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/lpx_image.dir/flags.make

CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o: CMakeFiles/lpx_image.dir/flags.make
CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o: /Users/ray/Desktop/LPXImage/src/lpx_globals.cpp
CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o: CMakeFiles/lpx_image.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/ray/Desktop/LPXImage/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o -MF CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o.d -o CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o -c /Users/ray/Desktop/LPXImage/src/lpx_globals.cpp

CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/ray/Desktop/LPXImage/src/lpx_globals.cpp > CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.i

CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/ray/Desktop/LPXImage/src/lpx_globals.cpp -o CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.s

CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o: CMakeFiles/lpx_image.dir/flags.make
CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o: /Users/ray/Desktop/LPXImage/src/mt_lpx_renderer.cpp
CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o: CMakeFiles/lpx_image.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/ray/Desktop/LPXImage/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o -MF CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o.d -o CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o -c /Users/ray/Desktop/LPXImage/src/mt_lpx_renderer.cpp

CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/ray/Desktop/LPXImage/src/mt_lpx_renderer.cpp > CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.i

CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/ray/Desktop/LPXImage/src/mt_lpx_renderer.cpp -o CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.s

CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o: CMakeFiles/lpx_image.dir/flags.make
CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o: /Users/ray/Desktop/LPXImage/src/mt_lpx_image.cpp
CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o: CMakeFiles/lpx_image.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/ray/Desktop/LPXImage/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o -MF CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o.d -o CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o -c /Users/ray/Desktop/LPXImage/src/mt_lpx_image.cpp

CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/ray/Desktop/LPXImage/src/mt_lpx_image.cpp > CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.i

CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/ray/Desktop/LPXImage/src/mt_lpx_image.cpp -o CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.s

CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o: CMakeFiles/lpx_image.dir/flags.make
CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o: /Users/ray/Desktop/LPXImage/src/lpx_logging.cpp
CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o: CMakeFiles/lpx_image.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/ray/Desktop/LPXImage/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o -MF CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o.d -o CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o -c /Users/ray/Desktop/LPXImage/src/lpx_logging.cpp

CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/ray/Desktop/LPXImage/src/lpx_logging.cpp > CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.i

CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/ray/Desktop/LPXImage/src/lpx_logging.cpp -o CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.s

# Object files for target lpx_image
lpx_image_OBJECTS = \
"CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o" \
"CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o" \
"CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o" \
"CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o"

# External object files for target lpx_image
lpx_image_EXTERNAL_OBJECTS =

liblpx_image.1.0.0.dylib: CMakeFiles/lpx_image.dir/src/lpx_globals.cpp.o
liblpx_image.1.0.0.dylib: CMakeFiles/lpx_image.dir/src/mt_lpx_renderer.cpp.o
liblpx_image.1.0.0.dylib: CMakeFiles/lpx_image.dir/src/mt_lpx_image.cpp.o
liblpx_image.1.0.0.dylib: CMakeFiles/lpx_image.dir/src/lpx_logging.cpp.o
liblpx_image.1.0.0.dylib: CMakeFiles/lpx_image.dir/build.make
liblpx_image.1.0.0.dylib: /opt/homebrew/lib/libopencv_highgui.4.11.0.dylib
liblpx_image.1.0.0.dylib: /opt/homebrew/lib/libopencv_videoio.4.11.0.dylib
liblpx_image.1.0.0.dylib: /opt/homebrew/lib/libopencv_imgcodecs.4.11.0.dylib
liblpx_image.1.0.0.dylib: /opt/homebrew/lib/libopencv_imgproc.4.11.0.dylib
liblpx_image.1.0.0.dylib: /opt/homebrew/lib/libopencv_core.4.11.0.dylib
liblpx_image.1.0.0.dylib: CMakeFiles/lpx_image.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/ray/Desktop/LPXImage/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking CXX shared library liblpx_image.dylib"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lpx_image.dir/link.txt --verbose=$(VERBOSE)
	$(CMAKE_COMMAND) -E cmake_symlink_library liblpx_image.1.0.0.dylib liblpx_image.1.dylib liblpx_image.dylib

liblpx_image.1.dylib: liblpx_image.1.0.0.dylib
	@$(CMAKE_COMMAND) -E touch_nocreate liblpx_image.1.dylib

liblpx_image.dylib: liblpx_image.1.0.0.dylib
	@$(CMAKE_COMMAND) -E touch_nocreate liblpx_image.dylib

# Rule to build all files generated by this target.
CMakeFiles/lpx_image.dir/build: liblpx_image.dylib
.PHONY : CMakeFiles/lpx_image.dir/build

CMakeFiles/lpx_image.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lpx_image.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lpx_image.dir/clean

CMakeFiles/lpx_image.dir/depend:
	cd /Users/ray/Desktop/LPXImage/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/ray/Desktop/LPXImage /Users/ray/Desktop/LPXImage /Users/ray/Desktop/LPXImage/build /Users/ray/Desktop/LPXImage/build /Users/ray/Desktop/LPXImage/build/CMakeFiles/lpx_image.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/lpx_image.dir/depend

