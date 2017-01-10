# Define a macro for the object files 
OBJECTS= SBgui.o RGBpixmap.o 

# Define a macro for the library file 
LIBES= -framework GLUT -framework OpenGL

# use macros rewrite makefile 
prog: $(OBJECTS) 
	c++ $(OBJECTS) $(LIBES) -o prog 

SBgui.o: SBgui.cpp
	g++ -std=c++11 -c SBgui.cpp
