#pragma once

namespace Tailslide {

typedef struct TailslideLType TAILSLIDE_LTYPE;
struct TailslideLType {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

// keep existing consumers inside tailslide working
using YYLTYPE = TailslideLType;

}
# define TAILSLIDE_LTYPE_IS_DECLARED 1
# define TAILSLIDE_LTYPE_IS_TRIVIAL 1
