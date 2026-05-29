#include "makefs.hpp"

#include <cassert>
#include <cstdio>
#include <string>

static int failures = 0;

static void
check (bool cond, const char *msg)
{
  if (!cond)
    {
      std::printf ("FAIL: %s\n", msg);
      ++failures;
    }
}

/* ------------------------------------------------------------------ */

static void
test_minimal ()
{
  qp::MakefsSpec spec;
  std::string err;
  const char *input = "NAME=hello\nVERSION=1.0\n";
  bool ok = qp::parse_makefs_string (input, "(test_minimal)", spec, err);
  check (ok, "minimal parse ok");
  check (spec.name == "hello", "minimal name");
  check (spec.version == "1.0", "minimal version");
}

static void
test_nginx_full ()
{
  const char *input = R"(NAME = nginx
VERSION = 1.26.0
RELEASE = 1
DESC = "High-performance HTTP server"
HOMEPAGE = "https://nginx.org"
LICENSE = "BSD-2-Clause"
DEPENDS = pcre openssl zlib
BDEPENDS = gcc make pkg-config
SOURCE = "https://nginx.org/download/nginx-$(VERSION).tar.gz"
SHA256 = "abc123..."

PRE_INSTALL = <<EOF
getent group nginx || groupadd nginx
EOF

[package.nginx-doc]
DESC = "Nginx documentation"
DEPENDS = nginx

[package.nginx-module-geoip]
DESC = "GeoIP module for nginx"
DEPENDS = nginx geoip

[trigger]
path = /usr/share/info
on = post-transaction
run = update-info-dir

prepare() {
    tar xf nginx-$(VERSION).tar.gz
}

build() {
    cd nginx-$(VERSION)
    ./configure --prefix=/usr
    make
}

package() {
    cd nginx-$(VERSION)
    make DESTDIR="$STAGE" install
}

package_nginx-doc() {
    mkdir -p "$STAGE/usr/share/doc/nginx"
    cp -r docs/* "$STAGE/usr/share/doc/nginx/"
}
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_nginx)", spec, err);
  check (ok, "nginx parse ok");
  if (!ok)
    std::printf ("  err: %s\n", err.c_str ());

  check (spec.name == "nginx", "nginx name");
  check (spec.version == "1.26.0", "nginx version");
  check (spec.release == "1", "nginx release");
  check (spec.description == "High-performance HTTP server", "nginx desc");
  check (spec.homepage == "https://nginx.org", "nginx homepage");
  check (spec.license == "BSD-2-Clause", "nginx license");

  check (spec.depends.size () == 3, "nginx depends count");
  check (spec.bdepends.size () == 3, "nginx bdepends count");

  /* Variable expansion in SOURCE. */
  check (spec.sources.size () == 1, "nginx sources count");
  check (spec.sources[0] == "https://nginx.org/download/nginx-1.26.0.tar.gz",
         "nginx source expansion");

  check (spec.sha256sums.size () == 1, "nginx sha256 count");
  check (spec.sha256sums[0] == "abc123...", "nginx sha256");

  /* Heredoc hook. */
  check (spec.pre_install.find ("groupadd nginx") != std::string::npos,
         "nginx pre_install heredoc");

  /* Sub-packages. */
  check (spec.subpackages.size () == 2, "nginx subpackage count");
  check (spec.subpackages[0].name == "nginx-doc", "subpkg 1 name");
  check (spec.subpackages[0].description == "Nginx documentation",
         "subpkg 1 desc");
  check (spec.subpackages[0].depends.size () == 1, "subpkg 1 depends count");
  check (spec.subpackages[0].depends[0] == "nginx", "subpkg 1 depends");
  check (spec.subpackages[1].name == "nginx-module-geoip", "subpkg 2 name");

  /* Trigger. */
  check (spec.triggers.size () == 1, "trigger count");
  check (spec.triggers[0].path == "/usr/share/info", "trigger path");
  check (spec.triggers[0].on == "post-transaction", "trigger on");
  check (spec.triggers[0].run == "update-info-dir", "trigger run");

  /* Functions. */
  check (spec.functions.count ("prepare") == 1, "prepare exists");
  check (spec.functions.count ("build") == 1, "build exists");
  check (spec.functions.count ("package") == 1, "package exists");
  check (spec.functions.count ("package_nginx-doc") == 1,
         "package_nginx-doc exists");

  /* $(VERSION) in function body is NOT expanded. */
  check (spec.functions["prepare"].find ("$(VERSION)") != std::string::npos,
         "function body keeps $(VERSION) raw");
  check (spec.functions["build"].find ("$(VERSION)") != std::string::npos,
         "build body keeps $(VERSION) raw");
}

static void
test_variable_expansion ()
{
  const char *input = R"(NAME = foo
VERSION = 1.0
A = $(VERSION)
B = ${A}-r1
C = unexpanded $(MISSING)
SOURCE = http://example.com/$(NAME)-$(B).tar.gz
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_var_expand)", spec, err);
  check (ok, "var expand parse ok");

  /* C has $(MISSING) which is undefined — left verbatim. */
  /* SOURCE should expand NAME and B.  B = A-r1 = $(VERSION)-r1 = 1.0-r1. */
  check (spec.sources.size () == 1, "var expand source count");
  check (spec.sources[0]
             == "http://example.com/foo-1.0-r1.tar.gz",
         "var expand chained");
}

static void
test_heredoc ()
{
  const char *input = R"(NAME = test
VERSION = 1.0
PRE_INSTALL = <<EOF
line one
line two
EOF
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_heredoc)", spec, err);
  check (ok, "heredoc parse ok");
  check (spec.pre_install == "line one\nline two\n", "heredoc body verbatim");
}

static void
test_function_body ()
{
  const char *input = R"mak(NAME = test
VERSION = 1

build() {
    echo "building $(NAME)"
    make -j$(nproc)
}
)mak";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_func)", spec, err);
  check (ok, "function body parse ok");
  check (spec.functions.count ("build") == 1, "function exists");

  /* $(NAME) and $(nproc) preserved as raw shell. */
  check (spec.functions["build"].find ("$(NAME)") != std::string::npos,
         "func body keeps $(NAME)");
  check (spec.functions["build"].find ("$(nproc)") != std::string::npos,
         "func body keeps $(nproc)");
}

static void
test_brace_expansion_in_function ()
{
  /* Brace expansion like {a,b} should not confuse the parser. */
  const char *input = R"(NAME = test
VERSION = 1

build() {
    echo {a,b,c}
    for i in {1..10}; do echo $i; done
}
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_brace)", spec, err);
  check (ok, "brace expansion parse ok");
  check (spec.functions.count ("build") == 1, "function exists");
}

static void
test_backslash_continuation ()
{
  const char *input = "NAME = foo \\\n  bar\nVERSION = 1\n";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_cont)", spec, err);
  check (ok, "continuation parse ok");
  check (spec.name == "foo   bar", "continuation joined name");
}

static void
test_comments_and_blanks ()
{
  const char *input = R"(# This is a comment

NAME = test

# another comment
VERSION = 2
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_comments)", spec, err);
  check (ok, "comments parse ok");
  check (spec.name == "test", "comments name");
  check (spec.version == "2", "comments version");
}

static void
test_duplicate_keys ()
{
  const char *input = R"(NAME = first
NAME = second
VERSION = 1
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_dup)", spec, err);
  check (ok, "duplicate keys parse ok");
  check (spec.name == "second", "duplicate last wins");
}

static void
test_crlf ()
{
  const char *input = "NAME=foo\r\nVERSION=2\r\n";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test_crlf)", spec, err);
  check (ok, "CRLF parse ok");
  check (spec.name == "foo", "CRLF name");
  check (spec.version == "2", "CRLF version");
}

static void
test_error_unclosed_heredoc ()
{
  const char *input = R"(NAME = test
VERSION = 1
PRE_INSTALL = <<EOF
some content
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test)", spec, err);
  check (!ok, "unclosed heredoc fails");
  check (err.find ("unclosed heredoc") != std::string::npos,
         "unclosed heredoc message");
}

static void
test_error_unclosed_function ()
{
  const char *input = R"(NAME = test
VERSION = 1
build() {
    echo hi
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test)", spec, err);
  check (!ok, "unclosed function fails");
  check (err.find ("unclosed function") != std::string::npos,
         "unclosed function message");
}

static void
test_error_malformed_section ()
{
  qp::MakefsSpec spec;
  std::string err;

  bool ok = qp::parse_makefs_string (
      "NAME = test\n[bad", "(test)", spec, err);
  check (!ok, "malformed section fails");
  check (err.find ("malformed") != std::string::npos,
         "malformed section message");
}

static void
test_error_file_not_found ()
{
  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs ("/nonexistent/MAKEFS", spec, err);
  check (!ok, "file not found fails");
  check (err.find ("cannot open") != std::string::npos,
         "file not found message");
}

static void
test_empty_file ()
{
  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string ("", "(test_empty)", spec, err);
  check (ok, "empty file parse ok");
  check (spec.name.empty (), "empty file name empty");
}

static void
test_depends_comma_separated ()
{
  const char *input = "NAME = test\nVERSION = 1\nDEPENDS = a, b, c\n";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test)", spec, err);
  check (ok, "comma depends parse ok");
  check (spec.depends.size () == 3, "comma depends count");
  check (spec.depends[0] == "a", "comma depends[0]");
  check (spec.depends[1] == "b", "comma depends[1]");
  check (spec.depends[2] == "c", "comma depends[2]");
}

static void
test_empty_function_body ()
{
  const char *input = R"(NAME = test
VERSION = 1
check() {}
)";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test)", spec, err);
  check (ok, "empty func parse ok");
  check (spec.functions.count ("check") == 1, "empty func exists");
}

static void
test_quoted_string_preserves_spaces ()
{
  const char *input
      = "NAME = test\nVERSION = 1\nDESC = \"hello world\"\n";

  qp::MakefsSpec spec;
  std::string err;
  bool ok = qp::parse_makefs_string (input, "(test)", spec, err);
  check (ok, "quoted parse ok");
  check (spec.description == "hello world", "quoted space preserved");
}

int
main ()
{
  test_minimal ();
  test_nginx_full ();
  test_variable_expansion ();
  test_heredoc ();
  test_function_body ();
  test_brace_expansion_in_function ();
  test_backslash_continuation ();
  test_comments_and_blanks ();
  test_duplicate_keys ();
  test_crlf ();
  test_error_unclosed_heredoc ();
  test_error_unclosed_function ();
  test_error_malformed_section ();
  test_error_file_not_found ();
  test_empty_file ();
  test_depends_comma_separated ();
  test_empty_function_body ();
  test_quoted_string_preserves_spaces ();

  if (failures)
    {
      std::printf ("\n%d test(s) FAILED\n", failures);
      return 1;
    }
  std::printf ("All tests passed.\n");
  return 0;
}
