# Offbeat Engine – Build für Plugin (VST3, VST2, AU, CLAP, LV2) und Offline-Renderer.
#   make            Plugin + offbeat-render (macOS: Universal-Binary für Intel und Apple Silicon)
#   make install    macOS: nach ~/Library/Audio/Plug-Ins kopieren und ad-hoc signieren
#   make check      Kick-Abnahme mit offbeat-render
# Windows (MSYS2/MinGW) und Linux: `make` baut nach bin/; Installation siehe README.

CXX ?= c++
DSP_SRC = src/dsp/Engine.cpp src/dsp/Params.cpp src/dsp/ParamText.cpp
DSP_HDR = $(wildcard src/dsp/*.hpp) tools/Measure.hpp
PLUGINS_DIR = $(HOME)/Library/Audio/Plug-Ins
VST3_DIR = $(PLUGINS_DIR)/VST3
LV2_DIR  = $(PLUGINS_DIR)/LV2

ifeq ($(shell uname -s),Darwin)
# Universal-Binary: läuft auf Intel- und Apple-Silicon-Macs
ARCH_FLAGS = -arch x86_64 -arch arm64 -mmacosx-version-min=10.15
PLUGIN_ENV = CFLAGS="$(ARCH_FLAGS)" CXXFLAGS="$(ARCH_FLAGS)" LDFLAGS="$(ARCH_FLAGS)" NOOPT=true  # NOOPT: ohne x86-only-Flags (-mfpmath=sse)
endif

all: plugin render

plugin:
	$(PLUGIN_ENV) $(MAKE) -C plugin
	$(MAKE) -C dpf/utils/lv2-ttl-generator
	dpf/utils/generate-ttl.sh $(CURDIR)/bin

render: bin/offbeat-render

bin/offbeat-render: tools/offbeat-render.cpp $(DSP_SRC) $(DSP_HDR)
	@mkdir -p bin
	$(CXX) -std=gnu++17 -O2 -Wall -Wextra -Isrc/dsp tools/offbeat-render.cpp $(DSP_SRC) -o $@

check: render
	bin/offbeat-render --preset "Kick - Hard Bounce" --note F1 --bpm 160 --check

install: plugin
	rm -rf "$(VST3_DIR)/offbeat.vst3" "$(LV2_DIR)/offbeat.lv2" "$(PLUGINS_DIR)/VST/offbeat.vst" \
	       "$(PLUGINS_DIR)/Components/offbeat.component" "$(PLUGINS_DIR)/CLAP/offbeat.clap"
	mkdir -p "$(VST3_DIR)" "$(LV2_DIR)" "$(PLUGINS_DIR)/VST" "$(PLUGINS_DIR)/Components" "$(PLUGINS_DIR)/CLAP"
	cp -R bin/offbeat.vst3 "$(VST3_DIR)/"
	cp -R bin/offbeat.lv2 "$(LV2_DIR)/"
	cp -R bin/offbeat.vst "$(PLUGINS_DIR)/VST/"
	cp -R bin/offbeat.component "$(PLUGINS_DIR)/Components/"
	cp -R bin/offbeat.clap "$(PLUGINS_DIR)/CLAP/"
	codesign --force --deep -s - "$(VST3_DIR)/offbeat.vst3"
	codesign --force -s - "$(LV2_DIR)/offbeat.lv2/offbeat.dylib"  # LV2-Ordner ist kein Bundle
	codesign --force --deep -s - "$(PLUGINS_DIR)/VST/offbeat.vst"
	codesign --force --deep -s - "$(PLUGINS_DIR)/Components/offbeat.component"
	codesign --force --deep -s - "$(PLUGINS_DIR)/CLAP/offbeat.clap"
	killall -9 AudioComponentRegistrar 2>/dev/null || true  # AU-Liste neu einlesen

clean:
	rm -rf bin build

.PHONY: all plugin render check install clean
