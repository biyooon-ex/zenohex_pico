ERLANG_ROOT ?= $(shell erl -noshell -eval 'io:format("~s", [code:root_dir()])' -s init stop)
ELIXIR_NIF_INCLUDE ?= $(ERLANG_ROOT)/usr/include
ZENOH_PICO_DIR ?= ../zenoh-pico
ZENOH_PICO_BUILD_DIR ?= $(ZENOH_PICO_DIR)/build
ZENOH_PICO_LIB ?= $(ZENOH_PICO_BUILD_DIR)/lib/libzenohpico.a
ZENOH_PICO_STAMP ?= $(ZENOH_PICO_BUILD_DIR)/.zenohex_built
CC ?= cc
CFLAGS ?= -O2 -fPIC -Wall -Wextra# -Werror
ZENOH_DEFS ?= -DZENOH_LINUX -DZENOH_COMPILER_GCC
LDFLAGS ?= -shared
TARGET ?= ../priv/zenohex_pico.so
SOURCE ?= zenohex_pico.c

all: $(TARGET)

$(ZENOH_PICO_STAMP):
	cmake -S $(ZENOH_PICO_DIR) -B $(ZENOH_PICO_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
		-DBUILD_TESTING=OFF \
		-DBUILD_EXAMPLES=OFF \
		-DBUILD_INTEGRATION=OFF \
		-DBUILD_TOOLS=OFF \
		-DZENOH_LOG=TRACE
	cmake --build $(ZENOH_PICO_BUILD_DIR) --target zenohpico_static
	touch $(ZENOH_PICO_STAMP)

../priv:
	mkdir -p ../priv

$(TARGET): $(SOURCE) $(ZENOH_PICO_STAMP) $(ZENOH_PICO_LIB) | ../priv
	$(CC) $(CFLAGS) \
		$(ZENOH_DEFS) \
		-I$(ELIXIR_NIF_INCLUDE) \
		-I$(ZENOH_PICO_DIR)/include \
		-o $@ $< \
		$(ZENOH_PICO_LIB) \
		-lpthread \
		$(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -f $(ZENOH_PICO_STAMP)

.PHONY: all clean
