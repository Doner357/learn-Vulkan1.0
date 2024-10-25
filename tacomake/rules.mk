ifndef TACO_RULES_MK
TACO_RULES_MK := 1

# ---- Unchangeable ----
override src_suffix_s := $(sort $(src_suffix))

override target := $(call fixexecutable,$(target))

# Append directory to target file
override build_target := $(target_dir)/$(target)

# Source files
override srcs := $(sort $(foreach d,$(src_suffix_s),$(call rwildcard,$(src_dir),*$d)))

# Object files
override objs := $(sort $(foreach d,$(src_suffix_s),$(patsubst %$d,$(obj_dir)/%.o,$(filter %$d,$(srcs)))))

# Dependency files
override deps := $(objs:.o=.d)

# Determine final compile flags
override compile_flags += -std=$(cpp_ver) $(addprefix -I,$(include_dir) $(ext_include_dir)) $(addprefix -D,$(macros))

# Determine final link flags
override linker_flags += $(foreach d,$(lib_dir) $(ext_lib_dir),$(call libdirflags,$d)) $(addprefix -l,$(libraries))

# Assets
override assets := $(call filewildcard,$(assets_dir))
override target_assets := $(assets:$(assets_dir)/%=$(target_dir)/%)
override target_assets_dirS := $(call extrdir,$(target_assets))

# Directories needed by object files and dependency files
override objs_mirror_dirs := $(call extrdir,$(objs))


override spv_shaders_srcs := $(sort $(foreach d,$(spv_shaders_src_suffix),$(call rwildcard,$(src_dir),*$d)))
# Target SPIR-V shaders
override spv_shaders_targets := $(patsubst $(spv_shaders_src_dir)/%,$(spv_shaders_target_dir)/%.spv,$(spv_shaders_srcs))
# Directories needed by SPIR-V shaders
override spv_mirror_dirs := $(call extrdir,$(spv_shaders_targets))


# Integrate directories need to be created
override required_dirs := $(sort $(target_dir) $(target_assets_dirS) $(objs_mirror_dirs) $(spv_mirror_dirs))


# Rule to link executable
$(build_target): $(objs) $(spv_shaders_targets) | $(target_dir)
	$(call msg,Starting linking...)
	@$(compiler) -o $@ $(objs) $(linker_flags)
	$(call msg,Building finished!)

# Dynamic compile rule for different source suffix
define compile
$$(obj_dir)/%.o: %$1 Makefile | $$(objs_mirror_dirs)
	$$(call msg,Compiling source file to object file: $$<)
	@$$(compiler) $$(compile_flags) -c $$< -o $$@
	$$(call msg,Compiling finished!)

endef
# Compile
$(eval $(foreach d,$(src_suffix_s),$(call compile,$d)))


# For SPIR-V shaders
$(spv_shaders_target_dir)/%.spv: $(spv_shaders_src_dir)/% | $(spv_mirror_dirs)
	$(call msg,Compiling GLSL shader file "$<" to SPIR-V shader "$@".)
	@$(spv_compiler) $< -o $@
	$(call msg,Compiling finished!)


# Ensure the build directory exists
$(required_dirs):
	$(call msg,Deteced missing directory "$@"$(comma) create new one...)
	@$(call mkdir,$(call fixpath,$@))


# Include dependency files if they exist
-include $(deps)

endif