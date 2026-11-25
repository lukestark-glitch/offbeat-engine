#!/usr/bin/make -f
# DPF-Build des Plugins. Aufruf über das Makefile im Projektordner.

NAME = offbeat

FILES_DSP = \
	OffbeatPlugin.cpp \
	../src/dsp/Engine.cpp \
	../src/dsp/Params.cpp

# Params.cpp nutzt auch die UI; sie steckt bei VST3 und LV2 (eine Datei) im selben Binary.
FILES_UI = OffbeatUI.cpp

DPF_PATH = ../dpf
DPF_BUILD_DIR = ../build/offbeat
DPF_TARGET_DIR = ../bin
include ../dpf/Makefile.plugins.mk

BUILD_CXX_FLAGS += -std=gnu++17 -I../src/dsp

TARGETS += vst3
TARGETS += lv2

all: $(TARGETS)
