add_executable(mp_gentest mp_gentest.cpp)
set(MP_GENERATED_C ${CMAKE_BINARY_DIR}/mp_generated.c)

add_custom_command(OUTPUT ${MP_GENERATED_C}
  COMMAND $<TARGET_FILE:mp_gentest> > ${MP_GENERATED_C}
)

add_library(obj_mp_generated_w_pass OBJECT ${MP_GENERATED_C})
add_dependencies(obj_mp_generated_w_pass MathPeephole)
target_compile_definitions(obj_mp_generated_w_pass PRIVATE TEST=1)
target_compile_options(obj_mp_generated_w_pass PRIVATE
  "-DTEST" "-O3" "-ffast-math"
  "SHELL:-Xclang -load -Xclang ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:MathPeephole>")

add_library(obj_mp_generated_wo_pass OBJECT ${MP_GENERATED_C})

add_executable(mp_tester mp_tester.c
  $<TARGET_OBJECTS:obj_mp_generated_w_pass> $<TARGET_OBJECTS:obj_mp_generated_wo_pass>)
target_link_libraries(mp_tester PUBLIC m)

add_test(NAME "MathPeephole" COMMAND mp_tester)
