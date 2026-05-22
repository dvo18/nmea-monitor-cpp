.PHONY: install build run test clean

PROJECT_ROOT := $(shell pwd)

# ── Install ───────────────────────────────────────────────────────────────────
install:
	sudo apt-get update -qq
	sudo apt-get install -y \
		cmake ninja-build g++ \
		qt6-base-dev \
		qt6-declarative-dev \
		libgl1-mesa-dev
	sudo apt-get install -y 'qml6-module-*'

# ── Build ──────────────────────────────────────────────────────────────────────
build:
	cmake -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DNMEA_BUILD_TESTS=ON
	cmake --build build --parallel
	cp sensors.json build/

# ── Test ──────────────────────────────────────────────────────────────────────
test: build
	ctest --test-dir build --output-on-failure --parallel 4

# ── Run ───────────────────────────────────────────────────────────────────────
run: build
	@./build/simulator/nmea_simulator > /tmp/nmea_sim_out.txt 2>/tmp/nmea_sim_err.txt & \
	SIM_PID=$$!; \
	trap "kill $$SIM_PID 2>/dev/null" EXIT INT TERM; \
	sleep 0.5; \
	PTY_LINE=$$(grep "^pty:" /tmp/nmea_sim_out.txt 2>/dev/null); \
	PTY_PATH=$${PTY_LINE#pty:}; \
	if [ -z "$$PTY_PATH" ]; then \
		echo "[run] error: could not read pty path from simulator"; \
		cat /tmp/nmea_sim_err.txt 2>/dev/null; \
		exit 1; \
	fi; \
	echo "[run] simulator pty: $$PTY_PATH"; \
	python3 -c "import json,sys; f='build/sensors.json'; d=json.load(open(f)); d['connection']['port']=sys.argv[1]; json.dump(d,open(f,'w'),indent=2)" "$$PTY_PATH"; \
	echo "[run] starting monitor on $$PTY_PATH..."; \
	NMEA_CONFIG_DIR="$(PROJECT_ROOT)/build" \
	NMEA_LOG_DIR="$(PROJECT_ROOT)/logs" \
	LIBGL_ALWAYS_SOFTWARE=1 \
	./build/monitor/nmea_monitor; \
	kill $$SIM_PID 2>/dev/null

# ── Clean — never touches logs/ ───────────────────────────────────────────────
clean:
	rm -rf build
	@rm -f /tmp/nmea_sim_out.txt /tmp/nmea_sim_err.txt