#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "util/exception.h"
#include "util/inifile.h"

UTIL_NAMESPACE_BEGIN

INISectionPtr
INISection::get_section (std::string const& key)
{
  size_t dot_pos = key.find_first_of('.');
  if (dot_pos == std::string::npos)
  {
    INISectionsType::iterator iter = this->sections.find(key);
    if (iter == this->sections.end())
      throw Exception("Section does not exist");
    else
      return iter->second;
  }
  else
  {
    std::string section = key.substr(0, dot_pos);
    std::string new_key = key.substr(dot_pos + 1);
    INISectionsType::iterator iter = this->sections.find(section);

    if (iter == this->sections.end())
      throw Exception("INI: " + key + ": Path to section does not exist");
    else
      return iter->second->get_section(new_key);
  }
}

/* ---------------------------------------------------------------- */

INIValuePtr
INISection::get_value (std::string const& key)
{
  INIValuesType::iterator iter = this->values.find(key);
  if (iter != this->values.end())
    return iter->second;

  size_t dot_pos = key.find_first_of('.');
  if (dot_pos == std::string::npos)
  {
    throw Exception("Key '" + key + "' does not exist");
  }
  else
  {
    std::string section = key.substr(0, dot_pos);
    std::string new_key = key.substr(dot_pos + 1);
    INISectionsType::iterator iter = this->sections.find(section);
    if (iter == this->sections.end())
      throw Exception("Path to key '" + key + "' does not exist");
    else
      return iter->second->get_value(new_key);
  }
}

/* ---------------------------------------------------------------- */

void
INISection::to_stream (std::ostream& outstr, std::string prefix)
{
  if (!this->do_stream)
    return;

  /* Process each value in this section. */
  {
    INIValuesType::iterator iter = this->values.begin();
    while (iter != this->values.end())
    {
      if (iter->first.size() == 0)
        outstr << "  " << "= ";
      else
        outstr << "  " << iter->first << " = ";
      outstr << iter->second->get<std::string>() << std::endl;
      iter++;
    }
    /* Print newline after each non-empty section. */
    if (this->values.size() > 0)
      outstr << std::endl;
  }

  /* Process each subsection. */
  {
    INISectionsType::iterator iter = sections.begin();
    while (iter != this->sections.end())
    {
      if (prefix.size() == 0)
      {
        outstr << "[" << iter->first << "]" << std::endl;
        iter->second->to_stream(outstr, iter->first);
      }
      else
      {
        outstr << "[" << prefix << "." << iter->first << "]" << std::endl;
        iter->second->to_stream(outstr, prefix + "." + iter->first);
      }
      iter++;
    }
  }
}

/* ================================================================ */

INIFile::INIFile (void)
{
  this->clear();
}

/* ---------------------------------------------------------------- */
/* Returns value or throws if value does not exist. */

INIValuePtr
INIFile::get_value (std::string const& key)
{
  try
  { return root->get_value(key); }
  catch (Exception& e)
  {
    throw Exception(e + ": " + key);
  }
}

/* ---------------------------------------------------------------- */
/* Returns the section or throws if section does not exist. */

INISectionPtr
INIFile::get_section (std::string const& key)
{
  try
  { return root->get_section(key); }
  catch (Exception& e)
  {
    throw Exception(e + ": " + key);
  }
}

/* ---------------------------------------------------------------- */
/* Returns section or creates a new if section does not exist. */

INISectionPtr
INIFile::get_or_create_section (std::string const& key)
{
  /* Try to find already existing section. */
  try
  { return this->get_section(key); }
  catch(Exception& e)
  { }

  /* Section does not exist. */
  //std::cout << "INI: Creating section " << key << std::endl;
  size_t dot_pos = key.find_last_of('.');
  if (dot_pos == 0 || dot_pos == key.size() - 1)
  {
    /* Report syntax error. */
    throw Exception("Syntax error: Invalid section name");
  }
  else if (dot_pos == std::string::npos)
  {
    /* Add new section to the root. */
    INISectionPtr new_section = INISection::create();
    this->root->add(key, new_section);
    return new_section;
  }
  else
  {
    /* Try to get parent section. */
    INISectionPtr parent;
    std::string parent_name = key.substr(0, dot_pos);
    std::string section_name = key.substr(dot_pos + 1);
    try
    {
      parent = this->get_or_create_section(parent_name);
      INISectionPtr new_section = INISection::create();
      parent->add(section_name, new_section);
      return new_section;
    }
    catch (Exception& e)
    {
      /* Parent section not found. */
      throw Exception("Semantic error: Parent section not found");
    }
  }
}

/* ---------------------------------------------------------------- */

void
INIFile::clear (void)
{
  this->root = INISection::create();
}

/* ---------------------------------------------------------------- */

void
INIFile::add_from_file (std::string const& filename)
{
  std::ifstream instr(filename.c_str());

  if (!instr.good())
  {
    throw Exception("Cannot open " + filename + ": "
        + std::string(strerror(errno)));
  }

  if (instr.peek() < 0)
  {
    throw Exception("Cannot open " + filename + ": "
        "Peek failed!");
  }

  //std::cout << "INI: Start parsing of " << filename << std::endl;
  this->add_from_file(instr);
  //std::cout << "INI: Parsing done" << std::endl;

  instr.close();
}

/* ---------------------------------------------------------------- */

void
INIFile::add_from_file (std::istream& instr)
{
  std::string buf;
  int buf_line = 0;
  INISectionPtr cur_section;
  while (!instr.eof() && instr.good())
  {
    std::getline(instr, buf);
    buf_line++;

    /* Clip string buffer. */
    size_t comment_pos = buf.find_first_of('#');
    if (comment_pos != std::string::npos)
      buf = buf.substr(0, comment_pos);

    util::string::clip(buf);

    if (buf.size() == 0)
      continue;

    /* Analyse content line. */
    if (buf[0] == '[' && buf[buf.size() - 1] == ']')
    {
      /* It's a new section. Clear current section. */
      cur_section = 0;

      /* Get the new sections name. */
      std::string section_name = buf.substr(1, buf.size() - 2);
      util::string::clip(section_name);
      if (section_name.size() == 0)
      {
        std::cout << "INI: Line " << buf_line << ": Syntax error: "
            << "No config name given" << std::endl;
        continue;
      }

      /* Try to get or insert the section. */
      try
      { cur_section = this->get_or_create_section(section_name); }
      catch (Exception& e)
      {
        std::cout << "INI: Line " << buf_line << ": " << e << std::endl;
        continue;
      }

      //std::cout << "Section inserted: " << section_name << std::endl;
    }
    else
    {
      /* There must be a current section for the value. */
      if (cur_section == 0)
      {
        std::cout << "INI: Line " << buf_line << ": Semantic error: "
            << "Cannot insert value to invalid section" << std::endl;
        continue;
      }
      size_t separator_pos = buf.find_first_of('=');
      if (separator_pos == std::string::npos)
      {
        std::cout << "INI: Line " << buf_line << ": Syntax error: "
            << "No key/value delimiter found" << std::endl;
        continue;
      }
      /* Extract key and value. */
      std::string key = buf.substr(0, separator_pos);
      std::string val = buf.substr(separator_pos + 1);
      util::string::clip(key);
      util::string::clip(val);
      /* Insert key-value pair. */
      INIValuePtr value = INIValue::create(val);
      cur_section->add(key, value);

      //std::cout << "INI: Value inserted: " << key
      //    << "=" << val << std::endl;
    }
  }
  //std::cout << "INI: Done parsing" << std::endl;
}

/* ---------------------------------------------------------------- */

void
INIFile::add_from_string (std::string const& conf_string)
{
  std::stringstream conf_file;
  conf_file << conf_string;
  this->add_from_file(conf_file);
}

/* ---------------------------------------------------------------- */

void
INIFile::to_file (std::string const& filename)
{
  //std::cout << "Saving configuration..." << std::endl;

  std::ofstream file(filename.c_str());
  if (file.fail())
  {
    //std::cout << "INI: Could not open " << filename << ": "
    //    << strerror(errno) << std::endl;
    throw Exception((filename + ": ") + std::strerror(errno));
  }

  /* Print config to file. */
  this->root->to_stream(file);
  file.close();
}

/* ---------------------------------------------------------------- */

void
INIFile::to_stream (std::ostream& outstr)
{
  this->root->to_stream(outstr);
}

UTIL_NAMESPACE_END
