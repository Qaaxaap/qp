#include "cli.hpp"
#include "util.hpp"

#include <cstdio>
#include <cstring>

int
main (int argc, char* argv[])
{
  i18n_init ();

  if (argc < 2)
    {
      print_usage (argv[0]);
      return 1;
    }

  const char* cmd = argv[1];

  if (std::strcmp (cmd, "--help") == 0)
    {
      print_usage (argv[0]);
      return 0;
    }

  if (std::strcmp (cmd, "--version") == 0)
    {
      print_version ();
      return 0;
    }

  const Subcommand* sc = find_subcommand (cmd);
  if (sc == nullptr)
    {
      std::printf (_ ("%s: unknown command '%s'\n"), argv[0], cmd);
      std::printf (_ ("Try '%s --help' for more information.\n"), argv[0]);
      return 1;
    }

  return sc->fn (argc - 1, argv + 1);
}
