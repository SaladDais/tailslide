#pragma once

namespace Tailslide {

typedef struct TAILSLIDE_LTYPE TAILSLIDE_LTYPE;
struct TAILSLIDE_LTYPE {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

// keep existing consumers inside tailslide working
using YYLTYPE = TAILSLIDE_LTYPE;

}
# define TAILSLIDE_LTYPE_IS_DECLARED 1
# define TAILSLIDE_LTYPE_IS_TRIVIAL 1
