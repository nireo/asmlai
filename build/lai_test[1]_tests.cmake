add_test( ParserTest.ExpressionStatement /home/eemil/dev/asmlai/build/lai_test [==[--gtest_filter=ParserTest.ExpressionStatement]==] --gtest_also_run_disabled_tests)
set_tests_properties( ParserTest.ExpressionStatement PROPERTIES WORKING_DIRECTORY /home/eemil/dev/asmlai/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set( lai_test_TESTS ParserTest.ExpressionStatement)
