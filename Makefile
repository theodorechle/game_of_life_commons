CPP_C=g++
CPP_FLAGS=-std=c++23 -Wall -g -MMD -MP
SDL_CMD=`pkg-config sdl3 sdl3-ttf --cflags --libs`
BIN_DIR=bin
OBJ_DIR=obj
SRC_DIR=src
TESTS_DIR=tests
TESTS_LIB=cpp_tests/bin/cpp_tests_lib
LIB=bin/game_of_life_commons_lib

# Subdirectories
SUBDIRS=network_input_handler

# Source files
SRC_SUBDIRS=$(foreach dir, $(SUBDIRS), $(wildcard $(SRC_DIR)/$(dir)/*.cpp))
SRC_TESTS=$(wildcard $(TESTS_DIR)/*.cpp) $(wildcard $(TESTS_DIR)/*/*.cpp)

# Object files
OBJ_MAIN=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_MAIN))
OBJ_SUBDIRS=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_SUBDIRS))
OBJ_TESTS=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_TEST_DIR)/%.o, $(SRC_TESTS))

# Executable targets
MAIN=$(BIN_DIR)/game_of_life_client
TESTS=$(BIN_DIR)/tests

.PHONY: clean tests lib

ifeq ($(DEBUG),1)
CPP_FLAGS += -DDEBUG
endif

lib: $(LIB).a

tests: $(TESTS)

lib: $(LIB).a

$(LIB).a: $(OBJ_MAIN) $(OBJ_SUBDIRS)
	@mkdir -p $(BIN_DIR)
	ar -r $@ $^

# Build the tests executable (tests + lib)
$(TESTS): $(OBJ_TESTS) $(LIB).a $(TESTS_LIB).a
	@mkdir -p $(BIN_DIR)
	$(CPP_C) $(CPP_FLAGS) -o $@ $^

# Rule for compiling all object files
$(OBJ_TEST_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CPP_C) $(CPP_FLAGS) -DDEBUG -c $< -o $@

$(TESTS_LIB).a:
	$(MAKE) -C cpp_tests -j lib DEBUG=$(DEBUG)

# Rule for compiling all object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CPP_C) $(CPP_FLAGS) -c $< -o $@

# Clean all generated files
clean:
	@find obj -mindepth 1 ! -name .gitkeep -delete
	@find bin -mindepth 1 ! -name .gitkeep -delete
	$(MAKE) -C cpp_tests clean
