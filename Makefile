PROJECT := audio_light_sync
CXX_LINUX := g++
CXX_WIN := x86_64-w64-mingw32-g++
TARGET_LINUX := $(PROJECT)
TARGET_WIN := $(PROJECT).exe
TEST_TARGET_WIN := unit_tests.exe
SOURCES := $(wildcard src/*.cpp src/*/*.cpp)
HEADERS := $(wildcard include/als/*/*.h)
TEST_SOURCES := tests/unit_tests.cpp \
                src/audio/SampleConverter.cpp \
                src/config/ConfigStore.cpp \
                src/config/RuntimeConfig.cpp \
                src/core/Telemetry.cpp \
                src/effects/ColorUtils.cpp \
                src/effects/EffectCatalog.cpp \
                src/effects/EffectEngine.cpp
INCLUDES := -Iinclude -I.
COMMON_FLAGS := -std=c++17 -O3 -march=native -ffast-math -funroll-loops \
                -Wall -Wextra -Wpedantic -DNDEBUG $(INCLUDES)
CFLAGS_LINUX := $(COMMON_FLAGS) -flto
LDFLAGS_LINUX := -lpipewire-0.3 -lfftw3f -lpthread -lm
CFLAGS_WIN := $(COMMON_FLAGS) \
              -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE
LDFLAGS_WIN := -L. -static-libgcc -static-libstdc++ \
               -lfftw3f -lws2_32 -lole32 -loleaut32 -lwinmm

.PHONY: all linux windows test-windows clean

all: linux

linux: $(TARGET_LINUX)

windows: $(TARGET_WIN)

$(TARGET_LINUX): $(SOURCES) $(HEADERS)
	@echo "Building for Linux..."
	$(CXX_LINUX) $(CFLAGS_LINUX) $(SOURCES) -o $(TARGET_LINUX) $(LDFLAGS_LINUX)
	@echo "Build complete: $(TARGET_LINUX)"

$(TARGET_WIN): $(SOURCES) $(HEADERS)
	@echo "Building for Windows (w64devkit)..."
	$(CXX_WIN) $(CFLAGS_WIN) $(SOURCES) -o $(TARGET_WIN) $(LDFLAGS_WIN)
	@echo "Build complete: $(TARGET_WIN)"
	@echo "NOTE: You need libfftw3f-3.dll in the same folder to run this executable."

test-windows: $(TEST_TARGET_WIN)
	./$(TEST_TARGET_WIN)

$(TEST_TARGET_WIN): $(TEST_SOURCES) $(HEADERS)
	$(CXX_WIN) -std=c++17 -O2 -Wall -Wextra -Wpedantic $(INCLUDES) \
		$(TEST_SOURCES) -o $(TEST_TARGET_WIN) \
		-static-libgcc -static-libstdc++

clean:
	rm -f $(TARGET_LINUX) $(TARGET_WIN) $(TEST_TARGET_WIN) *.o
