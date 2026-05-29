#pragma once

#include <map>
#include <string>
#include <vector>

namespace qp {

struct SubPackage
{
  std::string name;
  std::string description;
  std::vector<std::string> depends;
};

struct Trigger
{
  std::string path;
  std::string on;
  std::string run;
};

struct MakefsSpec
{
  std::string name;
  std::string version;
  std::string release;
  std::string description;
  std::string homepage;
  std::string license;

  std::vector<std::string> depends;
  std::vector<std::string> bdepends;

  std::vector<std::string> sources;
  std::vector<std::string> sha256sums;

  std::string pre_install;
  std::string post_install;

  std::vector<SubPackage> subpackages;
  std::vector<Trigger> triggers;

  std::map<std::string, std::string> functions;
};

bool
parse_makefs (const std::string &path, MakefsSpec &out, std::string &err);

bool
parse_makefs_string (const std::string &content, const std::string &source_name,
                     MakefsSpec &out, std::string &err);

} // namespace qp
