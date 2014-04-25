/*
 * Simple string tokenizers.
 * Written by Simon Fuhrmann.
 */

#ifndef UTIL_TOKENIZE_HEADER
#define UTIL_TOKENIZE_HEADER

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Simple tokenizer. Reads an input string and splits the string
 * according to some rules. The individual tokens are then stored
 * in the vector baseclass.
 */
class Tokenizer : public std::vector<std::string>
{
public:
    /**
     * Very simple tokenziation at a given delimiter characater.
     * Two subsequent delimiter characters lead to an empty token.
     */
    void split (std::string const& str, char delim = ' ');

    /**
     * A tokenizer that parses shell commands into tokens.
     * It handles quotes gracefully, placing quoted strings into
     * a single token and removes the quotes.
     */
    void parse_cmd (std::string const& str);

    /**
     * Concatenates 'num' tokens with a space character
     * starting from token at position 'pos'. Passing '0' as
     * 'num' argument means to concat all remaining tokens.
     */
    std::string concat (std::size_t pos, std::size_t num = 0);
};

/* ---------------------------------------------------------------- */

inline void
Tokenizer::split (std::string const& str, char delim)
{
    this->clear();
    std::size_t last = 0;
    std::size_t cur = 0;
    for (; cur < str.size(); ++cur)
        if (str[cur] == delim)
        {
            std::string token = str.substr(last, cur - last);
            if (!token.empty())
                this->push_back(token);
            last = cur + 1;
        }

    if (last < str.size())
        this->push_back(str.substr(last));
}

/* ---------------------------------------------------------------- */

inline void
Tokenizer::parse_cmd (std::string const& str)
{
    this->clear();
    bool in_quote = false;
    std::string token;
    for (std::size_t i = 0; i < str.size(); ++i)
    {
        char chr = str[i];

        if (chr == ' ' && !in_quote)
        {
            this->push_back(token);
            token.clear();
        }
        else if (chr == '"')
            in_quote = !in_quote;
        else
            token.push_back(chr);
    }
    this->push_back(token);
}

/* ---------------------------------------------------------------- */

inline std::string
Tokenizer::concat (std::size_t pos, std::size_t num)
{
    std::stringstream ss;
    std::size_t max = (num == 0
        ? this->size()
        : std::min(pos + num, this->size()));

    for (std::size_t i = pos; i < max; ++i)
    {
        ss << (i == pos ? "" : " ");
        ss << this->at(i);
    }

    return ss.str();
}

UTIL_NAMESPACE_END

#endif /* UTIL_TOKENIZE_HEADER */
