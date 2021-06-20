# CMakeLists.txt

idf_component_register(SRCS "myfi.c"
  INCLUDE_DIRS "include" "."
  REQUIRES nvs_flash)
