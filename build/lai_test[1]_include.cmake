if(EXISTS "/home/eemil/dev/asmlai/build/lai_test[1]_tests.cmake")
  include("/home/eemil/dev/asmlai/build/lai_test[1]_tests.cmake")
else()
  add_test(lai_test_NOT_BUILT lai_test_NOT_BUILT)
endif()