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
CMAKE_SOURCE_DIR = /cs/usr/unixraz/OS/ex2/ex2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/yechiel_new.dir/depend.make
# Include the progress variables for this target.
include CMakeFiles/yechiel_new.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/yechiel_new.dir/flags.make

CMakeFiles/yechiel_new.dir/uthreads_new.cpp.o: CMakeFiles/yechiel_new.dir/flags.make
CMakeFiles/yechiel_new.dir/uthreads_new.cpp.o: ../uthreads_new.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/yechiel_new.dir/uthreads_new.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/yechiel_new.dir/uthreads_new.cpp.o -c /cs/usr/unixraz/OS/ex2/ex2/uthreads_new.cpp

CMakeFiles/yechiel_new.dir/uthreads_new.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/yechiel_new.dir/uthreads_new.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/unixraz/OS/ex2/ex2/uthreads_new.cpp > CMakeFiles/yechiel_new.dir/uthreads_new.cpp.i

CMakeFiles/yechiel_new.dir/uthreads_new.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/yechiel_new.dir/uthreads_new.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/unixraz/OS/ex2/ex2/uthreads_new.cpp -o CMakeFiles/yechiel_new.dir/uthreads_new.cpp.s

CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.o: CMakeFiles/yechiel_new.dir/flags.make
CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.o: ../tests/yechiel.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.o -c /cs/usr/unixraz/OS/ex2/ex2/tests/yechiel.cpp

CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /cs/usr/unixraz/OS/ex2/ex2/tests/yechiel.cpp > CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.i

CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /cs/usr/unixraz/OS/ex2/ex2/tests/yechiel.cpp -o CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.s

# Object files for target yechiel_new
yechiel_new_OBJECTS = \
"CMakeFiles/yechiel_new.dir/uthreads_new.cpp.o" \
"CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.o"

# External object files for target yechiel_new
yechiel_new_EXTERNAL_OBJECTS =

yechiel_new: CMakeFiles/yechiel_new.dir/uthreads_new.cpp.o
yechiel_new: CMakeFiles/yechiel_new.dir/tests/yechiel.cpp.o
yechiel_new: CMakeFiles/yechiel_new.dir/build.make
yechiel_new: CMakeFiles/yechiel_new.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable yechiel_new"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/yechiel_new.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/yechiel_new.dir/build: yechiel_new
.PHONY : CMakeFiles/yechiel_new.dir/build

CMakeFiles/yechiel_new.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/yechiel_new.dir/cmake_clean.cmake
.PHONY : CMakeFiles/yechiel_new.dir/clean

CMakeFiles/yechiel_new.dir/depend:
	cd /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /cs/usr/unixraz/OS/ex2/ex2 /cs/usr/unixraz/OS/ex2/ex2 /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug/CMakeFiles/yechiel_new.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/yechiel_new.dir/depend
