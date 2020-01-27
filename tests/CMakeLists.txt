
include(CTest)

add_test(NAME m5p_4_4_4  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 4 4 4)
add_test(NAME m5p_4_4_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 4 4 10)
add_test(NAME m5p_5_4_4  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 5 4 4)

add_test(NAME m5p_10_4_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 10 4 10)
add_test(NAME m5p_10_4_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 10 4 20)
add_test(NAME m5p_100_4_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 100 4 10)
add_test(NAME m5p_100_4_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 100 4 20)

add_test(NAME m5p_10_8_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 10 8 10)
add_test(NAME m5p_10_8_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 10 8 20)
add_test(NAME m5p_100_8_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 100 8 10)
add_test(NAME m5p_100_8_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 100 8 20)

add_test(NAME m9p_10_4_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 10 4 10)
add_test(NAME m9p_10_4_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 10 4 20)
add_test(NAME m9p_100_4_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 100 4 10)
add_test(NAME m9p_100_4_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 100 4 20)

add_test(NAME m9p_10_8_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 10 8 10)
add_test(NAME m9p_10_8_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 10 8 20)
add_test(NAME m9p_100_8_10  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 100 8 10)
add_test(NAME m9p_100_8_20  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m9p 100 8 20)

add_test(NAME bmw7st_1_4_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/GHS_psdef/bmw7st_1.tar.gz 4 10)
add_test(NAME bmw7st_1_4_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/GHS_psdef/bmw7st_1.tar.gz 4 20)
add_test(NAME bmw7st_1_8_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/GHS_psdef/bmw7st_1.tar.gz 8 10)
add_test(NAME bmw7st_1_8_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/GHS_psdef/bmw7st_1.tar.gz 8 20)

add_test(NAME cfd2_4_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/cfd2.tar.gz 4 10)
add_test(NAME cfd2_4_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/cfd2.tar.gz 4 20)
add_test(NAME cfd2_8_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/cfd2.tar.gz 8 10)
add_test(NAME cfd2_8_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/cfd2.tar.gz 8 20)

add_test(NAME gearbox_4_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/gearbox.tar.gz 4 10)
add_test(NAME gearbox_4_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/gearbox.tar.gz 4 20)
add_test(NAME gearbox_8_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/gearbox.tar.gz 8 10)
add_test(NAME gearbox_8_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Rothberg/gearbox.tar.gz 8 20)

add_test(NAME xenon2_4_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Ronis/xenon2.tar.gz 4 10)
add_test(NAME xenon2_4_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Ronis/xenon2.tar.gz 4 20)
add_test(NAME xenon2_8_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Ronis/xenon2.tar.gz 8 10)
add_test(NAME xenon2_8_20 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh"
  https://sparse.tamu.edu/MM/Ronis/xenon2.tar.gz 8 20)
