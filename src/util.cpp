#include "util.hpp"

#include <cstdlib>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static const char*
find_locale_dir ()
{
  const char* dir = std::getenv ("TEXTDOMAINDIR");
  if (dir != nullptr)
    return dir;

  if (access (LOCALEDIR, F_OK) == 0)
    return LOCALEDIR;

  if (access ("po/locale", F_OK) == 0)
    return "po/locale";

  return LOCALEDIR;
}

void
i18n_init ()
{
  setlocale (LC_ALL, "");
  bindtextdomain ("qp", find_locale_dir ());
  textdomain ("qp");
}

int
run_command (const std::vector<std::string>& args)
{
  if (args.empty ())
    return -1;

  pid_t pid = fork ();
  if (pid == -1)
    return -1;

  if (pid == 0)
    {
      std::vector<char*> argv;
      argv.reserve (args.size () + 1);
      for (const auto& a : args)
        argv.push_back (const_cast<char*> (a.c_str ()));
      argv.push_back (nullptr);

      execvp (argv[0], argv.data ());
      _exit (127);
    }

  int status;
  if (waitpid (pid, &status, 0) == -1)
    return -1;

  if (WIFEXITED (status))
    return WEXITSTATUS (status);
  return -1;
}
