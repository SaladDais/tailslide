foo() {
    return llOwnerSay("illegal");  // $[E10018]
}

default {
    state_entry() {
        foo();
    }
}
