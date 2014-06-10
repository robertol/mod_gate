file(GLOB MIRBBIN_SRC_C . tools/mirb/*.c)
add_executable(mirb ${MIRBBIN_SRC_C})
target_link_libraries(mirb libmruby_static ${MRUBY_LIBS})

install(TARGETS mirb RUNTIME DESTINATION bin)
