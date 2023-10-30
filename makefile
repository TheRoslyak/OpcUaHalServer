# Compiler
CC = g++

# Compilation flags
CFLAGS = -std=c++17 -Wall -fPIC -DULAPI

# Paths to search for header files
INCLUDES = -I/usr/include/linuxcnc \
          -I/home/armada/Downloads/linuxcnc-dev/src/hal

           

# Link flags
LDFLAGS = 

# Libraries for linking
LIBS =  -llinuxcnchal -lopen62541

# Source files
SRCS = main.cpp

# Output file
OUTPUT = /usr/bin/opcuaserver

# Rule to compile all source files
all:
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) $(LDFLAGS) $(LIBS) -o $(OUTPUT)

# Rule for cleaning
clean:
	rm -f $(OUTPUT)
