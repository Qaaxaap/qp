#include "config.h"

#include "cli.hpp"
#include "util.hpp"

#include <cstdio>
#include <cstring>

static const Subcommand subcommands[]
    = { { "search", N_ ("<keyword>"),
          N_ ("Search available packages"),
          cmd_search },
        { "install", N_ ("<packages...>"),
          N_ ("Install packages and dependencies"),
          cmd_install },
        { "remove", N_ ("<packages...>"),
          N_ ("Remove installed packages"),
          cmd_remove },
        { "rebuild", N_ ("[packages...]"),
          N_ ("Rebuild installed packages"),
          cmd_rebuild },
        { "list", "",
          N_ ("List installed packages"),
          cmd_list },
        { "update", "",
          N_ ("Update package metadata from remote"),
          cmd_update },
        { "register", N_ ("<pkg> [version] [deps...]"),
          N_ ("Register manually-installed package"),
          cmd_register },
        { "unregister", N_ ("<pkg>"),
          N_ ("Unregister a manual package"),
          cmd_unregister },
        { nullptr, nullptr, nullptr, nullptr } };

void
print_usage (const char* prog)
{
  std::printf (_ ("Usage: %s <command> [options...]\n\n"), prog);
  std::printf ("%s\n\n", _ ("Commands:"));

  for (const Subcommand* sc = subcommands; sc->name != nullptr; ++sc)
    {
      if (sc->args[0] != '\0')
        std::printf ("  %-12s %-22s %s\n", sc->name, sc->args,
                     _ (sc->desc));
      else
        std::printf ("  %-12s %-22s %s\n", sc->name, "", _ (sc->desc));
    }

  std::printf ("\n%s\n", _ ("Options:"));
  std::printf ("  %-12s %-22s %s\n", "--help", "", _ ("Show this help"));
  std::printf ("  %-12s %-22s %s\n", "--version", "",
               _ ("Show version information"));
}

void
print_version ()
{
  std::printf ("qp %s\n", PACKAGE_VERSION);
  std::printf ("%s\n",
               _ ("Copyright (C) 2026 Qaaxaap.  License GPLv3+."));
}

static int
not_impl (const char* name)
{
  std::printf (_ ("%s: not yet implemented.\n"), name);
  return 0;
}

int
cmd_search (int, char**) { return not_impl ("search"); }
int
cmd_install (int, char**) { return not_impl ("install"); }
int
cmd_remove (int, char**) { return not_impl ("remove"); }
int
cmd_rebuild (int, char**) { return not_impl ("rebuild"); }
int
cmd_list (int, char**) { return not_impl ("list"); }
int
cmd_update (int, char**) { return not_impl ("update"); }
int
cmd_register (int, char**) { return not_impl ("register"); }
int
cmd_unregister (int, char**) { return not_impl ("unregister"); }

const Subcommand*
find_subcommand (const char* name)
{
  for (const Subcommand* sc = subcommands; sc->name != nullptr; ++sc)
    {
      if (std::strcmp (sc->name, name) == 0)
        return sc;
    }
  return nullptr;
}
