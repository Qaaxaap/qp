#pragma once

using subcmd_fn = int (*)(int argc, char* argv[]);

struct Subcommand
{
  const char* name;
  const char* args;
  const char* desc;
  subcmd_fn fn;
};

int cmd_search (int argc, char* argv[]);
int cmd_install (int argc, char* argv[]);
int cmd_remove (int argc, char* argv[]);
int cmd_rebuild (int argc, char* argv[]);
int cmd_upgrade (int argc, char* argv[]);
int cmd_list (int argc, char* argv[]);
int cmd_update (int argc, char* argv[]);
int cmd_register (int argc, char* argv[]);
int cmd_unregister (int argc, char* argv[]);

void print_usage (const char* prog);
void print_version ();

const Subcommand* find_subcommand (const char* name);
