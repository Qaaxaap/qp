#pragma once

#include <libintl.h>
#include <string>
#include <vector>

#define _(String)  gettext(String)
#define N_(String) (String)

void i18n_init();

int run_command(const std::vector<std::string>& args);
