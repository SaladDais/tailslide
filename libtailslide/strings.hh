#ifndef LSLINT_STRINGS_HH
#define LSLINT_STRINGS_HH

#include "loctype.hh"

namespace Tailslide {
class ScriptAllocator;

char *parse_string(ScriptAllocator *allocator, char *input, YYLTYPE *lloc=nullptr);
std::string escape_string(const char *str);
}

#endif
