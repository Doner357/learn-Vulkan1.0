ifndef TACO_FUNCTIONS_MK
TACO_FUNCTIONS_MK := 1

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

endif