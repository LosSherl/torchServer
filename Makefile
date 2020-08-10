all:
	mkdir -p bin
	cd build && make

client:
	mkdir -p bin
	cd build && make client
