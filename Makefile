configure: Makefile CMakeLists.txt
	cmake -S . -B ./build

build: configure
	cmake --build ./build

run-just_log: build
	./blola.py collect -s examples ./build/CMakeFiles/blola_example_just_log.dir/examples/just_log/main.cpp.ii
	./build/blola_example_just_log | ./blola.py log

