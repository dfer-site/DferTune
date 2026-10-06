export GITHASH 		:= $(shell git -c safe.directory=$(CURDIR) rev-parse --short HEAD 2>/dev/null || echo unknown)
export VERSION := 5.6.0-dfer.1
export API_VERSION 	:= 8
export WANT_FLAC 	:= 1
export WANT_MP3 	:= 1
export WANT_WAV 	:= 1

# 使用上游 libryazhahand 的固定提交。
# 该提交包含 Switch 2 风格渲染器 (ult::useSwitch2Style),
# 悬浮菜单将其作为库的标准功能使用。只有在完成
# 完整的 devkitA64 CI 验证后,才能修改这个固定提交。
LIBRYAZHAHAND_REPO ?= https://github.com/Dimasick-git/libryazhahand.git
LIBRYAZHAHAND_PIN  ?= fd11fe0e31a3c73a293504710a8336a5c74d4254
RYAZHAHAND_DIR     ?= overlay/lib/libryazhahand

all: overlay nxExt module

clean:
	$(MAKE) -C DferTune/nxExt clean
	$(MAKE) -C overlay clean
	$(MAKE) -C DferTune clean
	-rm -r dist
	-rm DferTune-*-*.zip

prepare-overlay-lib:
	@if [ ! -d "$(RYAZHAHAND_DIR)/.git" ]; then \
		echo "Cloning libryazhahand into $(RYAZHAHAND_DIR)..."; \
		rm -rf "$(RYAZHAHAND_DIR)"; \
		mkdir -p "$(dir $(RYAZHAHAND_DIR))"; \
		git clone "$(LIBRYAZHAHAND_REPO)" "$(RYAZHAHAND_DIR)"; \
	fi
	@cd "$(RYAZHAHAND_DIR)" && \
		git fetch --quiet origin "$(LIBRYAZHAHAND_PIN)" 2>/dev/null || true; \
		git checkout --quiet "$(LIBRYAZHAHAND_PIN)"
	@if [ ! -f "$(RYAZHAHAND_DIR)/ryazhahand.mk" ]; then \
		echo "Missing $(RYAZHAHAND_DIR)/ryazhahand.mk -- bad clone?" >&2; \
		exit 1; \
	fi

overlay: prepare-overlay-lib
	$(MAKE) -C overlay

nxExt:
	$(MAKE) -C DferTune/nxExt

module: nxExt
	$(MAKE) -C DferTune

dist: all
	rm -rf dist
	mkdir -p dist/switch/.overlays
		mkdir -p dist/atmosphere/contents/420000000000000F/flags
			mkdir -p dist/config/DferTune/lang
			touch dist/atmosphere/contents/420000000000000F/flags/boot2.flag
			cp DferTune/DferTune.nsp dist/atmosphere/contents/420000000000000F/exefs.nsp
			cp overlay/DferTune-Overlay.ovl dist/switch/.overlays/
			cp overlay/lang/*.json dist/config/DferTune/lang/
		cp DferTune/toolbox.json dist/atmosphere/contents/420000000000000F/
	cd dist; zip -r DferTune-$(VERSION)-$(GITHASH).zip ./**/; cd ../;
	-hactool -t nso DferTune/DferTune.nso

.PHONY: all clean overlay nxExt module dist prepare-overlay-lib
