// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class str_base;

//------------------------------------------------------------------------------
enum class match_type : unsigned char
{
    do_not_use,     // complete.c relies on the type never being 0, so it can use savestring().
    none,           // Behaves like dir if match ends with path sep, otherwise like file.
    word,           // Matches and displays the whole word even if it contains slashes.
    arg,            // Prevents appending a space if the match ends with a colon or equal sign.
    alias,          // Displays match using the alias color.
    file,           // Displays match using the file color and only displays the last path component.
    dir,            // Displays match using the directory color, only displays the last path component, and adds a trailing path separator.
    link,           // Displays match using the symlink color and only displays the last path component.
    mask = 0x0f,
    hidden = 0x40,  // Displays file/dir/link matches using the hidden color.
    readonly = 0x80, // Displays file/dir/link matches using the readonly color.
};

DEFINE_ENUM_FLAG_OPERATORS(match_type);

//------------------------------------------------------------------------------
inline bool is_pathish(match_type type)
{
    type &= match_type::mask;
    return type == match_type::file || type == match_type::dir || type == match_type::link;
}



//------------------------------------------------------------------------------
class shadow_bool
{
public:
                            shadow_bool(bool default_value) : m_default(default_value) { reset(); }

    operator                bool() const { return get(); }
    bool                    operator=(bool) = delete;

    void                    reset() { m_has_explicit = false; m_explicit = false; m_implicit = m_default; }
    void                    set_explicit(bool value) { m_explicit = value; m_has_explicit = true; }
    void                    set_implicit(bool value) { m_implicit = value; }
    bool                    get() const { return m_has_explicit ? m_explicit : m_implicit; }
    bool                    is_explicit() const { return m_has_explicit; }

private:
    bool                    m_has_explicit : 1;
    bool                    m_explicit : 1;
    bool                    m_implicit : 1;
    bool                    m_default : 1;
};



//------------------------------------------------------------------------------
class matches
{
public:
    virtual unsigned int    get_match_count() const = 0;
    virtual const char*     get_match(unsigned int index) const = 0;
    virtual match_type      get_match_type(unsigned int index) const = 0;
    virtual bool            is_suppress_append() const = 0;
    virtual shadow_bool     is_filename_completion_desired() const = 0;
    virtual bool            is_filename_display_desired() const = 0;
    virtual char            get_append_character() const = 0;
    virtual int             get_suppress_quoting() const = 0;
    virtual int             get_word_break_adjustment() const = 0;
};



//------------------------------------------------------------------------------
match_type to_match_type(int mode, int attr);
match_type to_match_type(const char* type_name);

//------------------------------------------------------------------------------
struct match_desc
{
    const char*             match;          // Match text.
    match_type              type;           // Match type.
};

//------------------------------------------------------------------------------
class match_builder
{
public:
                            match_builder(matches& matches);
    bool                    add_match(const char* match, match_type type);
    bool                    add_match(const match_desc& desc);
    void                    set_append_character(char append);
    void                    set_suppress_append(bool suppress=true);
    void                    set_suppress_quoting(int suppress=1); //0=no, 1=yes, 2=suppress end quote

    void                    set_matches_are_files(bool files=true);

private:
    matches&                m_matches;
};