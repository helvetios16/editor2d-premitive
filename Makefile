UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	CXXFLAGS = -I/opt/homebrew/include
	LDFLAGS  = -L/opt/homebrew/lib
	LIBS     = -framework OpenGL -framework GLUT -lglfw
else
	CXXFLAGS =
	LDFLAGS  =
	LIBS     = -lGL -lglfw -lglut -lm -lpthread -ldl
endif

all:
	g++ $(CXXFLAGS) main.cc -o main $(LDFLAGS) $(LIBS); ./main

clean:
	rm -f main
