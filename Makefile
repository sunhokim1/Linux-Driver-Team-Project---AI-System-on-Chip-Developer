CC ?= gcc
CPPFLAGS += -Iinclude -Iuser
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
BUILD_DIR := build
APP := $(BUILD_DIR)/smart-fan
USER_SOURCES := user/main.c user/event/event_handler.c user/state/fan_state.c user/output/fan_output.c
USER_HEADERS := $(wildcard include/*.h user/*/*.h)
KDIR ?= /lib/modules/$(shell uname -r)/build
TEST_NAMES := test_event test_state test_pwm
TEST_BINS := $(addprefix $(BUILD_DIR)/,$(TEST_NAMES))

.PHONY: all user kernel test test-build clean
all: user

user: $(APP)

$(BUILD_DIR):
	mkdir -p $@

$(APP): $(USER_SOURCES) $(USER_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(USER_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

kernel:
	$(MAKE) -C kernel/ultrasonic KDIR="$(KDIR)"
	$(MAKE) -C kernel/fan_pwm KDIR="$(KDIR)"

test-build: $(TEST_BINS)

$(BUILD_DIR)/test_%: tests/test_%.c $(filter-out user/main.c,$(USER_SOURCES)) $(USER_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(filter-out user/main.c,$(USER_SOURCES)) $(LDFLAGS) $(LDLIBS) -o $@

test: test-build
	@for test in $(TEST_BINS); do \
		"$$test"; status=$$?; \
		if [ $$status -ne 0 ] && [ $$status -ne 77 ]; then exit $$status; fi; \
	done

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C kernel/ultrasonic KDIR="$(KDIR)" clean
	$(MAKE) -C kernel/fan_pwm KDIR="$(KDIR)" clean
