# Intro

![Build Status](https://github.com/SaladDais/tailslide/workflows/build/badge.svg) [![codecov](https://codecov.io/gh/SaladDais/tailslide/branch/master/graph/badge.svg?token=5BQCSA63F1)](https://codecov.io/gh/SaladDais/tailslide)


Tailslide provides an embeddable parser, AST representation, and tree walker library for
Second Life's Linden Scripting Language. These can be used as the foundation for an
LSL compiler or a superset of LSL.

Also provided is a CLI utility to quickly lint or optimize LSL scripts,
as well as visualize the AST of the supplied script.

If you're mainly interested in a command-line linting or optimization utility,
please see [lslint](https://github.com/Makopo/lslint) or
[LSL-PyOptimizer](https://github.com/Sei-Lisa/LSL-PyOptimizer), respectively.

# Credits
Tailslide is heavily based on the [lslint](https://github.com/pclewis/lslint)
LSL linter written by pclewis, see the [NOTICE.txt](NOTICE.txt) file for the
original README and credits.

# Build

## Linux & OSX

`cmake`, `flex` and `bison` must be installed through your system's package manager.
On OS X you _must_ install the Homebrew versions of `flex` and `bison` because the
versions provided with XCode are extremely old.

```bash
git clone https://github.com/SaladDais/tailslide.git
cd tailslide
mkdir build
cd build
cmake ..
make
```

# Windows

Not tested, may be compilable in an [MSYS2](https://www.msys2.org/) environment.

# Tech Overview

## Parsing

The parser behaves more or less like the one on SL's servers, quirks and all.

For example:
```
default { $$$$
    '' state_entry() {
        llOwnerSay(L"Hello
world!");
    }"
}/*{
```

is completely handled, and equivalent to

```
default {
    state_entry() {
        llOwnerSay("\"Hello\nworld!");
    }
}
```

Special attention has been paid to all the weird, undocumented corner cases.

## Optimizations

### Constant Folding

The constant folding implementation is currently very simple. Tailslide makes multiple
walks down the tree checking if an expression is constant, and replaces it with its value.

```
integer foo = 2;
foo = foo + 2 + 4 + 3 + 5;
```

won't currently be simplified at all due to how expressions are represented and will
require term rewriting to move non-constant parts of expressions as far to the right (left) as possible.

### Unused Variable / Function Pruning

Any variable or function that is unused after constant folding may be pruned to save bytecode space.

### Symbol Mangling

Names of user defined functions, their parameters and globals contribute to bytecode size.
To reduce bytecode size, Tailslide can mangle user-defined symbols into more compact ones like
`_a`, `_b`, etc. This may also help with disambiguation when converting the AST to another
language with different scoping / variable shadowing rules.

This isn't the most efficient naming scheme if targeting SL if targeting SL in particular
since it's better to take advantage of strings already in the constant pool.

# License
MIT, scripts used for test data (barring those added by me and those in `bugs/`) are
property of their respective owners
