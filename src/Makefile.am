bin_PROGRAMS = qp

qp_SOURCES = main.cpp cli.cpp cli.hpp util.cpp util.hpp makefs.cpp makefs.hpp config.cpp config.hpp

qp_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir) -DLOCALEDIR='"$(localedir)"' @LFSPKG_CPPFLAGS@

qp_CXXFLAGS = -std=c++17 -Wall -Wextra

qp_LDADD = @LFSPKG_LIBS@
