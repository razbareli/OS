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
include CMakeFiles/sigint_handler.dir/depend.make
# Include the progress variables for this target.
include CMakeFiles/sigint_handler.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/sigint_handler.dir/flags.make

CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.o: CMakeFiles/sigint_handler.dir/flags.make
CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.o: ../demo_singInt_handler.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.o -c /cs/usr/unixraz/OS/ex2/ex2/demo_singInt_handler.c

CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /cs/usr/unixraz/OS/ex2/ex2/demo_singInt_handler.c > CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.i

CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /cs/usr/unixraz/OS/ex2/ex2/demo_singInt_handler.c -o CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.s

# Object files for target sigint_handler
sigint_handler_OBJECTS = \
"CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.o"

# External object files for target sigint_handler
sigint_handler_EXTERNAL_OBJECTS =

sigint_handler: CMakeFiles/sigint_handler.dir/demo_singInt_handler.c.o
sigint_handler: CMakeFiles/sigint_handler.dir/build.make
sigint_handler: CMakeFiles/sigint_handler.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable sigint_handler"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sigint_handler.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/sigint_handler.dir/build: sigint_handler
.PHONY : CMakeFiles/sigint_handler.dir/build

CMakeFiles/sigint_handler.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/sigint_handler.dir/cmake_clean.cmake
.PHONY : CMakeFiles/sigint_handler.dir/clean

CMakeFiles/sigint_handler.dir/depend:
	cd /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /cs/usr/unixraz/OS/ex2/ex2 /cs/usr/unixraz/OS/ex2/ex2 /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug /cs/usr/unixraz/OS/ex2/ex2/cmake-build-debug/CMakeFiles/sigint_handler.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/sigint_handler.dir/depend

