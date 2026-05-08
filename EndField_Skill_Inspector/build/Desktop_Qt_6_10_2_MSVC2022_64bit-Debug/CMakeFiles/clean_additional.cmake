# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\EndField_Skill_Inspector_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\EndField_Skill_Inspector_autogen.dir\\ParseCache.txt"
  "EndField_Skill_Inspector_autogen"
  )
endif()
