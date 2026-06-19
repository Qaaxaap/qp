#include "config.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

static std::string
trim (std::string s)
{
  size_t a = 0;
  while (a < s.size () && (s[a] == ' ' || s[a] == '\t'))
    ++a;
  size_t b = s.size ();
  while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t'))
    --b;
  return s.substr (a, b - a);
}

static std::string
unquote (const std::string &v)
{
  auto t = trim (v);
  if (t.size () >= 2 && t.front () == '"' && t.back () == '"')
    return t.substr (1, t.size () - 2);
  return t;
}

static void
apply (Config &cfg, const std::string &key, const std::string &value)
{
  if (key == "REPO_URL")
    cfg.repo_url = unquote (value);
  else if (key == "REPO_DIR")
    cfg.repo_dir = unquote (value);
  else if (key == "BUILD_DIR")
    cfg.build_dir = unquote (value);
  else if (key == "LOG_DIR")
    cfg.log_dir = unquote (value);
}

static void
parse_file (const std::string &path, Config &cfg)
{
  std::ifstream in (path);
  if (!in)
    return;

  std::string line;
  while (std::getline (in, line))
    {
      line = trim (line);
      if (line.empty () || line[0] == '#')
        continue;

      auto pos = line.find ('=');
      if (pos == std::string::npos)
        continue;

      std::string key = trim (line.substr (0, pos));
      std::string value = trim (line.substr (pos + 1));
      apply (cfg, key, value);
    }
}

Config
load_config ()
{
  Config cfg;

  parse_file ("/etc/qp.conf", cfg);

  const char *home = std::getenv ("HOME");
  if (home != nullptr)
    {
      std::string user_conf = std::string (home) + "/.config/qp/qp.conf";
      parse_file (user_conf, cfg);
    }

  return cfg;
}
