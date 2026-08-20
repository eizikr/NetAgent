cmake_minimum_required(VERSION 3.20)

project(netagent LANGUAGES C)

set(CMAKE_C_STANDARD 99) 		# Compile according to C99 standard.
set(CMAKE_C_STANDARD_REQUIRED ON)	# If cant provide this standard, fail and not "replace" with another one.
set(CMAKE_C_EXTENSIONS OFF)		# Work with ISO C and not GNU dialect

enable_testing()

add_executable(netagent
	src/main.c
	src/ethernet.c
	src/ipv4.c
	src/packet.c
	src/icmp.c
	src/udp.c
	src/twamp.c
	src/capture.c
)

add_executable(test_ethernet
    tests/test_ethernet.c
    src/ethernet.c
)

add_executable(test_ipv4
    tests/test_ipv4.c
    src/ipv4.c
)

target_include_directories(netagent PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(test_ethernet PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(test_ipv4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(netagent PRIVATE	# Add warnings, private because those flags belong to the target - netagent
	-Wall
	-Wextra
	-Wpedantic
)

target_compile_options(test_ipv4 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_compile_options(test_ethernet PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME ipv4_parser
    COMMAND test_ipv4
)

add_test(
    NAME ethernet_parser
    COMMAND test_ethernet
)


add_executable(test_udp
    tests/test_udp.c
    src/udp.c
)

target_include_directories(test_udp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(test_udp PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME udp_parser
	COMMAND test_udp
)
    

# cmake -S . -B build/cmake  (Create build system)
# cmake --build build/cmake  (Activate the build system)
# cmake --build build/cmake --verbose (Show the actual GCC commands)
# if no changes needed you can run touch src/main.c
