# Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
TARGET = data_processor
SOURCES = data_processor.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)
	@rm -f $(OBJECTS)  # Auto-remove object files after linking

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)