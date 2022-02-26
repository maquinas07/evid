PREFIX=/usr/local/

INCLUDE_DIRS_NOTIFY=/usr/include/glib-2.0 /usr/lib/glib-2.0/include /usr/include/gdk-pixbuf-2.0
CFLAGS_NOTIFY=-DHAVE_NOTIFY $(INCLUDE_DIRS_NOTIFY:%=-I%)
LDFLAGS_NOTIFY=-lnotify

# Uncomment if you want to be able to select the output folder via zenity
# CFLAGS_ZENITY=-DHAVE_ZENITY

CFLAGS_ALL=-Wall -Wpedantic $(CFLAGS_NOTIFY) $(CFLAGS_ZENITY)
LDFLAGS_ALL=-lX11 $(LDFLAGS_NOTIFY)

SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin

SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

BIN=$(BIN_DIR)/evid

.PHONY: all clean cat
all: $(BIN)

$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS_ALL) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS_ALL) -c $^ -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR) && rm -rf $(OBJ_DIR)

install: all
	mkdir -p $(PREFIX)$(BIN_DIR)
	cp -f $(BIN) $(PREFIX)$(BIN)
	chmod 755 $(PREFIX)$(BIN)

uninstall:
	rm -f $(PREFIX)$(BIN)
