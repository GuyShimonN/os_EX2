# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++11

# Name of the output executables
TARGET1 = mync
TARGET2 = ttt

# Source files
SRCS1 = mynetcat.cpp
SRCS2 = ttt.cpp

# Object files
OBJS1 = $(SRCS1:.cpp=.o)
OBJS2 = $(SRCS2:.cpp=.o)

# Rule to link the programs
all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(OBJS2)

# Rule to compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to clean intermediate files
.PHONY: clean
clean:
	rm -f $(OBJS1) $(OBJS2) $(TARGET1) $(TARGET2)