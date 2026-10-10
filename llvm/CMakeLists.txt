cmake_minimum_required(VERSION 3.20)
project(anc216_llvm LANGUAGES C CXX)

# This bootstrap builds independently of the emulator/assembler. It uses LLVM's
# public libraries and TableGen before integration into an LLVM source tree.
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(ANC216_LLVM_TABLEGEN
    NAMES llvm-tblgen
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    REQUIRED)
execute_process(COMMAND "${ANC216_LLVM_TABLEGEN}" --version
    OUTPUT_VARIABLE ANC216_TABLEGEN_VERSION
    RESULT_VARIABLE ANC216_TABLEGEN_STATUS)
if(NOT ANC216_TABLEGEN_STATUS EQUAL 0 OR
   NOT ANC216_TABLEGEN_VERSION MATCHES "LLVM version ${LLVM_PACKAGE_VERSION}([^0-9.]|$)")
    message(FATAL_ERROR "llvm-tblgen must match LLVM ${LLVM_PACKAGE_VERSION}; use the same LLVM main build")
endif()

set(ANC216_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/Target/ANC216")
set(ANC216_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${ANC216_GENERATED_DIR}")
file(GLOB ANC216_TABLEGEN_DEPENDENCIES CONFIGURE_DEPENDS "${ANC216_TARGET_DIR}/*.td")
set(ANC216_TABLEGEN_INCLUDES -I "${ANC216_TARGET_DIR}")
foreach(include_dir IN LISTS LLVM_INCLUDE_DIRS)
    list(APPEND ANC216_TABLEGEN_INCLUDES -I "${include_dir}")
endforeach()
foreach(kind IN ITEMS RegisterInfo InstrInfo SubtargetInfo)
    if(kind STREQUAL "RegisterInfo")
        set(generator -gen-register-info)
    elseif(kind STREQUAL "InstrInfo")
        set(generator -gen-instr-info)
    else()
        set(generator -gen-subtarget)
    endif()
    set(output "${ANC216_GENERATED_DIR}/ANC216Gen${kind}.inc")
    add_custom_command(OUTPUT "${output}"
        COMMAND "${ANC216_LLVM_TABLEGEN}" ${ANC216_TABLEGEN_INCLUDES}
            ${generator} "${ANC216_TARGET_DIR}/ANC216.td" -o "${output}"
        DEPENDS ${ANC216_TABLEGEN_DEPENDENCIES}
        VERBATIM)
    list(APPEND ANC216_GENERATED_FILES "${output}")
endforeach()
add_custom_target(anc216-tables ALL DEPENDS ${ANC216_GENERATED_FILES})

llvm_map_components_to_libnames(ANC216_LLVM_LIBRARIES MC Support TargetParser)
add_executable(anc216-target-info
    tools/target-info.cpp
    lib/Target/ANC216/TargetInfo/ANC216TargetInfo.cpp)
target_include_directories(anc216-target-info SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(anc216-target-info PRIVATE "${ANC216_TARGET_DIR}")
separate_arguments(ANC216_LLVM_DEFINITIONS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(anc216-target-info PRIVATE ${ANC216_LLVM_DEFINITIONS})
target_link_libraries(anc216-target-info PRIVATE ${ANC216_LLVM_LIBRARIES})

enable_testing()
add_test(NAME anc216_target_registration COMMAND anc216-target-info)
set_tests_properties(anc216_target_registration PROPERTIES
    PASS_REGULAR_EXPRESSION "Registered target: anc216")
