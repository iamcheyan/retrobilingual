.PHONY: all test sync build deploy

all: test

test:
	python3 tests/test_host.py

sync:
	@if [ -d "../PJOS-LINUX" ]; then \
		cp core/retrobilingual.c ../PJOS-LINUX/src/retrobilingual.c; \
		cp core/retrobilingual.h ../PJOS-LINUX/src/retrobilingual.h; \
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
