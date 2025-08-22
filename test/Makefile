CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
TARGET = test_count_player_sessions
SOURCE = test_simple.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean


