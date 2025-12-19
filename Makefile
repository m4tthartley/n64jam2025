##
##	Created by Matt Hartley on 10/12/2025.
##	Copyright 2025 GiantJelly. All rights reserved.
##

BUILD_DIR=build

include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

N64_CFLAGS += -std=gnu2x -Os

src = main.c

all: n64jam2025.z64

GLB_FILES := $(wildcard assets/*.glb)
PNG_FILES := $(wildcard assets/*.png)
T3DM_FILES := $(GLB_FILES:assets/%.glb=filesystem/%.t3dm) $(PNG_FILES:assets/%.png=filesystem/%.sprite)

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/%.t3dm: assets/%.glb
	@mkdir -p $(dir $@)
	@echo "    [T3D-MODEL] $@"
	$(T3D_GLTF_TO_3D) "$<" $@
	$(N64_BINDIR)/mkasset -c 2 -o filesystem $@

$(BUILD_DIR)/n64jam2025.dfs: $(T3DM_FILES)
$(BUILD_DIR)/n64jam2025.elf: $(src:%.c=$(BUILD_DIR)/%.o)

n64jam2025.z64: N64_ROM_TITLE="N64 Jam 2025"
n64jam2025.z64: $(BUILD_DIR)/n64jam2025.dfs

clean:
	rm -rf $(BUILD_DIR) *.z64

build_lib:
	rm -rf $(BUILD_DIR) *.z64
	make -C $(T3D_INST)
	make all

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
