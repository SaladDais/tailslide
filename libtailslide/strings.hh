#ifndef LSLINT_STRINGS_HH
#define LSLINT_STRINGS_HH

namespace Tailslide {
class ScriptAllocationManager;

char *parse_string(ScriptAllocationManager *allocator, char *input);
std::string escape_string(const char *str);
}

#endif
