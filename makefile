# Deal with the difference of the operating system
# Usage: $(call function name,...)
# -------------------------------------------------------------
# fixpath       : Fix path with backslash
# mkdir         : Fix make directory call
# cp            : Fix copy file and directorie call
# rm            : Fix the remove files and directories call
# fixexecutable : Fix the different executable file suffix
ifdef OS
	fixexecutable = $1.exe
	rm            = for %%f in ($1) do if exist %%f\* (rmdir /s /q %%f) else if exist %%f (del /q %%f)
	fixpath       = $(subst /,\,$1)
	mkdir         = for %%f in ($1) do if not exist %%f mkdir %%f
	cp            = copy /y $1 $2
else
	ifeq ($(shell uname), Linux)
		fixexecutable = $1
		rm            = rm -f -r $1
		fixpath       = $1
		mkdir         = mkdir -p $1
		cp            = cp -f -r $1 $2
	endif
endif

# Utility functions
# -------------------------------------------------------------
# Find all targets in the directory with the specified pattern. Specify the pattern with * to get all contents.
# Usage: $(call rwildcard,directory,pattern)
rwildcard = $(wildcard $1/$2) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))
# Extract all directories in variable with no "/" suffix, and remove all duplicated directory
# Usage: $(call extrdir,$(var))
extrdir = $(sort $(patsubst %/,%,$(dir $1)))
# Extract all files with its path in a variable. Note that this can't really recognize if a target is 
# a file or directory. This is just a literal function, if two or more have the same prefix, the longest literal is taken.
# Usage: $(call extrfile,$(var))
extrfile = $(filter-out $(patsubst %/,%,$(dir $1)),$1)
# Extract all directories in specified directory with no "/" suffix, and remove all duplicated directory
# Usage: $(call dirwildcard,directory name)
dirwildcard = $(call extrdir,$(call rwildcard,$1,*))
# Extract all files in specified directory. Note that this can't really recognize if a target is 
# a file or directory. This is just a literal function, if two or more have the same prefix, the longest literal is taken.
# So this function can also be used to find deepest level target in each folder in a directory.
# Usage: $(call filewildcard,directory name)
filewildcard = $(call extrfile,$(call rwildcard,$1,*))
# Automatically scans libraries in specified directory and generates link directory flags.
# Usage: $(call libdirflags,libraries directory name)
libdirflags = $(addprefix -L,$(call dirwildcard,$1))
# Automatically scans libraries in specified directory and generates link flags.
# Its use is not recommended because link order is important for the linker.
# Not recommended to use it.
# Usage: $(call liblinkflags,libraries directory name)
liblinkflags = $(addprefix -l,$(patsubst lib%,%,$(basename $(notdir $(filter %.lib %.a %.so,$(call filewildcard,$1))))))
# Message function using echo
# Usage: $(call msg,message)
msg = @echo $1
# Comma
comma :=,


####################################################
# Compile
####################################################
# Target file
TARGET := learn_vulkan
TARGET := $(call fixexecutable,$(TARGET))

# CPP compiler
CXX := g++

# CPP version
CXXVERSION := c++20

# Directories
TARGET_DIR  := build
SRC_DIR     := src
INCLUDE_DIR := include
LIB_DIR     := lib
OBJ_DIR     := objs

# Directory contains resources for program
ASSETS_DIR := assets

# Compiler flags
CXXFLAGS := -g -Wall -std=$(CXXVERSION) -I$(INCLUDE_DIR) -Wextra -MMD -MP

# Linker flags
LDFLAGS := -static-libstdc++
# Link libraries, this will be append -l prefix automatically
LDLIBS := vulkan-1 glfw3 gdi32


# ---- Unchangeable ----

# Append directory to target file
override BUILDTARGET := $(TARGET_DIR)/$(TARGET)

# Source files
override CPP_SRCS := $(call rwildcard,$(SRC_DIR),*.cpp)
override C_SRCS   := $(call rwildcard,$(SRC_DIR),*.c)
override SRCS     := $(CPP_SRCS) $(C_SRCS)

# Object files
override OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))
override OBJS += $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRCS))

# Dependency files
override DEPS := $(OBJS:.o=.d)

# Determine final link flags
override LDFLAGS += $(call libdirflags,$(LIB_DIR)) $(addprefix -l,$(LDLIBS))

# Assets
override ASSETS := $(call filewildcard,$(ASSETS_DIR))
override TARGET_ASSETS := $(ASSETS:$(ASSETS_DIR)/%=$(TARGET_DIR)/%)
override TARGET_ASSETS_DIRS := $(call extrdir,$(TARGET_ASSETS))

# Directories needed by object files and dependency files
override OBJS_MIRROR_DIRS := $(call extrdir,$(OBJS))

# Integrate directories need to be created
override REQUIRED_DIRS := $(sort $(TARGET_DIR) $(TARGET_ASSETS_DIRS) $(OBJS_MIRROR_DIRS))

# Default target
all: $(BUILDTARGET)

# Rule to link executable
$(BUILDTARGET): $(OBJS) | $(TARGET_DIR)
	$(call msg,Starting linking...)
	@$(CXX) -o $@ $(OBJS) $(LDFLAGS)
	$(call msg,Building finished!)

# Rule to compile source files into object files
$(OBJ_DIR)/%.o: %.cpp | $(OBJS_MIRROR_DIRS)
	$(call msg,Compiling C++ source file to object file: $<)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	$(call msg,Compiling finished!)

# Rule to compile C source files into object files
$(OBJ_DIR)/%.o: %.c | $(OBJS_MIRROR_DIRS)
	$(call msg,Compiling C source file to object file: $<)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	$(call msg,Compiling finished!)

# Ensure the build directory exists
$(REQUIRED_DIRS):
	$(info Deteced missing directory "$@"$(comma) create new one...)
	@$(call mkdir,$(call fixpath, $@))


# Include dependency files if they exist
-include $(DEPS)


####################################################
# -- init --
# This call help you to create the directories
# stucture which fit this makefile.
####################################################
# Directories to be create
MKDIRS := 	$(SRC_DIR)		\
			$(INCLUDE_DIR)	\
			$(LIB_DIR)		\
			$(ASSETS_DIR)

# Run method
init:
	$(call msg,Starting to initialize workspace...)
	@$(call mkdir,$(call fixpath,$(MKDIRS)))
	$(call msg,Initialization completed!)
	@cd .


####################################################
# -- run --
# This call help you to compile and run project.
####################################################
# File to be executed
EXECUTE			:= ./$(TARGET)
EXECUTEFLAGS	:=

# Run method
run: $(BUILDTARGET) $(TARGET_ASSETS)
	$(call msg,Start running program...)
	$(call msg,-------------------------- Start Running --------------------------)
	@cd $(TARGET_DIR) && $(call fixpath, $(EXECUTE)) $(EXECUTEFLAGS)
	$(call msg,-------------------------- End Running --------------------------)

$(TARGET_DIR)/%: $(ASSETS_DIR)/% | $(TARGET_ASSETS_DIRS)
	$(info Copy asset "$^" into target "$@"...)
	$(call cp,$(call fixpath,$^),$(call fixpath,$@))


####################################################
# -- clean --
# This call help you to clean up files generated
# by compiler.
# Default are obj files, dependency files and
# and executable file
####################################################
# Targets to be clean
CL_TARGETS := 	$(BUILDTARGET)	    \
				$(OBJS)			    \
				$(DEPS)

# Clean up all generated target
clean:
	$(call msg,Starting to delete created files...)
	@$(call rm,$(call fixpath,$(CL_TARGETS)))
	$(call msg,Done!)
	@cd .


####################################################
# -- clean-all --
# This call help you to clean up all generated targets.
# Default are objs and build directories.
####################################################
# Targets to be clean
CLALL_TARGETS := 	$(TARGET_DIR)	\
					$(OBJ_DIR)

# Clean up all generated target
clean-all:
	$(call msg,Starting to delete created files...)
	@$(call rm,$(call fixpath,$(CLALL_TARGETS)))
	$(call msg,Done!)
	@cd .


# Phony targets
.PHONY: all init run clean clean-all