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
	@echo ""
	@echo "Override the serial port (default: /dev/ttyUSB0):"
	@echo "  make flash PORT=/dev/ttyACM0"

run:
	g++ -std=c++17 -I$(SRC) $(SKETCH)/main.cpp -o $(SKETCH)/phantom_node && \
	./$(SKETCH)/phantom_node

build:
	arduino-cli compile --fqbn $(FQBN) \
		--build-property "compiler.cpp.extra_flags=-I$(CURDIR)/$(SRC)" \
		--output-dir $(BUILD) \
		$(SKETCH)/

upload: build
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) \
		--upload-field upload.speed=115200 \
		--input-dir $(BUILD) \
		$(SKETCH)/

monitor:
	arduino-cli monitor -p $(PORT) --config baudrate=115200

flash: build
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) \
		--upload-field upload.speed=115200 \
		--input-dir $(BUILD) \
		$(SKETCH)/ && \
	arduino-cli monitor -p $(PORT) --config baudrate=115200

.PHONY: run build upload monitor flash help
