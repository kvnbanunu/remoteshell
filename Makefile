build: clean format
	@mkdir -p build
	@clang src/server.c src/setup.c src/remoteshell.c -o build/server
	@clang src/client.c src/setup.c -o build/client

build-server:
	@mkdir -p build
	@clang src/server.c src/setup.c src/remoteshell.c -o build/server

build-client:
	@mkdir -p build
	@clang src/client.c src/setup.c -o build/client

debug: format
	@mkdir -p debug/
	@clang -Wall -Wextra -Wpedantic -O2 -Wconversion src/server.c src/setup.c src/remoteshell.c -o debug/server
	@clang -Wall -Wextra -Wpedantic -O2 -Wconversion src/client.c src/setup.c -o debug/client

debug-server:
	@mkdir -p debug/
	@clang -Wall -Wextra -Wpedantic -O2 -Wconversion src/server.c src/setup.c src/remoteshell.c -o debug/server

debug-client:
	@mkdir -p debug/
	@clang -Wall -Wextra -Wpedantic -O2 -Wconversion src/client.c src/setup.c -o debug/client

clean:
	@rm -rf build/
	@rm -rf debug/

format:
	@clang-format -i -style=file include/*.h src/*.c

server:
	@./build/server

client:
	@./build/client $(ARGS)
