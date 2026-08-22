CXX      := g++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra -Isrc
DEPFLAGS := -MMD -MP
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)
TTF_CFLAGS := $(shell pkg-config --cflags sdl3-ttf 2>/dev/null)
TTF_LIBS   := $(shell pkg-config --libs sdl3-ttf 2>/dev/null)
ifneq ($(TTF_LIBS),)
  SDL_CFLAGS += $(TTF_CFLAGS)
  SDL_LIBS   += $(TTF_LIBS)
endif

TARGET := paint
SRC    := paint.cpp src/app/app.cpp src/app/render.cpp \
          src/io/input.cpp src/draw/draw.cpp \
          src/draw/canvas.cpp src/io/bmp.cpp
OBJ    := $(SRC:.cpp=.o)
DEPS   := $(SRC:.cpp=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ) $(DEPS)

-include $(DEPS)

.PHONY: all clean
