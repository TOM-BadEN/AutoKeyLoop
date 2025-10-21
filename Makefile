#---------------------------------------------------------------------------------
# AutoKeyLoop 主 Makefile
# 用于编译 ovl 和 sys 模块，并将编译产物复制到 out 目录
#---------------------------------------------------------------------------------

.PHONY: all clean ovl sys prepare-out show-result

# 模块目录
OVL_DIR := ovl-AutoKeyLoop
SYS_DIR := sys-AutoKeyLoop
SYS_NOTIF_DIR := sys-Notification

# 输出目录
OUT_DIR := out
OUT_SWITCH := $(OUT_DIR)/switch
OUT_OVERLAYS := $(OUT_SWITCH)/.overlays
OUT_ATMOSPHERE := $(OUT_DIR)/atmosphere/contents/4100000002025924
OUT_ATMOSPHERE_BASE := $(OUT_DIR)/atmosphere/contents

# 编译产物路径
OVL_OUTPUT := $(OVL_DIR)/ovl-AutoKeyLoop.ovl
SYS_OUTPUT_DIR := $(SYS_DIR)/out/4100000002025924
SYS_NOTIF_SOURCE := $(SYS_NOTIF_DIR)/atmosphere/contents

# 状态文件
BUILD_STATUS := .build_status

# 颜色定义
COLOR_GREEN := \033[0;32m
COLOR_RED := \033[0;31m
COLOR_RESET := \033[0m

#---------------------------------------------------------------------------------
# 默认目标：编译所有模块并复制到 out 目录
#---------------------------------------------------------------------------------
all:
	@rm -f $(BUILD_STATUS)
	@$(MAKE) --no-print-directory ovl sys || true
	@$(MAKE) --no-print-directory show-result

#---------------------------------------------------------------------------------
# 显示最终编译结果
#---------------------------------------------------------------------------------
show-result:
	@echo "========================================"
	@if [ -f $(BUILD_STATUS) ]; then \
		echo "编译结果："; \
		while IFS= read -r line; do \
			if echo "$$line" | grep -q "编译失败"; then \
				printf "$(COLOR_RED)%s$(COLOR_RESET)\n" "$$line"; \
			elif echo "$$line" | grep -q "编译完成"; then \
				printf "$(COLOR_GREEN)%s$(COLOR_RESET)\n" "$$line"; \
			else \
				echo "$$line"; \
			fi; \
		done < $(BUILD_STATUS); \
		if grep -q "编译失败" $(BUILD_STATUS); then \
			rm -f $(BUILD_STATUS); \
			exit 1; \
		else \
			mkdir -p $(OUT_OVERLAYS); \
			mkdir -p $(OUT_ATMOSPHERE); \
			mkdir -p $(OUT_ATMOSPHERE_BASE); \
			mkdir -p $(OUT_SWITCH); \
			cp -f $(OVL_OUTPUT) $(OUT_OVERLAYS)/; \
			cp -rf $(SYS_OUTPUT_DIR)/* $(OUT_ATMOSPHERE)/; \
			if [ -d $(SYS_NOTIF_SOURCE) ]; then \
				cp -rf $(SYS_NOTIF_SOURCE)/* $(OUT_ATMOSPHERE_BASE)/; \
				echo "  sys-Notification 已复制到输出目录"; \
			fi; \
			echo "========================================"; \
			echo "  输出目录: $(OUT_DIR)/"; \
			echo "========================================"; \
			rm -f $(BUILD_STATUS); \
		fi; \
	fi

#---------------------------------------------------------------------------------
# 编译 overlay 模块
#---------------------------------------------------------------------------------
ovl:
	@echo "========================================"
	@echo "编译 overlay 模块..."
	@echo "========================================"
	@if $(MAKE) --no-print-directory -C $(OVL_DIR); then \
		echo "  模块 ovl-AutoKeyLoop，编译完成" >> $(BUILD_STATUS); \
	else \
		echo "  模块 ovl-AutoKeyLoop，编译失败" >> $(BUILD_STATUS); \
		exit 1; \
	fi

#---------------------------------------------------------------------------------
# 编译 sysmodule 模块
#---------------------------------------------------------------------------------
sys:
	@echo "========================================"
	@echo "编译 sysmodule 模块..."
	@echo "========================================"
	@if $(MAKE) --no-print-directory -C $(SYS_DIR); then \
		echo "  模块 sys-AutoKeyLoop，编译完成" >> $(BUILD_STATUS); \
	else \
		echo "  模块 sys-AutoKeyLoop，编译失败" >> $(BUILD_STATUS); \
		exit 1; \
	fi

#---------------------------------------------------------------------------------
# 准备输出目录
#---------------------------------------------------------------------------------
prepare-out:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OUT_OVERLAYS)
	@mkdir -p $(OUT_ATMOSPHERE)
	@mkdir -p $(OUT_SWITCH)

#---------------------------------------------------------------------------------
# 清理所有编译产物
#---------------------------------------------------------------------------------
clean:
	@echo "========================================"
	@echo "清理所有编译产物..."
	@echo "========================================"
	@echo "清理 overlay 模块..."
	@$(MAKE) --no-print-directory -C $(OVL_DIR) clean
	@echo "清理 sysmodule 模块..."
	@$(MAKE) --no-print-directory -C $(SYS_DIR) clean
	@echo "清理 out 目录..."
	@rm -rf $(OUT_DIR)
	@rm -f $(BUILD_STATUS)
	@echo "========================================"
	@echo "清理结果："
	@printf "$(COLOR_GREEN)  模块 ovl-AutoKeyLoop，已清理$(COLOR_RESET)\n"
	@printf "$(COLOR_GREEN)  模块 sys-AutoKeyLoop，已清理$(COLOR_RESET)\n"
	@printf "$(COLOR_GREEN)  输出目录 out/（包含 sys-Notification），已删除$(COLOR_RESET)\n"
	@echo "========================================"

