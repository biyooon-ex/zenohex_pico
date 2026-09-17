cmake_minimum_required(VERSION 3.16)

set(ZENOHPICO_SUBMODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../zenoh-pico")

if(NOT EXISTS "${ZENOHPICO_SUBMODULE_DIR}/CMakeLists.txt")
	message(FATAL_ERROR
		"zenoh-pico submodule is missing at ${ZENOHPICO_SUBMODULE_DIR}. Run at repo root: git submodule update --init --recursive")
endif()

# ESP-IDF first evaluates components in script mode to collect requirements.
# add_subdirectory() is available only during the subsequent configure phase.
if(CMAKE_SCRIPT_MODE_FILE)
	idf_component_register(
		REQUIRES
			esp_driver_uart
			esp_hw_support
			freertos
			lwip
	)
	return()
endif()

# Let zenoh-pico configure its own feature defaults and generated headers with
# the ESP-IDF toolchain instead of duplicating its configuration here.
set(ZP_PLATFORM espidf CACHE STRING "zenoh-pico platform profile" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static zenoh-pico library" FORCE)
add_subdirectory(
	"${ZENOHPICO_SUBMODULE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}/zenoh-pico"
)

idf_component_register(
	REQUIRES
		esp_driver_uart
		esp_hw_support
		freertos
		lwip
)

target_link_libraries(zenohpico_static
	PUBLIC
		idf::esp_driver_uart
		idf::esp_hw_support
		idf::freertos
		idf::lwip
)
target_link_libraries(${COMPONENT_LIB} INTERFACE zenohpico_static)
