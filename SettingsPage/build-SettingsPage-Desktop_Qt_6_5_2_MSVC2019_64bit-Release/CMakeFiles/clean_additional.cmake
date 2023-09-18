# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\SettingsPage_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\SettingsPage_autogen.dir\\ParseCache.txt"
  "SettingsPage_autogen"
  )
endif()
