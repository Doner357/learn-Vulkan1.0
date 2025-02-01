####################################################
# C/C++
####################################################
# Target file
target := learn_vulkan

# CPP compiler
compiler := g++

# CPP version
cpp_ver := c++20

# Directories
target_dir  := build
src_dir     := src
include_dir := include
lib_dir     := lib
obj_dir     := objs

# Directory contains resources for program
assets_dir := assets

# If you need to connect to external include and library directories,
# note that these directories won't be created when using "make init".
# Note: When assigning multiple values to the same variable in a Makefile,
# remember to add a backslash (\) at the end of each line.
ext_include_dir := 
ext_lib_dir     := 

# Compiler flags
compile_flags := -O5 -g -Wall -Wextra -MMD -MP

# Define Macros
macros := 

# Linker flags
linker_flags := -static
# Link libraries, this will be append -l prefix automatically. For example, "gdi32" will become -lgdi32.
# So just type the libraries name you want to link, like "gdi32 opengl32 pthread".
libraries := vulkan-1 glfw3 gdi32


# Source file suffix configurations (Not recommended to change)
src_suffix := .cpp .cxx .cp .c++ .CPP .cc .c .C



####################################################
# SPIR-V
####################################################
# Shaders compiler
spv_compiler := tools/glslc

# Shaders Directories
spv_shaders_src_dir    := $(src_dir)/shaders
spv_shaders_target_dir := $(target_dir)/shaders


# Source file suffix configurations (Not recommended to change)
spv_shaders_src_suffix := .vert .frag .geom .tesc .tese .rgen .rahit .rchit .rmiss .rcall .comp


####################################################
####################################################
include tacomake/platform.mk
include tacomake/functions.mk
include tacomake/rules.mk
include tacomake/commands.mk