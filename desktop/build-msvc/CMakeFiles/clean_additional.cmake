# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "BackPet_autogen"
  [[CMakeFiles\BackPet_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\BackPet_autogen.dir\ParseCache.txt]]
  )
endif()
