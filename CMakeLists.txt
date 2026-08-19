cmake_minimum_required(VERSION 3.20)

project(netagent LANGUAGES C)

set(CMAKE_C_STANDARD 99) 		# Compile according to C99 standard.
set(CMAKE_C_STANDARD_REQUIRED ON)	# If cant provide this standard, fail and not "replace" with another one.
set(CMAKE_C_EXTENSIONS OFF)		# Work with ISO C and not GNU dialect

add_executable(netagent
	src/main.c
	src/ethernet.c
	src/ipv4.c
	src/packet.c
	src/icmp.c
	src/udp.c
)

target_include_directories(netagent PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(netagent PRIVATE	# Add warnings, private because those flags belong to the target - netagent
	-Wall
	-Wextra
	-Wpedantic
)


# cmake -S . -B build/cmake  (Create build system)
# cmake --build build/cmake  (Activate the build system)
# cmake --build build/cmake --verbose (Show the actual GCC commands)
# if no changes needed you can run touch src/main.c
