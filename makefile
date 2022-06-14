PROJECT_NAME := coelacanth

ifeq ($(OS),Windows_NT)
  detected_os := Windows
else
  detected_os := $(shell uname)
endif
ifeq ($(detected_os),Linux)
  include scripts/makefile_linux
endif
ifeq ($(detected_os),Windows)
  include scripts/makefile_windows
endif
