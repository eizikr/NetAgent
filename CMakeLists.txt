cmake_minimum_required(VERSION 3.20)

project(netagent LANGUAGES C)

set(CMAKE_C_STANDARD 99) 		    # Compile according to C99 standard.
set(CMAKE_C_STANDARD_REQUIRED ON)	# If cant provide this standard, fail and not "replace" with another one.
set(CMAKE_C_EXTENSIONS OFF)		    # Work with ISO C and not GNU dialect

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
    src/tx.c
    src/stats.c
)

target_compile_definitions(netagent PRIVATE
    _POSIX_C_SOURCE=200809L
    _GNU_SOURCE
)

target_include_directories(netagent PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(netagent PRIVATE	# Add warnings, private because those flags belong to the target - netagent
-Wall
-Wextra
-Wpedantic
)


# TESTS:

##################
# ETHERNET TESTS #
##################
add_executable(test_ethernet
    tests/test_ethernet.c
    src/ethernet.c
)

target_include_directories(test_ethernet PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(test_ethernet PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME ethernet_parser
    COMMAND test_ethernet
)

###############
# IPv4 TESTS #
###############
add_executable(test_ipv4
    tests/test_ipv4.c
    src/ipv4.c
)

target_include_directories(test_ipv4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_compile_options(test_ipv4 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME ipv4_parser
    COMMAND test_ipv4
)


##############
# UDP TESTS #
##############
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
    
##############
# ICMP TESTS #
##############
add_executable(test_icmp
    tests/test_icmp.c
    src/icmp.c
)

target_include_directories(test_icmp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(test_icmp PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME icmp_parser
    COMMAND test_icmp
)


###############
# TWAMP TESTS #
###############
add_executable(test_twamp
    tests/test_twamp.c
    src/twamp.c
)

target_include_directories(test_twamp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(test_twamp PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME twamp_parser
    COMMAND test_twamp
)

###############
# STATS TESTS #
###############
add_executable(test_stats
    src/stats.c
    tests/test_stats.c
)

target_include_directories(test_stats PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(
    NAME stats_tracking
    COMMAND test_stats
)

# cmake -S . -B build/cmake  (Create build system)
# cmake --build build/cmake  (Activate the build system)
# cmake --build build/cmake --verbose (Show the actual GCC commands)
# if no changes needed you can run touch src/main.c
