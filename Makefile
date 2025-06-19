.PHONY: build
PROJECT_NAME = "Template"
# build:
# 	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
build: 
	@cmake -B cmake-build -DCMAKE_PREFIX_PATH=~/dev/JUCE
run:
	@cmake --build cmake-build
	@open ./cmake-build/$(PROJECT_NAME)_artefacts/Standalone/TapPluginTemplate.app
