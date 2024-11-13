# CMakeList.txt : CMake project for ParcoDeliverable1, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (ParcoDeliverable1 "ParcoDeliverable1.cpp" "ParcoDeliverable1.h" "Defs.h" "Utils.h" "Bench.h" "Utils.cpp" "Matrix_utils.h" "Matrix_utils.cpp" "Matrix_manip.h" "Matrix_manip.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.16)
  set_property(TARGET ParcoDeliverable1 PROPERTY CXX_STANDARD 20)
else()
  set_property(TARGET ParcoDeliverable1 PROPERTY CXX_STANDARD 11)
endif()

target_link_libraries(ParcoDeliverable1 gomp)
target_link_options(ParcoDeliverable1 PUBLIC "-flto")

# TODO: Add tests and install targets if needed.
