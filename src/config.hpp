#pragma once

#include <string>

struct Config
{
  std::string qur_url = "https://github.com/Qaaxaap/qur.git";
  std::string qur_dir = "/var/lib/qur";
  std::string build_dir = "/tmp/qp-build";
};

Config load_config ();
