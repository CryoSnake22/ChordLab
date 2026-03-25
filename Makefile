.PHONY: build
PROJECT_NAME = "Chordy"
# build:
# 	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
build: 
	@cmake -B cmake-build 
	@cmake --build cmake-build
run:
	@cmake --build cmake-build
	@open ./cmake-build/$(PROJECT_NAME)_artefacts/Standalone/$(PROJECT_NAME).app
