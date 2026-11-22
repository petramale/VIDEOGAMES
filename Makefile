# Compila lo Studio Videogiochi con GCC (MinGW su Windows, come quello di Dev-C++).
#
#   Windows (prompt dei comandi, con il bin di TDM-GCC nel PATH):  mingw32-make
#   Linux:                                                          make
#   Da Linux verso Windows:  make PIATTAFORMA=windows CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
#
# In alternativa: apri StudioVideogiochi.dev con Embarcadero Dev-C++ e premi F11.

ifeq ($(OS),Windows_NT)
  PIATTAFORMA ?= windows
else
  PIATTAFORMA ?= linux
endif

CC  ?= gcc
CXX ?= g++
RAYLIB   := librerie/raylib/src
OBJ_DIR  := build/$(PIATTAFORMA)
BIN_DIR  := bin

CFLAGS   := -O2 -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -I$(RAYLIB) -I$(RAYLIB)/external/glfw/include -w
CXXFLAGS := -O2 -std=c++17 -Wall -I$(RAYLIB) -Ilibrerie/nlohmann

ifeq ($(PIATTAFORMA),windows)
  ESEGUIBILE := $(BIN_DIR)/StudioVideogiochi.exe
  LDFLAGS    := -static -mwindows -lopengl32 -lgdi32 -lwinmm -lwinhttp
  WINDRES    ?= $(patsubst %g++,%windres,$(CXX))
  RISORSE    := $(OBJ_DIR)/studio_rc.o
else
  ESEGUIBILE := $(BIN_DIR)/StudioVideogiochi
  CFLAGS     += -D_GNU_SOURCE -D_GLFW_X11
  LDFLAGS    := -lGL -lm -lpthread -ldl -lrt -lX11
endif

RAYLIB_SRC := rcore rshapes rtextures rtext rmodels raudio utils rglfw
RAYLIB_OBJ := $(addprefix $(OBJ_DIR)/raylib/,$(addsuffix .o,$(RAYLIB_SRC)))
STUDIO_SRC := $(wildcard src/*.cpp)
STUDIO_OBJ := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(STUDIO_SRC))

all: $(ESEGUIBILE)

$(ESEGUIBILE): $(RAYLIB_OBJ) $(STUDIO_OBJ) $(RISORSE)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/raylib/%.o: $(RAYLIB)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.cpp src/*.h
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Icona dell'eseguibile (logo Elvantis), solo su Windows.
$(OBJ_DIR)/studio_rc.o: risorse/studio.rc risorse/elvantis.ico
	@mkdir -p $(dir $@)
	$(WINDRES) -I risorse -i $< -o $@

clean:
	rm -rf build bin

.PHONY: all clean
