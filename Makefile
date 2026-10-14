CC ?= cc
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -Iinclude
CFLAGS ?= -O2 -g -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?=

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)
TARGET := bin/myls

.PHONY: all clean test help
all: $(TARGET)

$(TARGET): $(OBJECTS) | bin
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build bin:
	mkdir -p $@

test: $(TARGET)
	bash tests/test_ls.sh ./$(TARGET)

clean:
	rm -rf build bin

help:
	@echo "make         Compile bin/myls"
	@echo "make test    Build and run tests"
	@echo "make clean   Remove object files and executable"

-include $(DEPS)
