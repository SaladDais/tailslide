#ifndef LSLINT_STRINGS_HH
#define LSLINT_STRINGS_HH

namespace Tailslide {
class ScriptAllocator;

char *parse_string(ScriptAllocator *allocator, char *input);
std::string escape_string(const char *str);
}

#endif
