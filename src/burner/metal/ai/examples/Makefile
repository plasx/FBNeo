CC = clang++
CFLAGS = -std=c++17 -O2 -Wall -Wextra
LDFLAGS = -framework CoreML -framework Vision -framework Foundation -framework Metal

# Define paths
SRC_DIR = .
AI_DIR = ..
INCLUDE_DIRS = -I. -I.. -I../.. -I../../.. -I../../../..

# The example source file
SRC = $(SRC_DIR)/simple_ai_test.cpp

# Target executable
TARGET = simple_ai_test

# Default target
all: $(TARGET)

# Build the example executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@ $(LDFLAGS)

# Create test models for the example
create_test_models:
	@mkdir -p models
	@echo "Creating test models..."
	@if command -v python3 > /dev/null; then \
		cd ../../..; \
		make -f makefile.metal convert_models; \
	else \
		echo "Python 3 not found. Cannot create test models."; \
	fi

# Run the example
run: $(TARGET) create_test_models
	./$(TARGET)

# Run with a specific model
run_with_model: $(TARGET)
	@if [ -z "$(MODEL)" ]; then \
		echo "Please specify a model with MODEL=path/to/model.mlmodel"; \
	else \
		./$(TARGET) $(MODEL); \
	fi

# Clean up
clean:
	rm -f $(TARGET)
	rm -rf models/*.mlmodel

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build the example (default)"
	@echo "  create_test_models - Create test models using PyTorch to CoreML conversion"
	@echo "  run              - Build and run the example with default model"
	@echo "  run_with_model   - Run with a specific model (MODEL=path/to/model.mlmodel)"
	@echo "  clean            - Remove build artifacts"

.PHONY: all create_test_models run run_with_model clean help 