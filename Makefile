CFLAGS ?= -O2
CFLAGS += -Wall -pedantic

BIN = hp-smtpd

.PHONY: all
all: $(BIN)

.PHONY: clean
clean:
	$(RM) $(BIN)
