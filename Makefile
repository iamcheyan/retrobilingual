.PHONY: all test sync build deploy

all: test

test:
	@mkdir -p build
	python3 tests/test_host.py
	python3 tests/test_pack.py
	gcc -std=c11 -O1 -Wall -I tests/host -I core core/adapters/fe6.c core/adapters/registry.c tests/host/test_fe6_stubs.c tests/host/test_fe6.c -o build/test-fe6
	./build/test-fe6
	gcc -std=c11 -O1 -Wall -I tests/host -I core core/adapters/fe7.c core/adapters/fe8.c core/adapters/registry.c tests/host/test_fe78_stubs.c tests/host/test_fe7_fe8.c -o build/test-fe78
	./build/test-fe78

sync:
	@if [ -d "../PJOS-LINUX" ]; then \
		cp core/retrobilingual.c ../PJOS-LINUX/src/retrobilingual.c; \
		cp core/retrobilingual.h ../PJOS-LINUX/src/retrobilingual.h; \
		rm -rf ../PJOS-LINUX/src/adapters; \
		cp -r core/adapters ../PJOS-LINUX/src/adapters; \
		cp tests/host/test_adapter.c ../PJOS-LINUX/tests/host/test_adapter.c; \
		cp tests/host/host_stubs.h ../PJOS-LINUX/tests/host/host_stubs.h; \
		cp tools/pack.py ../PJOS-LINUX/tools/retrobilingual/pack.py; \
		echo "Synced core & tools to ../PJOS-LINUX"; \
	fi

build: sync
	@if [ -d "../PJOS-LINUX" ]; then \
		cd ../PJOS-LINUX && sh tools/build-retrobilingual-mgba-h700.sh; \
	else \
		echo "Error: ../PJOS-LINUX not found"; exit 1; \
	fi

deploy: build
	@if [ -d "../PJOS-LINUX" ]; then \
		cd ../PJOS-LINUX && sh tools/deploy-retrobilingual.sh; \
	else \
		echo "Error: ../PJOS-LINUX not found"; exit 1; \
	fi
