## Add definitions ##
add_definitions(-D_LARGEFILE_SOURCE -D_LARGE_FILES -D_FILE_OFFSET_BITS=64)

## Targets ##
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include"
                    "${CMAKE_CURRENT_BINARY_DIR}/../include")
link_directories("${CMAKE_CURRENT_BINARY_DIR}/../lib")
foreach(src suftest mksary sasearch bwt unbwt)
  add_executable(${src} ${src}.cpp)
  target_link_libraries(${src} divsufsort)
	target_compile_features(${src} PUBLIC cxx_std_20)
endforeach(src)
