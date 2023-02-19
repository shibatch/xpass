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

  add_custom_target(mp_tester_${lctype} ALL
    DEPENDS ${TARGET_MATHPEEPHOLE} ${MP_GENERATED_${TYPE}_C}
    COMMAND ${CMAKE_C_COMPILER} -S -emit-llvm -O1 -ffast-math ${MP_GENERATED_${TYPE}_C} -DTEST -o "${MP_GENERATED_${TYPE}_C}_wo.ll"
    COMMAND ${OPT_COMMAND} "-load-pass-plugin=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>"
    "-passes=math-peephole" "${MP_GENERATED_${TYPE}_C}_wo.ll" "-S" "-o" "${MP_GENERATED_${TYPE}_C}_w.ll"
    COMMAND ${CMAKE_C_COMPILER} -O1 -ffast-math -D${TYPE}
    ${CMAKE_CURRENT_SOURCE_DIR}/mp_tester.c "${MP_GENERATED_${TYPE}_C}_w.ll" ${MP_GENERATED_${TYPE}_C}
    -o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mp_tester_${lctype}"
    )

  add_test(NAME "MathPeephole1_${TYPE}" COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mp_tester_${lctype}")
endforeach()

#

add_custom_target(mp_check ALL
  DEPENDS ${TARGET_MATHPEEPHOLE}
  COMMAND ${CMAKE_C_COMPILER} -S -emit-llvm -O1 -ffast-math ${CMAKE_CURRENT_SOURCE_DIR}/mp_check.c -o "${CMAKE_BINARY_DIR}/mp_check_wo.ll"
  COMMAND ${OPT_COMMAND} "-load-pass-plugin=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>"
  "-passes=math-peephole" ${CMAKE_BINARY_DIR}/mp_check_wo.ll "-S" "-o" "${CMAKE_BINARY_DIR}/mp_check_w.ll"
  COMMAND ${CMAKE_C_COMPILER} -O1 -ffast-math "${CMAKE_BINARY_DIR}/mp_check_w.ll" -o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mp_check"
  )

add_test(NAME "MathPeephole2" COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mp_check")

#

add_custom_target(check_ll ALL
  DEPENDS ${TARGET_MATHPEEPHOLE}
  COMMAND ${OPT_COMMAND} "-load-pass-plugin=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>"
  "-passes=math-peephole" ${CMAKE_CURRENT_SOURCE_DIR}/check.ll "-S" "-o" "${CMAKE_BINARY_DIR}/check_out.ll"
  )

if (FILECHECK_COMMAND)
  add_test(NAME MP_FileCheck COMMAND ${FILECHECK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/check.ll --input-file ${CMAKE_BINARY_DIR}/check_out.ll)
endif()

#

set(FLAGS_FOR_BENCHMARK "-O3" CACHE STRING "Compiler flags for benchmark")

add_executable(bench_latency_wo microbench.cpp)
target_compile_options(bench_latency_wo PRIVATE -ffast-math ${FLAGS_FOR_BENCHMARK} -Wno-psabi)
target_compile_definitions(bench_latency_wo PRIVATE LATENCY=1)

add_executable(bench_throughput_wo microbench.cpp)
target_compile_options(bench_throughput_wo PRIVATE -ffast-math ${FLAGS_FOR_BENCHMARK} -Wno-psabi)
target_compile_definitions(bench_throughput_wo PRIVATE THROUGHPUT=1)

add_custom_target(bench_latency_w ALL
  DEPENDS ${TARGET_MATHPEEPHOLE}
  COMMAND ${CMAKE_CXX_COMPILER} -S -emit-llvm -O1 -DLATENCY -ffast-math -Wno-psabi
  ${CMAKE_CURRENT_SOURCE_DIR}/microbench.cpp -o "${CMAKE_BINARY_DIR}/microbench_l_wo.ll"
  COMMAND ${OPT_COMMAND} "-load-pass-plugin=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>"
  "-passes=math-peephole" ${CMAKE_BINARY_DIR}/microbench_l_wo.ll "-S" "-o" "${CMAKE_BINARY_DIR}/microbench_l_w.ll"
  COMMAND ${CMAKE_CXX_COMPILER} -O1 -ffast-math "${CMAKE_BINARY_DIR}/microbench_l_w.ll" -o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bench_latency_w"
  )

add_custom_target(bench_throughput_w ALL
  DEPENDS ${TARGET_MATHPEEPHOLE}
  COMMAND ${CMAKE_CXX_COMPILER} -S -emit-llvm -O1 -DTHROUGHPUT -ffast-math -Wno-psabi
  ${CMAKE_CURRENT_SOURCE_DIR}/microbench.cpp -o "${CMAKE_BINARY_DIR}/microbench_t_wo.ll"
  COMMAND ${OPT_COMMAND} "-load-pass-plugin=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_MATHPEEPHOLE}>"
  "-passes=math-peephole" ${CMAKE_BINARY_DIR}/microbench_t_wo.ll "-S" "-o" "${CMAKE_BINARY_DIR}/microbench_t_w.ll"
  COMMAND ${CMAKE_CXX_COMPILER} -O1 -ffast-math "${CMAKE_BINARY_DIR}/microbench_t_w.ll" -o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bench_throughput_w"
  )
