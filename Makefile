# Define a macro for the object files 
OBJECTS= SB16.o RGBpixmap.o 

# Define a macro for the library file 
LIBES= -framework GLUT -framework OpenGL

# use macros rewrite makefile 
prog: $(OBJECTS) 
	c++ $(OBJECTS) $(LIBES) -o prog 

SB16.o: SB16.cpp
	g++ -std=c++11 -c SB16.cpp
