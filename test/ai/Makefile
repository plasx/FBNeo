CXX = g++
CXXFLAGS = -std=c++17 -Wall -I../../src -I../../3rdparty/nlohmann
LDFLAGS = -lstdc++ -lm

# List of test files
TEST_SOURCES = test_ai_input_frame.cpp \
               test_ai_output_action.cpp \
               test_ai_memory_mapping.cpp \
               test_neural_ai_controller.cpp \
               test_ai_torch_policy.cpp

# Generate test binary targets from sources
TEST_BINS = $(TEST_SOURCES:.cpp=)

# Default target builds all tests
all: $(TEST_BINS)

# Rule to build test binaries
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# Run all tests
run: $(TEST_BINS)
	@echo "Running AI unit tests..."
	@for test in $(TEST_BINS); do \
		echo "\n=== Running $$test ==="; \
		./$$test; \
	done

# Clean up
clean:
	rm -f $(TEST_BINS)

.PHONY: all run clean 