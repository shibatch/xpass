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

foreach(TYPE DOUBLE FLOAT DOUBLE2 FLOAT4)
  string(TOLOWER ${TYPE} lctype)

  add_library(obj_mp_generated_${lctype}_w_pass OBJECT ${MP_GENERATED_${TYPE}_C})
  add_dependencies(obj_mp_generated_${lctype}_w_pass MathPeephole)
  target_compile_definitions(obj_mp_generated_${lctype}_w_pass PRIVATE TEST=1)
  target_compile_options(obj_mp_generated_${lctype}_w_pass PRIVATE
    "-DTEST" "-O3" "-ffast-math"
    "SHELL:-Xclang -load -Xclang ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:MathPeephole>")

  add_library(obj_mp_generated_${lctype}_wo_pass OBJECT ${MP_GENERATED_${TYPE}_C})

  add_executable(mp_tester_${lctype} mp_tester.c
    $<TARGET_OBJECTS:obj_mp_generated_${lctype}_w_pass> $<TARGET_OBJECTS:obj_mp_generated_${lctype}_wo_pass>)
  target_compile_definitions(mp_tester_${lctype} PRIVATE ${TYPE}=1)
  target_link_libraries(mp_tester_${lctype} PUBLIC m)

  add_test(NAME "MathPeephole_${TYPE}" COMMAND mp_tester_${lctype})
endforeach()
