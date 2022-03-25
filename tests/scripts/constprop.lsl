// Constant propogation test

integer s = 2;

default {
    state_entry() {
        integer    i = 1;
        float      f = 1.0;
        vector     v = <1,2,3>;
        quaternion r = <3,2,1,0>;
        list       l = [1,2,3];
        if ( s == 2 ) return;                   // $[E20012]
        string     s = "hello";                 // $[E20001]

        if ( i == f ) return;                   // $[E20012]
        if ( i != f ) return;                   // $[E20013]
        if ( i == (f+1) ) return;               // $[E20013]
        if ( (i+1) == (f+1) ) return;           // $[E20012]
        if ( f == 1.0 ) return;                 // $[E20012]
        if ( f == 1.1 ) return;                 // $[E20013]

        if ( v == <1,2,3> ) return;             // $[E20012]
        if ( v == <r.z, r.y, r.x> ) return;     // $[E20012]
        if ( v.x == v.y ) return;               // $[E20013]
        if ( r == <v.z, v.y, v.x, 0> ) return;  // $[E20012]

        if ( l == [6,5,2] ) return;             // $[E20012] $[E20011]
        if ( l == [r.z, r.y, r.x] ) return;     // $[E20012] $[E20011]
        if ( l == [] ) return;                  // $[E20013]

        if ( s == "hello" ) return;             // $[E20012]
        if ( s + " world" == "h" + "e" + "llo" + " wo" + "rld" ) return;    // $[E20012]
        if ( i ^ 1 == 1 ) return;               // $[E20013]
        if ( ~i == 0xFFffFFfe ) return;         // $[E20012]
        if ( f == 1E0 ) return;                 // $[E20012]
        if ( f == 1.E0 ) return;                // $[E20012]
        if ( f == 0.1E1F ) return;              // $[E20012]
        if ( f == .1E1F ) return;               // $[E20012]
        if ( (string)1 == "1" ) return;         // $[E20012]
        if ( (string)1 == "0" ) return;         // $[E20013]
        if ( (integer)"0xF" == 15 ) return;     // $[E20012]
        if ( (integer)"077" == 77 ) return;     // $[E20012]
        if ( (integer)"foo" == 0 ) return;      // $[E20012]
        if ( (float)"0x1" == 1.0 ) return;      // $[E20012]
        if ( (float)((float)"1.0") == 1.0 ) return;      // $[E20012]
    }
}
