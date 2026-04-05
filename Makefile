# ── Board selection ─────────────────────────────────────────────────────────
# make flash                    → ESP32 (default)
# make flash BOARD=nrf52840     → Adafruit Feather nRF52840
BOARD  ?= esp32

ifeq ($(BOARD),esp32)
  FQBN         = esp32:esp32:esp32
  UPLOAD_FLAGS = --upload-field upload.speed=57600
  PORT        ?= /dev/ttyUSB0
else ifeq ($(BOARD),nrf52840)
  FQBN         = adafruit:nrf52:feather52840
  UPLOAD_FLAGS =
  PORT        ?= /dev/ttyACM0
else
  $(error Unknown BOARD "$(BOARD)". Valid values: esp32, nrf52840)
endif

SKETCH = src/main
BUILD  = build/$(BOARD)
SRC    = src

help:
	@echo "Makefile commands:"
	@echo "  run             - compile and run the desktop C++ executable"
	@echo "  build           - compile the sketch        (default: BOARD=esp32)"
	@echo "  upload          - compile and upload to the board"
	@echo "  monitor         - open live serial monitor (exit with Ctrl+C)"
	@echo "  flash           - compile, upload and open serial monitor"
	@echo "  docs            - generate docs.md from source headers"
	@echo "  setup-esp32     - install the ESP32 Arduino core"
	@echo "  setup-nrf       - install the Adafruit nRF52 Arduino core"
	@echo "  uf2             - copy built .uf2 to mounted nRF52840 bootloader drive"
	@echo ""
	@echo "Board selection (default: esp32):"
	@echo "  make flash BOARD=nrf52840"
	@echo "  make uf2   BOARD=nrf52840   (double-tap reset first)"
	@echo ""
	@echo "Override the serial port:"
	@echo "  make flash PORT=/dev/ttyACM0 BOARD=nrf52840"

run:
	mkdir -p build/desktop && \
	g++ -std=c++17 -I$(SRC) src/desktop/main.cpp -o build/desktop/phantom_node && \
	./build/desktop/phantom_node

build:
	arduino-cli compile --fqbn $(FQBN) \
		--build-property "compiler.cpp.extra_flags=-I$(CURDIR)/$(SRC)" \
		--output-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/

upload: build
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) \
		$(UPLOAD_FLAGS) \
		--input-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/

monitor:
	@echo "--- Monitoring $(PORT) --- Press Ctrl+C to exit ---"
ifeq ($(BOARD),nrf52840)
	@python3 monitor.py $(PORT) 115200
else
	@while true; do \
		arduino-cli monitor -p $(PORT) --config baudrate=115200,dtr=off,rts=off 2>/dev/null; \
		sleep 0.2; \
	done
endif

flash: build
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) \
		$(UPLOAD_FLAGS) \
		--input-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/ && \
	arduino-cli monitor -p $(PORT) --config baudrate=115200

uf2: build
	@python3 uf2conv.py $(CURDIR)/$(BUILD)/main.ino.hex \
		--convert --family 0xADA52840 \
		--output $(CURDIR)/$(BUILD)/main.ino.uf2
	@UF2_DRIVE=$$(find /media/$(USER) /run/media/$(USER) -maxdepth 2 -name "INFO_UF2.TXT" 2>/dev/null | head -1 | xargs -I{} dirname {}); \
	if [ -z "$$UF2_DRIVE" ]; then \
		echo "UF2 written to $(BUILD)/main.ino.uf2"; \
		echo "Double-tap reset on the nRF52840 then re-run to flash, or copy manually."; exit 0; \
	fi; \
	cp $(CURDIR)/$(BUILD)/main.ino.uf2 "$$UF2_DRIVE/" && \
	echo "Flashed to $$UF2_DRIVE"

docs:
	mkdir -p build/desktop && \
	g++ -std=c++17 build_docs.cpp -o build/desktop/build_docs && \
	./build/desktop/build_docs

setup-esp32:
	arduino-cli config add board_manager.additional_urls \
		https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
	arduino-cli core update-index
	arduino-cli core install esp32:esp32

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

.PHONY: run build upload monitor flash uf2 docs help setup-esp32 setup-nrf
