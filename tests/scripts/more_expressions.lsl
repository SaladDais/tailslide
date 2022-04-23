float no_implicit_cast_func() {
    // this is broken under LSO because it doesn't cast and re-interprets as float.
    return 3;
}

default {
    state_entry() {
        vector v = <1, 2, 3>;
        rotation r = <1, 2, 3, 4>;
        list l = [v, r, ""];  // $[E20009]
        integer explicit_cast = (integer)2.0;  // $[E20009]
        float implicit_cast = 2;  // $[E20009]
        float f = no_implicit_cast_func();  // $[E20009]
    }
}
