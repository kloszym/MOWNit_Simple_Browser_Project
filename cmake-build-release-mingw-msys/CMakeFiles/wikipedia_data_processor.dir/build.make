# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 4.0

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

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = D:\MSys\mingw64\bin\cmake.exe

# The command to remove a file.
RM = D:\MSys\mingw64\bin\cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = D:\Mownit\Browser

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = D:\Mownit\Browser\cmake-build-release-mingw-msys

# Include any dependencies generated for this target.
include CMakeFiles/wikipedia_data_processor.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/wikipedia_data_processor.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/wikipedia_data_processor.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/wikipedia_data_processor.dir/flags.make

CMakeFiles/wikipedia_data_processor.dir/codegen:
.PHONY : CMakeFiles/wikipedia_data_processor.dir/codegen

CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj: CMakeFiles/wikipedia_data_processor.dir/flags.make
CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj: CMakeFiles/wikipedia_data_processor.dir/includes_CXX.rsp
CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj: D:/Mownit/Browser/wikipedia_data_processor.cpp
CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj: CMakeFiles/wikipedia_data_processor.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=D:\Mownit\Browser\cmake-build-release-mingw-msys\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj"
	D:\MSys\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj -MF CMakeFiles\wikipedia_data_processor.dir\wikipedia_data_processor.cpp.obj.d -o CMakeFiles\wikipedia_data_processor.dir\wikipedia_data_processor.cpp.obj -c D:\Mownit\Browser\wikipedia_data_processor.cpp

CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.i"
	D:\MSys\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E D:\Mownit\Browser\wikipedia_data_processor.cpp > CMakeFiles\wikipedia_data_processor.dir\wikipedia_data_processor.cpp.i

CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.s"
	D:\MSys\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S D:\Mownit\Browser\wikipedia_data_processor.cpp -o CMakeFiles\wikipedia_data_processor.dir\wikipedia_data_processor.cpp.s

# Object files for target wikipedia_data_processor
wikipedia_data_processor_OBJECTS = \
"CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj"

# External object files for target wikipedia_data_processor
wikipedia_data_processor_EXTERNAL_OBJECTS =

wikipedia_data_processor.exe: CMakeFiles/wikipedia_data_processor.dir/wikipedia_data_processor.cpp.obj
wikipedia_data_processor.exe: CMakeFiles/wikipedia_data_processor.dir/build.make
wikipedia_data_processor.exe: D:/MSys/mingw64/lib/libcurl.dll.a
wikipedia_data_processor.exe: D:/MSys/mingw64/lib/libQt6Core.dll.a
wikipedia_data_processor.exe: CMakeFiles/wikipedia_data_processor.dir/linkLibs.rsp
wikipedia_data_processor.exe: CMakeFiles/wikipedia_data_processor.dir/objects1.rsp
wikipedia_data_processor.exe: CMakeFiles/wikipedia_data_processor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=D:\Mownit\Browser\cmake-build-release-mingw-msys\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable wikipedia_data_processor.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\wikipedia_data_processor.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/wikipedia_data_processor.dir/build: wikipedia_data_processor.exe
.PHONY : CMakeFiles/wikipedia_data_processor.dir/build

CMakeFiles/wikipedia_data_processor.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\wikipedia_data_processor.dir\cmake_clean.cmake
.PHONY : CMakeFiles/wikipedia_data_processor.dir/clean

CMakeFiles/wikipedia_data_processor.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" D:\Mownit\Browser D:\Mownit\Browser D:\Mownit\Browser\cmake-build-release-mingw-msys D:\Mownit\Browser\cmake-build-release-mingw-msys D:\Mownit\Browser\cmake-build-release-mingw-msys\CMakeFiles\wikipedia_data_processor.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/wikipedia_data_processor.dir/depend

