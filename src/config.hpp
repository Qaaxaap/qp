#pragma once

#include <string>

struct Config
{
  std::string repo_url = "https://github.com/Qaaxaap/qur.git";
  std::string repo_dir = "/var/lib/qur";
  std::string build_dir = "/tmp/qp-build";
  std::string log_dir = "/var/log/qp";
};

Config load_config ();
