.PHONY: configure build test sample spec

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

build: configure
	cmake --build build -j

test: build
	ctest --test-dir build --output-on-failure

sample: build
	wat2wasm samples/add.wat -o build/add.wasm
	./build/miniwm build/add.wasm add 19 23

spec: build
	python3 tests/run_spec.py third_party/spec/test/core/i32.wast

