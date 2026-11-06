cmake_minimum_required(VERSION 3.16)

set(ZENOHPICO_DIR "${CMAKE_CURRENT_LIST_DIR}/../../zenoh-pico")

if(NOT EXISTS "${ZENOHPICO_DIR}/CMakeLists.txt")
	message(FATAL_ERROR
		"zenoh-pico is missing at ${ZENOHPICO_DIR}. Run at repo root: git clone --branch 1.10.2 https://github.com/eclipse-zenoh/zenoh-pico.git")
endif()

set(ZENOHPICO_IDF_REQUIRES
	esp_driver_uart
	esp_hw_support
	freertos
	lwip
)

# ESP-IDF evaluates each component twice so it can first collect all component
# dependencies, resolve the build graph, and then create targets in dependency
# order. The first pass uses CMake script mode, where add_subdirectory() is
# unavailable.
if(CMAKE_SCRIPT_MODE_FILE)
	# This records dependencies only; it does not create the component target.
	idf_component_register(REQUIRES ${ZENOHPICO_IDF_REQUIRES})
	return()
endif()

# Build static zenoh-pico with the ESP-IDF toolchain. zenoh-pico configures its
# own feature defaults and generated headers instead of duplicating them here.

# zenoh-pico initializes ZP_PLATFORM in the CMake cache. A normal variable can
# be discarded while that entry is created, so store the ESP-IDF profile in the CMake cache.
set(ZP_PLATFORM espidf CACHE STRING "Built-in or external platform profile" FORCE)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(
	"${ZENOHPICO_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}/zenoh-pico"
)

# This creates the component target during ESP-IDF's normal configure phase.
idf_component_register(REQUIRES ${ZENOHPICO_IDF_REQUIRES})

foreach(require IN LISTS ZENOHPICO_IDF_REQUIRES)
	target_link_libraries(zenohpico_static PRIVATE idf::${require})
endforeach()
target_link_libraries(${COMPONENT_LIB} INTERFACE zenohpico_static)
