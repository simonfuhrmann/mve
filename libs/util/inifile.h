/*
 * INI file reader/writer with support for hierarchical sections.
 * Written by Simon Fuhrmann.
 */

#ifndef UTIL_INI_FILE_HEADER
#define UTIL_INI_FILE_HEADER

#include <string>
#include <map>

#include "defines.h"
#include "string.h"
#include "refptr.h"

UTIL_NAMESPACE_BEGIN

class INISection;
class INIValue;
typedef RefPtr<INISection> INISectionPtr;
typedef RefPtr<INIValue> INIValuePtr;
typedef std::map<std::string, INISectionPtr> INISectionsType;
typedef std::map<std::string, INIValuePtr> INIValuesType;

/* ---------------------------------------------------------------- */

/**
 * A single value for the INI reader system.
 * Changing the value is directly reflected in the INI hierarchy.
 */
class INIValue
{
private:
    std::string value;

protected:
    INIValue (void) {}

public:
    /** Creates a new empty INI value. */
    static INIValuePtr create (void);
    /** Creates a new valued INI value. */
    static INIValuePtr create (std::string const& _value);

    /** Returns type-converted INI value. */
    template <typename T>
    T get (void) const;

    /** Sets INI value from any type. */
    template <typename T>
    void set (T const& value);

    /** Returns INI value reference. */
    std::string& operator* (void);
    /** Returns INI value const-reference. */
    std::string const& operator* (void) const;
};

/* ---------------------------------------------------------------- */

/**
 * A section for the INI reader system.
 * Sections may contain both, values and sections, thus the INI system is
 * recursively defined. Different levels of the hierarchy are separated
 * by a period, e.g. "mysection.myelement" refers to section OR value
 * "myelement" within section "mysection".
 */
class INISection
{
private:
    INISectionsType sections;
    INIValuesType values;
    bool do_stream;

protected:
    INISection (void);

public:
    /** Creates a new empty config section. */
    static INISectionPtr create (void);

    /** Adds a config section to the section. */
    void add (std::string const& key, INISectionPtr section);
    /** Adds a config value to the section. */
    void add (std::string const& key, INIValuePtr value);

    /** Returns a config section by key or throws. */
    INISectionPtr get_section (std::string const& key);
    /** Returns a config value by key or throws. */
    INIValuePtr get_value (std::string const& key);

    /** Deletes a config section by key. */
    void remove_section (std::string const& key);
    /** Deletes a config value by key. */
    void remove_value (std::string const& key);

    /** Returns begin-iterator for the values. */
    INIValuesType::iterator values_begin (void);
    /** Returns end-iterator for the values. */
    INIValuesType::iterator values_end (void);
    /** Returns begin-iterator for the sections. */
    INISectionsType::iterator sections_begin (void);
    /** Returns end-iterator for the sections. */
    INISectionsType::iterator sections_end (void);

    /** Returns value directly available in this section or throws. */
    INIValuesType::iterator find_value (std::string const& key);
    /** Returns section directly available in this section or throws. */
    INISectionsType::iterator find_section (std::string const& key);

    /** Deletes all values in this section. */
    void clear_values (void);
    /** Deletes all sections in this section. */
    void clear_sections (void);

    /** Recursively writes this section to the given stream. */
    void to_stream (std::ostream& outstr, std::string prefix = "");

    /** Sets a flag to specify whether this section should be streamed. */
    void set_do_stream (bool do_stream);
};

/* ---------------------------------------------------------------- */

/**
 * Frontend to the INI reader system.
 * A recursively defined hierarchy of sections and values
 * may be build up manually or by using the add_from_file()
 * methods. Both sections and values are referenced by keys.
 */
class INIFile
{
private:
    INISectionPtr root;

public:
    /** Creates an empty root section. */
    INIFile (void);

    /** Returns a value for an hierarchical key. */
    INIValuePtr get_value (std::string const& key);
    /** Returns a section for an hierarchical key. */
    INISectionPtr get_section (std::string const& key);
    /** Returns a (possibly just created) section for a key. */
    INISectionPtr get_or_create_section (std::string const& key);

    /** Clears the hierarchy by creating a new empty root section. */
    void clear (void);

    /** Parses the given stream and adds content to the hierarchy. */
    void add_from_file (std::istream& instr);
    /** Parses the given file and adds content to the hierarchy. */
    void add_from_file (std::string const& filename);
    /** Parses the given string and adds content to the hierarchy. */
    void add_from_string (std::string const& conf_string);

    /** Writes the current hierarchy to file. */
    void to_file (std::string const& filename);
    /** Writes the current hierarchy to stream. */
    void to_stream (std::ostream& outstr);
};

/* ---------------------------------------------------------------- */

inline INIValuePtr
INIValue::create (void)
{
    return INIValuePtr(new INIValue);
}

inline INIValuePtr
INIValue::create (std::string const& value)
{
    INIValuePtr ret(new INIValue);
    ret->set(value);
    return ret;
}

template <typename T>
inline T
INIValue::get (void) const
{
    return util::string::convert<T>(this->value);
}

template <>
inline bool
INIValue::get<bool> (void) const
{
    return this->value == "true" || this->value == "1";
}

template <typename T>
inline void
INIValue::set (T const& value)
{
    this->value = util::string::get(value);
}

template <>
inline void
INIValue::set<bool> (bool const& value)
{
    this->value = (value ? "true" : "false");
}

inline std::string const&
INIValue::operator* (void) const
{
    return this->value;
}

inline std::string&
INIValue::operator* (void)
{
    return this->value;
}

inline
INISection::INISection (void)
    : do_stream(true)
{
}

inline INISectionPtr
INISection::create (void)
{
    return INISectionPtr(new INISection);
}

inline void
INISection::add (std::string const& key, INISectionPtr section)
{
    this->sections[key] = section;
}

inline void
INISection::add (std::string const& key, INIValuePtr value)
{
    this->values[key] = value;
}

inline INIValuesType::iterator
INISection::values_begin (void)
{
    return this->values.begin();
}

inline INIValuesType::iterator
INISection::values_end (void)
{
    return this->values.end();
}

inline INISectionsType::iterator
INISection::sections_begin (void)
{
    return this->sections.begin();
}

inline INISectionsType::iterator
INISection::sections_end (void)
{
    return this->sections.end();
}

inline INIValuesType::iterator
INISection::find_value (std::string const& key)
{
    return this->values.find(key);
}

inline INISectionsType::iterator
INISection::find_section (std::string const& key)
{
    return this->sections.find(key);
}

inline void
INISection::clear_values (void)
{
    this->values.clear();
}

inline void
INISection::clear_sections (void)
{
    this->sections.clear();
}

inline void
INISection::remove_section (std::string const& key)
{
    this->sections.erase(key);
}

inline void
INISection::remove_value (std::string const& key)
{
    this->values.erase(key);
}

inline void
INISection::set_do_stream (bool do_stream)
{
    this->do_stream = do_stream;
}

UTIL_NAMESPACE_END

#endif /* UTIL_INI_FILE_HEADER */
