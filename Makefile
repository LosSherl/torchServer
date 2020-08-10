all:
	mkdir -p bin
	cd build && make

client:
	mkdir -p bin
	cd build && make client

clean:
	rm -rf ./bin/server ./bin/client 