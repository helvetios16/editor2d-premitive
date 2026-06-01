UNAME := $(shell uname)

SRCDIR  := src
INCDIR  := include
BUILDDIR := build
BIN     := $(BUILDDIR)/main

SRCS := $(wildcard $(SRCDIR)/*.cc)

ifeq ($(UNAME), Darwin)
	CXX      = g++
	CXXFLAGS = -std=c++11 -I$(INCDIR) -I/opt/homebrew/include -Wall -Wextra
	LDFLAGS  = -L/opt/homebrew/lib
	LIBS     = -framework OpenGL -framework GLUT -lglfw
else
	CXX      = g++
	CXXFLAGS = -std=c++11 -I$(INCDIR) -Wall -Wextra
	LDFLAGS  =
	LIBS     = -lGL -lGLU -lglfw -lglut -lm -lpthread -ldl
endif

all: $(BIN)
	./$(BIN)

$(BIN): $(SRCS)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(BIN) $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BUILDDIR)
