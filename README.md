# Intro

libSLing provides a parser, AST representation, and tree walker for
Second Life's Linden Scripting Language.

Also provided is a CLI utility to quickly lint or optimize LSL scripts,
as well as visualize the AST of the supplied script.

## Build

## Linux & OSX

libSLing is only tested on Linux & OS X, and requires `cmake`, `flex` and `bison`.

```bash
git clone https://github.com/SaladDais/libSLing.git
cd libSLing
mkdir build
cd build
cmake ..
make
```

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

Obviously, there will be some differences, mostly around floating point operations.

## Optimizations

### Constant Folding

The naive approach is taken. libSLing makes multiple walks down the tree checking if an expression is constant, and replaces
it with its value.

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
To reduce bytecode size, libSLing can mangle user-defined symbols into more compact ones like
`_a`, `_b`, etc. This may also help with disambiguation when converting the AST to another
language with different scoping / variable shadowing rules.


# License
MIT

# Credits
libSLing is heavily based on [lslint](https://github.com/pclewis/lslint) by pclewis
