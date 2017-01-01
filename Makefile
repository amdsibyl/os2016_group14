# Define a macro for the object files 
OBJECTS= SB13.o RGBpixmap.o 

# Define a macro for the library file 
LIBES= -framework GLUT -framework OpenGL

# use macros rewrite makefile 
prog: $(OBJECTS) 
	c++ $(OBJECTS) $(LIBES) -o prog 
