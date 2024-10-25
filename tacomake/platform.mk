ifndef TACO_PLATFORM_MK
TACO_PLATFORM_MK := 1

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

endif