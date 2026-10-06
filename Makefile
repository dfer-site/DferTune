export GITHASH 		:= $(shell git -c safe.directory=$(CURDIR) rev-parse --short HEAD 2>/dev/null || echo unknown)
export VERSION := 5.6.0
export API_VERSION 	:= 8
export WANT_FLAC 	:= 1
export WANT_MP3 	:= 1
export WANT_WAV 	:= 1

# Используем закреплённую ревизию открытого libryazhahand владельца.
# Этот pin содержит Switch 2 style renderer (ult::useSwitch2Style), который
# используется оверлеем как штатная библиотечная возможность. Изменять pin
# можно только вместе с полной devkitA64-проверкой CI.
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
