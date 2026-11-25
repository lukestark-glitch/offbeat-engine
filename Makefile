# Offbeat – Build für Plugin (VST3 + LV2, x86_64) und Offline-Renderer.
#   make            Plugin + offbeat-render
#   make install    nach ~/Library/Audio/Plug-Ins kopieren und ad-hoc signieren
#   make check      Kick-Abnahme mit offbeat-render

CXX ?= c++
DSP_SRC = src/dsp/Engine.cpp src/dsp/Params.cpp
DSP_HDR = $(wildcard src/dsp/*.hpp)
VST3_DIR = $(HOME)/Library/Audio/Plug-Ins/VST3
LV2_DIR  = $(HOME)/Library/Audio/Plug-Ins/LV2

all: plugin render

plugin:
	$(MAKE) -C plugin
	$(MAKE) -C dpf/utils/lv2-ttl-generator
	dpf/utils/generate-ttl.sh $(CURDIR)/bin

render: bin/offbeat-render

bin/offbeat-render: tools/offbeat-render.cpp $(DSP_SRC) $(DSP_HDR)
	@mkdir -p bin
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Isrc/dsp tools/offbeat-render.cpp $(DSP_SRC) -o $@

check: render
	bin/offbeat-render --preset "Kick - Hard Bounce" --note F1 --bpm 160 --check

install: plugin
	rm -rf "$(VST3_DIR)/offbeat.vst3" "$(LV2_DIR)/offbeat.lv2"
	mkdir -p "$(VST3_DIR)" "$(LV2_DIR)"
	cp -R bin/offbeat.vst3 "$(VST3_DIR)/"
	cp -R bin/offbeat.lv2 "$(LV2_DIR)/"
	codesign --force --deep -s - "$(VST3_DIR)/offbeat.vst3"
	codesign --force -s - "$(LV2_DIR)/offbeat.lv2/offbeat_dsp.dylib"  # LV2-Ordner ist kein Bundle

clean:
	rm -rf bin build

.PHONY: all plugin render check install clean
