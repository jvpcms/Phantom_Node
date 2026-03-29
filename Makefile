FQBN    = esp32:esp32:esp32
PORT   ?= /dev/ttyUSB0
SKETCH  = src/main
BUILD   = build
SRC     = src

help:
	@echo "Makefile commands:"
	@echo "  run            - compile and run the desktop C++ executable"
	@echo "  build          - compile the sketch for ESP32"
	@echo "  upload         - compile and upload to the board"
	@echo "  monitor        - open live serial monitor (exit with Ctrl+C)"
	@echo "  flash          - compile, upload and open serial monitor"
	@echo "  docs           - generate docs.md from source headers"
	@echo ""
	@echo "Override the serial port (default: /dev/ttyUSB0):"
	@echo "  make flash PORT=/dev/ttyACM0"

run:
	mkdir -p $(BUILD) && \
	g++ -std=c++17 -I$(SRC) $(SKETCH)/main.cpp -o $(BUILD)/phantom_node && \
	./$(BUILD)/phantom_node

build:
	arduino-cli compile --fqbn $(FQBN) \
		--build-property "compiler.cpp.extra_flags=-I$(CURDIR)/$(SRC)" \
		--output-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/

upload: build
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) \
		--upload-field upload.speed=57600 \
		--input-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/

monitor:
	arduino-cli monitor -p $(PORT) --config baudrate=115200

flash: build
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) \
		--upload-field upload.speed=57600 \
		--input-dir $(CURDIR)/$(BUILD) \
		$(SKETCH)/ && \
	arduino-cli monitor -p $(PORT) --config baudrate=115200

docs:
	mkdir -p $(BUILD) && g++ -std=c++17 build_docs.cpp -o $(BUILD)/build_docs && ./$(BUILD)/build_docs

.PHONY: run build upload monitor flash docs help
