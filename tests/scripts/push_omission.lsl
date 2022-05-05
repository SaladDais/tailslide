default {
    state_entry() {
        integer i = 1;
        integer j = 2;
        // the dup / pop for the result of this assignment may be omitted
        i = 3;
        // but not for the = in the subexpression of this one!
        j = i = 4;
        // we can't omit the pop because the pushing of the retval is controlled by the callee
        llFrand(1.0);
    }
}
