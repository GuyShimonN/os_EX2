# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++11

# Executable names
TARGET1 = mync
TARGET2 = ttt

# Source files
SRCS1 = mync.cpp
SRCS2 = ttt.cpp

# Object files
OBJS1 = $(SRCS1:.cpp=.o)
OBJS2 = $(SRCS2:.cpp=.o)

# Default target
all: $(TARGET1) $(TARGET2)

# Linking the executables
$(TARGET1): $(OBJS1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(OBJS2)

# Compiling source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS1) $(OBJS2) $(TARGET1) $(TARGET2)

.PHONY: all clean