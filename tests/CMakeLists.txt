configure_file(./CTestCustom.cmake.in ${CMAKE_BINARY_DIR}/CTestCustom.cmake)

set(type m5p)
set(size 4)
set(npart 4)
set(nlevel 4)
set(mirror 5)
set(weight 3)

add_test(NAME ${type}_${size}_${npart}_${nlevel}_m${mirror}w${weight}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gen_test.sh
  ${type} ${size} ${npart} ${nlevel} ${mirror} ${weight} --keepfiles)

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
set(mirrors 5)
set(weights 3)

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
