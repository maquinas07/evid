PREFIX=/usr/local/

# Comment if you want to disable X extensions
CFLAGS_XEXTENSIONS=-DHAVE_XEXTENSIONS `pkg-config --cflags xfixes`
LDFLAGS_XEXTENSIONS=`pkg-config --libs xfixes`

# Comment if you want to disable the dependency to libnotify. Errors will be printed to stderr.
CFLAGS_NOTIFY:=-DHAVE_NOTIFY `pkg-config --cflags libnotify gio-2.0`
LDFLAGS_NOTIFY:=`pkg-config --libs libnotify gio-2.0`

# Uncomment if you want to be able to select the output folder via zenity
# CFLAGS_ZENITY=-DHAVE_ZENITY

CFLAGS_ALL:=-Wall -Wpedantic -O2 $(CFLAGS_NOTIFY) $(CFLAGS_ZENITY) $(CFLAGS_XEXTENSIONS)
LDFLAGS_ALL:=-lX11 $(LDFLAGS_NOTIFY) $(LDFLAGS_XEXTENSIONS)

SRC_DIR:=src
OBJ_DIR:=obj
BIN_DIR:=bin

SRC:=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

BIN=$(BIN_DIR)/evid

.PHONY: all clean cat
all: $(BIN)

$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $^ $(LDFLAGS_ALL) -o $@

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
