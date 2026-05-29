#include "makefs.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace qp {

namespace {

/* --- basic utilities ----------------------------------------------------- */

std::string
trim (const std::string &s)
{
  size_t a = 0;
  while (a < s.size () && std::isspace (static_cast<unsigned char> (s[a])))
    ++a;
  size_t b = s.size ();
  while (b > a && std::isspace (static_cast<unsigned char> (s[b - 1])))
    --b;
  return s.substr (a, b - a);
}

bool
starts_with (const std::string &s, const std::string &prefix)
{
  return s.size () >= prefix.size ()
         && s.compare (0, prefix.size (), prefix) == 0;
}

std::vector<std::string>
split_list (const std::string &s)
{
  std::vector<std::string> out;
  std::string cur;
  for (char c : s)
    {
      if (c == ',' || std::isspace (static_cast<unsigned char> (c)))
        {
          if (!cur.empty ())
            {
              out.push_back (cur);
              cur.clear ();
            }
        }
      else
        cur += c;
    }
  if (!cur.empty ())
    out.push_back (cur);
  return out;
}

std::string
unquote (const std::string &v)
{
  auto t = trim (v);
  if (t.size () >= 2 && t.front () == '"' && t.back () == '"')
    return t.substr (1, t.size () - 2);
  return t;
}

std::string
strip_cr (const std::string &s)
{
  if (!s.empty () && s.back () == '\r')
    return s.substr (0, s.size () - 1);
  return s;
}

/* --- variable expansion -------------------------------------------------- */

bool
expand_vars (std::string &value,
             const std::map<std::string, std::string> &vars, std::string &err)
{
  static const int kMaxIter = 10;
  for (int iter = 0; iter < kMaxIter; ++iter)
    {
      bool changed = false;
      for (size_t i = 0; i < value.size ();)
        {
          if (value[i] != '$')
            {
              ++i;
              continue;
            }
          if (i + 1 >= value.size ())
            {
              ++i;
              continue;
            }

          char open = value[i + 1];
          char close = (open == '(') ? ')' : (open == '{') ? '}' : '\0';
          if (close == '\0')
            {
              ++i;
              continue;
            }

          size_t start = i + 2;
          size_t end = value.find (close, start);
          if (end == std::string::npos)
            {
              ++i;
              continue;
            }

          std::string name = trim (value.substr (start, end - start));
          auto it = vars.find (name);
          if (it != vars.end ())
            {
              value.replace (i, end - i + 1, it->second);
              changed = true;
            }
          else
            i = end + 1;
        }
      if (!changed)
        return true;
    }
  err = "circular variable reference or expansion depth exceeded";
  return false;
}

/* --- parser state -------------------------------------------------------- */

enum class State
{
  TOP,
  HEREDOC,
  FUNCTION_BODY
};

enum class SectionKind
{
  NONE,
  PACKAGE,
  TRIGGER
};

struct ParseCtx
{
  const std::string &source;
  std::string &err;
  MakefsSpec &spec;

  State state = State::TOP;
  SectionKind section = SectionKind::NONE;

  std::map<std::string, std::string> raw_vars;
  std::map<std::string, std::string> section_vars;

  SubPackage cur_pkg{};
  Trigger cur_trig{};

  std::string heredoc_key;
  std::string heredoc_term;
  std::string heredoc_buf;

  std::string func_name;
  std::string func_buf;
  int func_depth = 0;
};

/* --- section flushing ---------------------------------------------------- */

void
flush_section (ParseCtx &ctx)
{
  if (ctx.section == SectionKind::NONE)
    return;

  if (ctx.section == SectionKind::PACKAGE)
    {
      for (const auto &[k, v] : ctx.section_vars)
        {
          if (k == "DESC")
            ctx.cur_pkg.description = unquote (v);
          else if (k == "DEPENDS")
            ctx.cur_pkg.depends = split_list (unquote (v));
        }
      ctx.spec.subpackages.push_back (std::move (ctx.cur_pkg));
      ctx.cur_pkg = SubPackage{};
    }
  else if (ctx.section == SectionKind::TRIGGER)
    {
      for (const auto &[k, v] : ctx.section_vars)
        {
          if (k == "path")
            ctx.cur_trig.path = v;
          else if (k == "on")
            ctx.cur_trig.on = v;
          else if (k == "run")
            ctx.cur_trig.run = v;
        }
      ctx.spec.triggers.push_back (std::move (ctx.cur_trig));
      ctx.cur_trig = Trigger{};
    }

  ctx.section = SectionKind::NONE;
  ctx.section_vars.clear ();
}

/* --- line classifiers (TOP state only) ----------------------------------- */

bool
try_parse_section (ParseCtx &ctx, const std::string &line, int lineno)
{
  if (line.empty () || line[0] != '[')
    return false;

  size_t end = line.find (']');
  if (end == std::string::npos)
    {
      ctx.err = ctx.source + ":" + std::to_string (lineno)
                + ": malformed section header";
      return true;
    }

  std::string body = trim (line.substr (1, end - 1));

  if (starts_with (body, "package."))
    {
      std::string name = trim (body.substr (8));
      if (name.empty ())
        {
          ctx.err = ctx.source + ":" + std::to_string (lineno)
                    + ": empty package name in section header";
          return true;
        }
      flush_section (ctx);
      ctx.section = SectionKind::PACKAGE;
      ctx.cur_pkg.name = name;
      ctx.section_vars.clear ();
      return true;
    }

  if (body == "trigger")
    {
      flush_section (ctx);
      ctx.section = SectionKind::TRIGGER;
      ctx.cur_trig = Trigger{};
      ctx.section_vars.clear ();
      return true;
    }

  ctx.err = ctx.source + ":" + std::to_string (lineno)
            + ": unknown section '" + body + "'";
  return true;
}

bool
try_parse_heredoc_start (ParseCtx &ctx, const std::string &line)
{
  auto pos = line.find ("= <<");
  if (pos == std::string::npos)
    return false;
  std::string key = trim (line.substr (0, pos));
  std::string term = trim (line.substr (pos + 4));
  if (key.empty () || term.empty ())
    return false;
  ctx.state = State::HEREDOC;
  ctx.heredoc_key = key;
  ctx.heredoc_term = term;
  ctx.heredoc_buf.clear ();
  return true;
}

bool
try_parse_function_header (ParseCtx &ctx, const std::string &line, int lineno)
{
  auto paren = line.find ("()");
  if (paren == std::string::npos)
    return false;

  std::string name = trim (line.substr (0, paren));
  if (name.empty ())
    return false;

  for (char c : name)
    {
      if (!std::isalnum (static_cast<unsigned char> (c)) && c != '_'
          && c != '-')
        {
          ctx.err = ctx.source + ":" + std::to_string (lineno)
                    + ": invalid function name '" + name + "'";
          return true;
        }
    }

  auto brace = line.find ('{', paren + 2);
  if (brace == std::string::npos)
    {
      ctx.err = ctx.source + ":" + std::to_string (lineno)
                + ": expected '{' after function header";
      return true;
    }

  flush_section (ctx);

  ctx.state = State::FUNCTION_BODY;
  ctx.func_name = name;
  ctx.func_depth = 1;
  std::string rest = trim (line.substr (brace + 1));
  for (char c : rest)
    {
      if (c == '{')
        ++ctx.func_depth;
      else if (c == '}')
        --ctx.func_depth;
    }

  if (ctx.func_depth == 0)
    {
      auto last = rest.rfind ('}');
      if (last != std::string::npos)
        rest = rest.substr (0, last);
      ctx.func_buf = rest;
      ctx.state = State::TOP;
      ctx.spec.functions[ctx.func_name] = trim (ctx.func_buf);
      return true;
    }
  ctx.func_buf = rest + "\n";
  return true;
}

bool
try_parse_assignment (ParseCtx &ctx, const std::string &line)
{
  auto pos = line.find ('=');
  if (pos == std::string::npos)
    return false;

  std::string key = trim (line.substr (0, pos));
  if (key.empty ())
    return false;

  std::string value = trim (line.substr (pos + 1));
  if (ctx.section == SectionKind::NONE)
    ctx.raw_vars[key] = value;
  else
    ctx.section_vars[key] = value;
  return true;
}

/* --- state processors ---------------------------------------------------- */

/* Read one logical line from |lines| starting at |idx|, handling backslash
   continuation.  Advances |idx| past all consumed lines. */
std::string
read_logical_line (const std::vector<std::string> &lines, size_t &idx)
{
  std::string result = lines[idx++];
  while (!result.empty () && result.back () == '\\')
    {
      result.pop_back ();
      if (idx >= lines.size ())
        break;
      result += lines[idx++];
    }
  return result;
}

bool
process_top_line (ParseCtx &ctx, const std::string &line, int lineno)
{
  std::string trimmed = trim (line);
  if (trimmed.empty () || trimmed[0] == '#')
    return true;

  if (try_parse_section (ctx, trimmed, lineno))
    return true;
  if (try_parse_heredoc_start (ctx, trimmed))
    return true;
  if (try_parse_function_header (ctx, trimmed, lineno))
    return true;
  if (try_parse_assignment (ctx, trimmed))
    return true;

  ctx.err = ctx.source + ":" + std::to_string (lineno)
            + ": unexpected token";
  return false;
}

bool
process_heredoc_line (ParseCtx &ctx, const std::string &line, int lineno)
{
  if (trim (line) == ctx.heredoc_term)
    {
      ctx.state = State::TOP;
      if (ctx.section == SectionKind::NONE)
        ctx.raw_vars[ctx.heredoc_key] = ctx.heredoc_buf;
      else
        ctx.section_vars[ctx.heredoc_key] = ctx.heredoc_buf;
      return true;
    }
  ctx.heredoc_buf += line + "\n";
  (void)lineno;
  return true;
}

bool
process_function_line (ParseCtx &ctx, const std::string &line)
{
  for (char c : line)
    {
      if (c == '{')
        ++ctx.func_depth;
      else if (c == '}')
        --ctx.func_depth;
    }

  if (ctx.func_depth == 0)
    {
      auto last = line.rfind ('}');
      if (last != std::string::npos)
        ctx.func_buf += line.substr (0, last);
      ctx.state = State::TOP;
      ctx.spec.functions[ctx.func_name] = trim (ctx.func_buf);
      return true;
    }

  ctx.func_buf += line + "\n";
  return true;
}

/* --- assign raw vars to spec --------------------------------------------- */

void
assign_spec_field (MakefsSpec &out, const std::string &key,
                   const std::string &value)
{
  if (key == "NAME")
    out.name = unquote (value);
  else if (key == "VERSION")
    out.version = unquote (value);
  else if (key == "RELEASE")
    out.release = unquote (value);
  else if (key == "DESC")
    out.description = unquote (value);
  else if (key == "HOMEPAGE")
    out.homepage = unquote (value);
  else if (key == "LICENSE")
    out.license = unquote (value);
  else if (key == "DEPENDS")
    out.depends = split_list (unquote (value));
  else if (key == "BDEPENDS")
    out.bdepends = split_list (unquote (value));
  else if (key == "SOURCE")
    out.sources = split_list (unquote (value));
  else if (key == "SHA256")
    out.sha256sums = split_list (unquote (value));
  else if (key == "PRE_INSTALL")
    out.pre_install = value;
  else if (key == "POST_INSTALL")
    out.post_install = value;
}

} // anonymous namespace

/* --- public API ---------------------------------------------------------- */

bool
parse_makefs_string (const std::string &content, const std::string &source_name,
                     MakefsSpec &out, std::string &err)
{
  out = MakefsSpec{};

  /* Split input into raw (physical) lines. */
  std::vector<std::string> raw_lines;
  {
    std::istringstream ss (content);
    std::string l;
    while (std::getline (ss, l))
      raw_lines.push_back (strip_cr (l));
  }

  ParseCtx ctx{ source_name, err, out };

  for (size_t i = 0; i < raw_lines.size ();)
    {
      int lineno = static_cast<int> (i + 1);
      bool ok = false;

      switch (ctx.state)
        {
        case State::TOP:
          {
            std::string logical = read_logical_line (raw_lines, i);
            ok = process_top_line (ctx, logical, lineno);
            break;
          }
        case State::HEREDOC:
          ok = process_heredoc_line (ctx, raw_lines[i], lineno);
          ++i;
          break;
        case State::FUNCTION_BODY:
          ok = process_function_line (ctx, raw_lines[i]);
          ++i;
          break;
        }

      if (!ok)
        return false;
      if (!err.empty ())
        return false;
    }

  /* Check for unterminated constructs. */
  if (ctx.state == State::HEREDOC)
    {
      err = source_name + ": unclosed heredoc (expected '"
            + ctx.heredoc_term + "')";
      return false;
    }
  if (ctx.state == State::FUNCTION_BODY)
    {
      err = source_name + ": unclosed function body '" + ctx.func_name + "'";
      return false;
    }

  flush_section (ctx);

  /* --- Pass 2: expand variables and assign ------------------------------- */

  /* Iteratively expand raw_vars until stable (handles cross-references). */
  for (int iter = 0; iter < 10; ++iter)
    {
      bool changed = false;
      for (auto &[k, v] : ctx.raw_vars)
        {
          std::string expanded = v;
          if (!expand_vars (expanded, ctx.raw_vars, err))
            return false;
          if (expanded != v)
            {
              v = expanded;
              changed = true;
            }
        }
      if (!changed)
        break;
    }

  for (const auto &[k, v] : ctx.raw_vars)
    assign_spec_field (out, k, v);

  /* Expand variables in sub-package and trigger fields. */
  for (auto &sub : out.subpackages)
    {
      expand_vars (sub.description, ctx.raw_vars, err);
      for (auto &d : sub.depends)
        expand_vars (d, ctx.raw_vars, err);
    }
  for (auto &trig : out.triggers)
    {
      expand_vars (trig.path, ctx.raw_vars, err);
      expand_vars (trig.run, ctx.raw_vars, err);
    }

  return true;
}

bool
parse_makefs (const std::string &path, MakefsSpec &out, std::string &err)
{
  std::ifstream in (path);
  if (!in)
    {
      err = "cannot open MAKEFS: " + path;
      return false;
    }
  std::ostringstream ss;
  ss << in.rdbuf ();
  return parse_makefs_string (ss.str (), path, out, err);
}

} // namespace qp
