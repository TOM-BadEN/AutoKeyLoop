#---------------------------------------------------------------------------------
# AutoKeyLoop 主 Makefile
# 用于编译 ovl 和 sys 模块，并将编译产物复制到 out 目录
#---------------------------------------------------------------------------------

.PHONY: all clean ovl sys prepare-out show-result

# 模块目录
OVL_DIR := ovl-AutoKeyLoop
SYS_DIR := sys-AutoKeyLoop

# 输出目录
OUT_DIR := out
OUT_SWITCH := $(OUT_DIR)/switch
OUT_OVERLAYS := $(OUT_SWITCH)/.overlays
OUT_ATMOSPHERE := $(OUT_DIR)/atmosphere/contents/4100000002025924

# 编译产物路径
OVL_OUTPUT := $(OVL_DIR)/ovl-AutoKeyLoop.ovl
SYS_OUTPUT_DIR := $(SYS_DIR)/out/4100000002025924

# 状态文件
BUILD_STATUS := .build_status

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
		cat $(BUILD_STATUS); \
		echo "========================================"; \
		if grep -q "编译失败" $(BUILD_STATUS); then \
			rm -f $(BUILD_STATUS); \
			exit 1; \
		else \
			echo "复制编译产物到 out 目录..."; \
			mkdir -p $(OUT_OVERLAYS); \
			mkdir -p $(OUT_ATMOSPHERE); \
			mkdir -p $(OUT_SWITCH); \
			cp -f $(OVL_OUTPUT) $(OUT_OVERLAYS)/; \
			cp -rf $(SYS_OUTPUT_DIR)/* $(OUT_ATMOSPHERE)/; \
			echo "========================================"; \
			echo "所有模块编译完成！"; \
			echo "输出目录: $(OUT_DIR)/"; \
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
	@echo "  模块 ovl-AutoKeyLoop，已清理"
	@echo "  模块 sys-AutoKeyLoop，已清理"
	@echo "  输出目录 out/，已删除"
	@echo "========================================"
	@echo "清理完成！"
	@echo "========================================"

