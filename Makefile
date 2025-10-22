#---------------------------------------------------------------------------------
# AutoKeyLoop 主 Makefile
# 用于编译 ovl、sys 和 sys-Notification 模块，并将编译产物复制到 out 目录
#---------------------------------------------------------------------------------

.PHONY: all clean ovl sys notif prepare-out show-result check update

# 模块目录
OVL_DIR := ovl-AutoKeyLoop
SYS_DIR := sys-AutoKeyLoop
SUBMODULE_DIR := sys-AutoKeyLoop/lib/libnotification
SYS_NOTIF_DIR := $(SUBMODULE_DIR)/sys-Notification

# 输出目录
OUT_DIR := out
OUT_SWITCH := $(OUT_DIR)/switch
OUT_OVERLAYS := $(OUT_SWITCH)/.overlays
OUT_ATMOSPHERE := $(OUT_DIR)/atmosphere/contents/4100000002025924
OUT_ATMOSPHERE_BASE := $(OUT_DIR)/atmosphere/contents

# 编译产物路径
OVL_OUTPUT := $(OVL_DIR)/ovl-AutoKeyLoop.ovl
SYS_OUTPUT_DIR := $(SYS_DIR)/out/4100000002025924
SYS_NOTIF_OUTPUT_DIR := $(SYS_NOTIF_DIR)/out/atmosphere/contents

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
	@$(MAKE) --no-print-directory ovl sys notif || true
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
		if [ -d $(SYS_NOTIF_OUTPUT_DIR) ]; then \
			cp -rf $(SYS_NOTIF_OUTPUT_DIR)/* $(OUT_ATMOSPHERE_BASE)/; \
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
# 编译 sys-Notification 模块
#---------------------------------------------------------------------------------
notif:
	@echo "========================================"
	@echo "编译 sys-Notification 模块..."
	@echo "========================================"
	@if $(MAKE) --no-print-directory -C $(SYS_NOTIF_DIR); then \
		echo "  模块 sys-Notification，编译完成" >> $(BUILD_STATUS); \
	else \
		echo "  模块 sys-Notification，编译失败" >> $(BUILD_STATUS); \
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
	@echo "清理 sys-Notification 模块..."
	@$(MAKE) --no-print-directory -C $(SYS_NOTIF_DIR) clean
	@echo "清理 out 目录..."
	@rm -rf $(OUT_DIR)
	@rm -f $(BUILD_STATUS)
	@echo "========================================"
	@echo "清理结果："
	@printf "$(COLOR_GREEN)  模块 ovl-AutoKeyLoop，已清理$(COLOR_RESET)\n"
	@printf "$(COLOR_GREEN)  模块 sys-AutoKeyLoop，已清理$(COLOR_RESET)\n"
	@printf "$(COLOR_GREEN)  模块 sys-Notification，已清理$(COLOR_RESET)\n"
	@printf "$(COLOR_GREEN)  输出目录 out/，已删除$(COLOR_RESET)\n"
	@echo "========================================"

#---------------------------------------------------------------------------------
# 检查子模块是否是最新版本
#---------------------------------------------------------------------------------
check:
	@echo "========================================"
	@echo "检查 libnotification 子模块状态..."
	@echo "========================================"
	@if git -C $(CURDIR)/$(SUBMODULE_DIR) fetch origin main 2>/dev/null; then \
		LOCAL=$$(git -C $(CURDIR)/$(SUBMODULE_DIR) rev-parse HEAD); \
		REMOTE=$$(git -C $(CURDIR)/$(SUBMODULE_DIR) rev-parse origin/main); \
		if [ "$$LOCAL" = "$$REMOTE" ]; then \
			printf "$(COLOR_GREEN)✓ 已是最新版本$(COLOR_RESET)\n"; \
		else \
			printf "$(COLOR_RED)⚠ 有新版本可用$(COLOR_RESET)\n"; \
			echo "  当前: $${LOCAL:0:8}"; \
			echo "  最新: $${REMOTE:0:8}"; \
			echo "  运行 'make update' 更新"; \
		fi; \
	else \
		printf "$(COLOR_RED)✗ 网络错误，无法连接 GitHub$(COLOR_RESET)\n"; \
	fi
	@echo "========================================"

#---------------------------------------------------------------------------------
# 更新子模块
#---------------------------------------------------------------------------------
update:
	@echo "========================================"
	@echo "更新 libnotification 子模块..."
	@echo "========================================"
	@if git -C $(CURDIR)/$(SUBMODULE_DIR) fetch origin main 2>/dev/null; then \
		LOCAL=$$(git -C $(CURDIR)/$(SUBMODULE_DIR) rev-parse HEAD); \
		REMOTE=$$(git -C $(CURDIR)/$(SUBMODULE_DIR) rev-parse origin/main); \
		if [ "$$LOCAL" = "$$REMOTE" ]; then \
			printf "$(COLOR_GREEN)✓ 已是最新版本，无需更新$(COLOR_RESET)\n"; \
		else \
			echo "  正在更新..."; \
			git submodule update --remote $(SUBMODULE_DIR); \
			printf "$(COLOR_GREEN)✓ 更新完成$(COLOR_RESET)\n"; \
			echo "  版本: $${REMOTE:0:8}"; \
		fi; \
	else \
		printf "$(COLOR_RED)✗ 更新失败，无法连接 GitHub$(COLOR_RESET)\n"; \
	fi
	@echo "========================================"

