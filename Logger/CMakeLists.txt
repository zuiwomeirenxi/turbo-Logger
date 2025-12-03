cmake_minimum_required(VERSION 3.10)
project(TurboLog LANGUAGES CXX)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(turbolog
	src/Logger.cpp
)
target_include_directories(turbolog PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

option(TURBOLOG_BUILD_EXAMPLES "Build TurboLog examples" ON)

if(TURBOLOG_BUILD_EXAMPLES)
	add_executable(turbolog_example examples/main.cpp)
	target_link_libraries(turbolog_example PRIVATE turbolog Threads::Threads)
	# logger_example and macros_example not built (sources missing)
	# perf_main removed per request
endif()

# Install headers for consumers
install(DIRECTORY include/ DESTINATION include)
