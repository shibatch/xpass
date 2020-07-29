add_executable(mp_gentest mp_gentest.cpp)

set(MP_GENERATED_DOUBLE_C ${CMAKE_BINARY_DIR}/mp_generated_double.c)
add_custom_command(OUTPUT ${MP_GENERATED_DOUBLE_C}
  COMMAND $<TARGET_FILE:mp_gentest> double bool asqrt > ${MP_GENERATED_DOUBLE_C}
)

set(MP_GENERATED_FLOAT_C ${CMAKE_BINARY_DIR}/mp_generated_float.c)
add_custom_command(OUTPUT ${MP_GENERATED_FLOAT_C}
  COMMAND $<TARGET_FILE:mp_gentest> float bool asqrtf > ${MP_GENERATED_FLOAT_C}
)

set(MP_GENERATED_DOUBLE2_C ${CMAKE_BINARY_DIR}/mp_generated_double2.c)
add_custom_command(OUTPUT ${MP_GENERATED_DOUBLE2_C}
  COMMAND $<TARGET_FILE:mp_gentest> double2 int2 asqrt2 > ${MP_GENERATED_DOUBLE2_C}
)

set(MP_GENERATED_FLOAT4_C ${CMAKE_BINARY_DIR}/mp_generated_float4.c)
add_custom_command(OUTPUT ${MP_GENERATED_FLOAT4_C}
  COMMAND $<TARGET_FILE:mp_gentest> float4 int4 asqrtf4 > ${MP_GENERATED_FLOAT4_C}
)

add_custom_target(MathPeephole_generated
  DEPENDS ${TARGET_MATHPEEPHOLE}
  COMMAND echo a > MathPeephole_generated
  )

foreach(TYPE DOUBLE FLOAT DOUBLE2 FLOAT4)
  string(TOLOWER ${TYPE} lctype)

  add_library(obj_mp_generated_${lctype}_w_pass OBJECT ${MP_GENERATED_${TYPE}_C})
  add_dependencies(obj_mp_generated_${lctype}_w_pass MathPeephole_generated)
  target_compile_definitions(obj_mp_generated_${lctype}_w_pass PRIVATE TEST=1)
  target_compile_options(obj_mp_generated_${lctype}_w_pass PRIVATE
    "-DTEST" "-O1" "-ffast-math"
    "SHELL:-Xclang -load -Xclang ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>")

  add_library(obj_mp_generated_${lctype}_wo_pass OBJECT ${MP_GENERATED_${TYPE}_C})

  add_executable(mp_tester_${lctype} mp_tester.c
    $<TARGET_OBJECTS:obj_mp_generated_${lctype}_w_pass> $<TARGET_OBJECTS:obj_mp_generated_${lctype}_wo_pass>)
  target_compile_definitions(mp_tester_${lctype} PRIVATE ${TYPE}=1)
  target_link_libraries(mp_tester_${lctype} PUBLIC m)

  add_test(NAME "MathPeephole1_${TYPE}" COMMAND mp_tester_${lctype})
endforeach()

#

add_executable(mp_check mp_check.c)
add_dependencies(mp_check MathPeephole_generated)
target_compile_options(mp_check PRIVATE
  "-DTEST" "-O1" "-ffast-math"
  "SHELL:-Xclang -load -Xclang ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>")

add_test(NAME "MathPeephole2" COMMAND mp_check)

#

add_custom_target(check_ll ALL
  DEPENDS ${TARGET_MATHPEEPHOLE}
  COMMAND ${CMAKE_C_COMPILER} "-O1" "-ffast-math" "-emit-llvm" "-S"
  ${CMAKE_CURRENT_SOURCE_DIR}/check.ll "-o" "${CMAKE_BINARY_DIR}/check_out.ll"
  "-Xclang" "-load" "-Xclang" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>"
  )

find_program(FILECHECK_COMMAND NAMES FileCheck FileCheck-10)

if (FILECHECK_COMMAND)
  add_test(NAME MP_FileCheck COMMAND ${FILECHECK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/check.ll --input-file ${CMAKE_BINARY_DIR}/check_out.ll)
endif()

#

set(FLAGS_FOR_BENCHMARK "-O3" CACHE STRING "Compiler flags for benchmark")

add_executable(bench_latency_wo microbench.cpp)
target_compile_options(bench_latency_wo PRIVATE -ffast-math ${FLAGS_FOR_BENCHMARK})
target_compile_definitions(bench_latency_wo PRIVATE LATENCY=1)

add_executable(bench_throughput_wo microbench.cpp)
target_compile_options(bench_throughput_wo PRIVATE -ffast-math ${FLAGS_FOR_BENCHMARK})
target_compile_definitions(bench_throughput_wo PRIVATE THROUGHPUT=1)

add_executable(bench_latency_w microbench.cpp)
target_compile_options(bench_latency_w PRIVATE -ffast-math ${FLAGS_FOR_BENCHMARK}
  "SHELL:-Xclang -load -Xclang ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>")
target_compile_definitions(bench_latency_w PRIVATE LATENCY=1)
add_dependencies(bench_latency_w MathPeephole_generated)

add_executable(bench_throughput_w microbench.cpp)
target_compile_options(bench_throughput_w PRIVATE -ffast-math ${FLAGS_FOR_BENCHMARK}
  "SHELL:-Xclang -load -Xclang ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>")
target_compile_definitions(bench_throughput_w PRIVATE THROUGHPUT=1)
add_dependencies(bench_throughput_w MathPeephole_generated)
