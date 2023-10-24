# Compiler
CC = g++

# Compilation flags
CFLAGS = -std=c++17 -Wall -fPIC -DRTAPI

# Paths to search for header files
INCLUDES = -Iinclude \
	       -I/usr/include/linuxcnc \
		   -I/usr/local/include \
		   -I/opt/linuxcnc/src/hal/ \
           -I/opt/linuxcnc/src/rtapi/ \
           -I/opt/linuxcnc/src \
		   -I/home/armada/Downloads/linuxcnc-dev/src/hal 
           #-I/usr/local/include/kdl 
           #-I/usr/include/eigen3 

# Link flags
LDFLAGS = -shared 

# Libraries for linking
#LIBS = -lyaml-cpp #-llinuxcnchal 
LIBS =  -llinuxcnchal -lopen62541


# Source files
SRCS = main.cpp \
       OPCUAServer.cpp 
    

# Output file
OUTPUT = /usr/lib/linuxcnc/modules/opcuaserver.so

# Rule to compile all source files
all:
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) $(LDFLAGS) $(LIBS) -o $(OUTPUT)

# Rule for cleaning
clean:
	rm -f $(OUTPUT)