# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.20

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
CMAKE_COMMAND = /usr/local/APP/jetbrains/clion/2021.2/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /usr/local/APP/jetbrains/clion/2021.2/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /cs/usr/unixraz/OS/ex5

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /cs/usr/unixraz/OS/ex5/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/ex5.dir/depend.make
# Include the progress variables for this target.
include CMakeFiles/ex5.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ex5.dir/flags.make

CMakeFiles/ex5.dir/main.cpp.o: CMakeFiles/ex5.dir/flags.make
CMakeFiles/ex5.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/unixraz/OS/ex5/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ex5.dir/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ex5.dir/main.cpp.o -c /cs/usr/unixraz/OS/ex5/main.cpp

CMakeFiles/ex5.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ex5.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/unixraz/OS/ex5/main.cpp > CMakeFiles/ex5.dir/main.cpp.i

CMakeFiles/ex5.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ex5.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/unixraz/OS/ex5/main.cpp -o CMakeFiles/ex5.dir/main.cpp.s

# Object files for target ex5
ex5_OBJECTS = \
"CMakeFiles/ex5.dir/main.cpp.o"

# External object files for target ex5
ex5_EXTERNAL_OBJECTS =

ex5: CMakeFiles/ex5.dir/main.cpp.o
ex5: CMakeFiles/ex5.dir/build.make
ex5: CMakeFiles/ex5.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/cs/usr/unixraz/OS/ex5/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ex5"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ex5.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ex5.dir/build: ex5
.PHONY : CMakeFiles/ex5.dir/build

CMakeFiles/ex5.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ex5.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ex5.dir/clean

CMakeFiles/ex5.dir/depend:
	cd /cs/usr/unixraz/OS/ex5/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /cs/usr/unixraz/OS/ex5 /cs/usr/unixraz/OS/ex5 /cs/usr/unixraz/OS/ex5/cmake-build-debug /cs/usr/unixraz/OS/ex5/cmake-build-debug /cs/usr/unixraz/OS/ex5/cmake-build-debug/CMakeFiles/ex5.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ex5.dir/depend
