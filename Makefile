CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude -Itests
LDFLAGS ?=
LDLIBS ?=

ifeq ($(OS),Windows_NT)
LDFLAGS += -static
endif

BUILD_DIR := build

CORE_SOURCES := \
	src/money.cpp \
	src/account.cpp \
	src/transaction.cpp \
	src/auth.cpp \
	src/storage.cpp \
	src/bank.cpp \
	src/ui.cpp \
	src/cli.cpp

APP_SOURCES := src/main.cpp

TEST_SOURCES := \
	tests/test_main.cpp \
	tests/test_money.cpp \
	tests/test_account.cpp \
	tests/test_auth.cpp \
	tests/test_bank.cpp \
	tests/test_storage.cpp \
	tests/test_cli.cpp

CORE_OBJECTS := $(CORE_SOURCES:src/%.cpp=$(BUILD_DIR)/%.o)
APP_OBJECTS := $(APP_SOURCES:src/%.cpp=$(BUILD_DIR)/%.o)
TEST_OBJECTS := $(TEST_SOURCES:tests/%.cpp=$(BUILD_DIR)/tests/%.o)

DEPS := $(CORE_OBJECTS:.o=.d) $(APP_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

.PHONY: all test run clean

all: $(BUILD_DIR)/atm

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/atm: $(CORE_OBJECTS) $(APP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/atm_tests: $(CORE_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

test: $(BUILD_DIR)/atm_tests
	$(BUILD_DIR)/atm_tests

run: $(BUILD_DIR)/atm
	$(BUILD_DIR)/atm --seed-demo

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
