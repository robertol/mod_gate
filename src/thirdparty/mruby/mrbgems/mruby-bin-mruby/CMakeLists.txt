file(GLOB MRUBYBIN_SRC_C . tools/mruby/*.c)

MESSAGE( STATUS "MRUBYBIN_SRC_C:   ${MRUBYBIN_SRC_C}" )

add_executable(mruby ${MRUBYBIN_SRC_C})
target_link_libraries(mruby libmruby_static ${MRUBY_LIBS})

install(TARGETS mruby RUNTIME DESTINATION bin)
