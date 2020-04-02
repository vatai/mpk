configure_file(./CTestCustom.cmake.in ${CMAKE_BINARY_DIR}/CTestCustom.cmake)

# add_test(NAME m5p_4_4_4  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 4 4 4)
# add_test(NAME m5p_4_4_10 COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 4 4 10)
# add_test(NAME m5p_5_4_4  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh" m5p 5 4 4)

# Partitioning algorithm search log:
#
# mirror:
# - cfd2 fails for 0, 1, 3
#
# weight update:
# - 1 generally performs poorly

set(CTEST_CUSTOM_PRE_TEST ../scripts/gen_matrices.sh)

set(nparts 4 8)
set(nlevels 10 20)
set(mirrors 2 4 5)
set(weights 0 2)

foreach(type m5p m9p)
  foreach(size 10 100)
    foreach(npart ${nparts})
      foreach(nlevel ${nlevels})
        foreach(mirror ${mirrors})
          foreach(weight ${weights})
            add_test(NAME ${type}_${size}_${npart}_${nlevel}_m${mirror}w${weight}
              COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh ${type} ${size} ${npart} ${nlevel} ${mirror} ${weight})
          endforeach(weight)
        endforeach(mirror)
      endforeach(nlevel)
    endforeach(npart)
  endforeach(size)
endforeach(type)

foreach(mtx GHS_psdef/bmw7st_1.tar.gz Rothberg/cfd2.tar.gz Rothberg/gearbox.tar.gz Ronis/xenon2.tar.gz)
  foreach(npart ${nparts})
    foreach(nlevel ${nlevels})
      foreach(mirror ${mirrors})
        foreach(weight ${weights})
          add_test(NAME "${mtx}_${npart}_${nlevel}_m${mirror}w${weight}"
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get_test.sh
            https://sparse.tamu.edu/MM/${mtx} ${npart} ${nlevel} ${mirror} ${weight})
        endforeach(weight)
      endforeach(mirror)
    endforeach(nlevel)
  endforeach(npart)
endforeach(mtx)
