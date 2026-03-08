ENV      = t-beam-supreme
PORT    ?= $(shell ls /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART 2>/dev/null | head -1)
BAUD     = 115200

.DEFAULT_GOAL := help

# ─── Build ─────────────────────────────────────────────────────────────────────

.PHONY: build
build:
	pio run -e $(ENV)

.PHONY: clean
clean:
	pio run -e $(ENV) --target clean

# ─── Flash ─────────────────────────────────────────────────────────────────────

.PHONY: flash
flash:
	pio run -e $(ENV) --target upload

.PHONY: flash-port
flash-port:
	@if [ -z "$(PORT)" ]; then echo "No serial port found. Use: make flash-port PORT=/dev/cu.xxx"; exit 1; fi
	pio run -e $(ENV) --target upload --upload-port $(PORT)

# ─── Monitor ───────────────────────────────────────────────────────────────────

.PHONY: monitor
monitor:
	pio device monitor --baud $(BAUD)

.PHONY: monitor-port
monitor-port:
	pio device monitor --port $(PORT) --baud $(BAUD)

# ─── Build + Flash + Monitor ───────────────────────────────────────────────────

.PHONY: run
run: build flash monitor

# ─── Filesystem (SPIFFS) ───────────────────────────────────────────────────────

.PHONY: fs-build
fs-build:
	pio run -e $(ENV) --target buildfs

.PHONY: fs-flash
fs-flash:
	pio run -e $(ENV) --target uploadfs

# ─── Ports ─────────────────────────────────────────────────────────────────────

.PHONY: ports
ports:
	pio device list

# ─── Tests ─────────────────────────────────────────────────────────────────────

.PHONY: test
test: test-native

.PHONY: test-native
test-native:
	pio test -e native

.PHONY: test-embedded
test-embedded:
	pio test -e embedded-test

.PHONY: test-verbose
test-verbose:
	pio test -e native -v

# ─── Help ──────────────────────────────────────────────────────────────────────

.PHONY: help
help:
	@echo ""
	@echo "  Draille — ESP32-S3 firmware"
	@echo ""
	@echo "  make build         Compile the firmware"
	@echo "  make flash         Compile + flash (auto-detect port)"
	@echo "  make flash-port    Compile + flash (PORT=/dev/cu.xxx)"
	@echo "  make monitor       Open serial monitor ($(BAUD) baud)"
	@echo "  make run           build + flash + monitor"
	@echo "  make clean         Clean build artifacts"
	@echo "  make fs-build      Build SPIFFS filesystem image"
	@echo "  make fs-flash      Flash SPIFFS filesystem to device"
	@echo "  make ports         List available serial ports"
	@echo ""
	@echo "  make test          Run native unit tests (no hardware)"
	@echo "  make test-embedded Run integration tests on device"
	@echo "  make test-verbose  Native tests with verbose output"
	@echo ""
