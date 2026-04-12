FQBN   = adafruit:nrf52:feather52840
SKETCH = src/main
BUILD  = build/nrf52840
SRC    = src
PORT1 ?= /dev/ttyACM1   # emitter
PORT2 ?= /dev/ttyACM0   # receiver

export PATH := $(CURDIR)/.venv/bin:$(PATH)

define uf2_flash
	python3 uf2conv.py $(CURDIR)/$(BUILD)/main.ino.hex \
		--convert --family 0xADA52840 \
		--output $(CURDIR)/$(BUILD)/main.ino.uf2
	@UF2_DRIVE=$$(find /media/$(USER) /run/media/$(USER) -maxdepth 2 -name "INFO_UF2.TXT" 2>/dev/null | head -1 | xargs -I{} dirname {}); \
	if [ -z "$$UF2_DRIVE" ]; then \
		echo "No bootloader drive found. Double-tap reset then re-run."; exit 1; \
	fi; \
	cp $(CURDIR)/$(BUILD)/main.ino.uf2 "$$UF2_DRIVE/" && \
	echo "Flashed to $$UF2_DRIVE — waiting for reboot..."; \
	sleep 3
endef

build-emit:
	arduino-cli compile --fqbn $(FQBN) \
		--build-property "compiler.cpp.extra_flags=-I$(CURDIR)/$(SRC) -DEMITTER=1" \
		--output-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/

build-recv:
	arduino-cli compile --fqbn $(FQBN) \
		--build-property "compiler.cpp.extra_flags=-I$(CURDIR)/$(SRC) -DEMITTER=0" \
		--output-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/

flash-emit: build-emit
	$(call uf2_flash)
	@python3 monitor.py $(PORT1) 115200

flash-recv: build-recv
	$(call uf2_flash)
	@python3 monitor.py $(PORT2) 115200

monitor-emit:
	@python3 monitor.py $(PORT1) 115200

monitor-recv:
	@python3 monitor.py $(PORT2) 115200

run:
	mkdir -p build/desktop && \
	g++ -std=c++17 -I$(SRC) src/desktop/main.cpp -o build/desktop/phantom_node && \
	./build/desktop/phantom_node

docs:
	mkdir -p build/desktop && \
	g++ -std=c++17 build_docs.cpp -o build/desktop/build_docs && \
	./build/desktop/build_docs

setup-nrf:
	arduino-cli config add board_manager.additional_urls \
		https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
	arduino-cli core update-index
	arduino-cli core install adafruit:nrf52
	@echo "Fetching uf2conv tooling (not committed to repo)..."
	curl -fsSL https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2conv.py -o uf2conv.py
	curl -fsSL https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2families.json -o uf2families.json
	@echo "Install Python dependencies:"
	@echo "  python3 -m venv .venv && .venv/bin/pip install -r requirements.txt"

.PHONY: build-emit build-recv flash-emit flash-recv monitor-emit monitor-recv run docs setup-nrf
