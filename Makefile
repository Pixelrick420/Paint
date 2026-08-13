CXX      := g++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS   := $(shell pkg-config --libs sdl2)

TARGET := paint
SRC    := paint.cpp
OBJ    := $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
