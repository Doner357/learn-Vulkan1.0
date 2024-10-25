ifndef TACO_COMMANDS_MK
TACO_COMMANDS_MK := 1

include tacomake/rules.mk

# Default target
all: $(build_target)

####################################################
# -- init --
# This call help you to create the directories
# stucture which fit this makefile.
####################################################
# Directories to be create
mkdirs := 	$(src_dir)		\
			$(include_dir)	\
			$(lib_dir)		\
			$(assets_dir)

# Run method
init:
	$(call msg,Starting to initialize workspace...)
	@$(call mkdir,$(call fixpath,$(mkdirs)))
	$(call msg,Initialization completed!)
	@cd .


####################################################
# -- run --
# This call help you to compile and run project.
####################################################
# File to be executed
execute			:= ./$(target)
execute_flags	:=

# Run method
run: $(build_target) $(target_assets)
	$(call msg,Start running program...)
	$(call msg,-------------------------- Start Running --------------------------)
	@cd $(target_dir) && $(call fixpath, $(execute)) $(execute_flags)
	$(call msg,-------------------------- End Running --------------------------)

$(target_dir)/%: $(assets_dir)/% | $(target_assets_dirS)
	$(call msg,Copy asset "$^" into target "$@"...)
	$(call cp,$(call fixpath,$^),$(call fixpath,$@))


####################################################
# -- clean --
# This call help you to clean up files generated
# by compiler.
# Default are obj files, dependency files and
# and executable file
####################################################
# Targets to be clean
clean_targets := 	$(build_target)	    \
					$(objs)			    \
					$(deps)

# Clean up all generated target
clean:
	$(call msg,Starting to delete created files...)
	@$(call rm,$(call fixpath,$(clean_targets)))
	$(call msg,Done!)
	@cd .


####################################################
# -- clean-all --
# This call help you to clean up all generated targets.
# Default are objs and build directories.
####################################################
# Targets to be clean
clean-all_targets := 	$(target_dir)	\
						$(obj_dir)

# Clean up all generated target
clean-all:
	$(call msg,Starting to delete created files...)
	@$(call rm,$(call fixpath,$(clean-all_targets)))
	$(call msg,Done!)
	@cd .


# Phony targets
.PHONY: all init run clean clean-all

endif