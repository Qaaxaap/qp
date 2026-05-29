check_PROGRAMS = test_main test_makefs

test_main_SOURCES = test_main.cpp
test_main_CPPFLAGS = -I$(top_srcdir)/include
test_main_CXXFLAGS = -std=c++17 -Wall -Wextra

test_makefs_SOURCES = test_makefs.cpp $(top_srcdir)/src/makefs.cpp
test_makefs_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
test_makefs_CXXFLAGS = -std=c++17 -Wall -Wextra

TESTS = test_main test_makefs
