#include <cstring>

#include "allocator.hh"

namespace Sling {

char *parse_string(char *input) {
  char *str = gAllocationManager->alloc((strlen(input) * 2) + 1);
  char *yp = input + 1;
  char *sp = str;
  int end = 0;
#define _APPEND_CHAR(_x) do {*sp++ = (_x); ++end; } while(0)
  // The first `"` after an `L"` string opener is part of the string value itself
  // due to lscript's broken parser.
  if (input[0] == 'L' && input[1] == '"') {
    _APPEND_CHAR('"');
    ++yp;
  }
  while (*yp) {
    if (*yp == '\\') {
      switch (*++yp) {
        case 'n':
          _APPEND_CHAR('\n');
          break;
        case 't':
          // Tab escape really means "four spaces" in LSL.
          for (int i = 0; i < 4; ++i) {
            _APPEND_CHAR(' ');
          }
          break;
        case '\\':
          _APPEND_CHAR('\\');
          break;
        case '"':
          _APPEND_CHAR('"');
          break;
        // should never be possible, but stop parsing if we hit a NUL immediately after a '\'
        case '\0':
          goto escaped_null;
        default:
          // "\f" just outputs "f"
          _APPEND_CHAR(*yp);
          break;
      }
      ++yp;
      escaped_null:;
    } else if (*yp == '"') {
      break;
    } else {
      _APPEND_CHAR(*yp++);
    }
  }
#undef _APPEND_CHAR
  str[end] = '\0';
  return str;
}
}
