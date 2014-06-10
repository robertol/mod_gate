
find_package(FLEX)

flex_target( rhtmlscanner 
  rhtml.lex ${CMAKE_CURRENT_BIANRY_DIR}/rhtml.yy.cpp
  COMPILE_FLAGS "-R -P rhtml2" )

#execute_process( COMMAND flex -R -P rhtml2 -orhtml_2.yy.cpp rhtml_2.lex
#                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )

set( RHTML_SRC_C ruby.cpp rhtml_parser.cpp 
     rhtml.cpp ${FLEX_rhtmlscanner_OUTPUTS} )

add_executable(rhtml ${RHTML_SRC_C})

add_dependencies(rhtml rhtmlscanner)

target_link_libraries(rhtml libmruby_static ${MRUBY_LIBS})
set_target_properties(rhtml PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS rhtml RUNTIME DESTINATION bin)
