ZENOH_PICO_DIR = ../zenoh-pico
ZENOH_PICO_BUILD_DIR = $(ZENOH_PICO_DIR)/build
ZENOH_PICO_LIB = $(ZENOH_PICO_BUILD_DIR)/lib/libzenohpico.a
ZENOH_PICO_DEFS ?= -DZENOH_LINUX -DZENOH_COMPILER_GCC
ZENOH_PICO_CFLAGS = -I$(ZENOH_PICO_DIR)/include -I$(ZENOH_PICO_BUILD_DIR)/include
ZENOH_PICO_LDFLAGS = -lpthread

CC ?= cc
CFLAGS ?= -O2 -fPIC -Wall -Wextra -Werror \
	-Wconversion -Wsign-conversion -Wformat=2 -Wshadow -Wstrict-prototypes \
	-D_FORTIFY_SOURCE=2 -fstack-protector-strong
LDFLAGS ?= -shared

ERL_CFLAGS  ?= -I$(ERTS_INCLUDE_DIR) -I$(ERL_EI_INCLUDE_DIR)
ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR) -lei

PRIV_DIR = $(MIX_APP_PATH)/priv
NIF_SO = $(PRIV_DIR)/zenohex_pico.so

SRC_DIR = .
SRC_C ?= $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/helper/*.c)

all: $(NIF_SO)

$(ZENOH_PICO_LIB):
	cmake -S $(ZENOH_PICO_DIR) -B $(ZENOH_PICO_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
		-DBUILD_TESTING=OFF \
		-DBUILD_EXAMPLES=OFF \
		-DBUILD_INTEGRATION=OFF \
		-DBUILD_TOOLS=OFF
	cmake --build $(ZENOH_PICO_BUILD_DIR) --target zenohpico_static

$(PRIV_DIR):
	mkdir -p $@

$(NIF_SO): $(SRC_C) $(ZENOH_PICO_LIB) | $(PRIV_DIR)
	$(CC) $(CFLAGS) \
		$(ZENOH_PICO_DEFS) \
		$(ZENOH_PICO_CFLAGS) \
		$(ERL_CFLAGS) \
		-o $@ \
		$(SRC_C) \
		$(ZENOH_PICO_LIB) \
		$(LDFLAGS) \
		$(ZENOH_PICO_LDFLAGS) \
		$(ERL_LDFLAGS)

clean:
	rm -f $(NIF_SO)

.PHONY: all clean
