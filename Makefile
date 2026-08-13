CXX      := g++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra -Isrc
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS   := $(shell pkg-config --libs sdl2)

TARGET := paint
SRC    := paint.cpp src/app/app.cpp src/app/render.cpp \
          src/io/input.cpp src/draw/draw.cpp \
          src/draw/canvas.cpp src/io/bmp.cpp
OBJ    := $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
