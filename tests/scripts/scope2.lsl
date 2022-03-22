llOwnerSay(string msg) { // $[E10026] $[E20009]
    llSay(0, msg);
}
test2() { // $[E20009]
    integer test; // $[E20009]
    {
        integer test; // $[E20001] $[E20009]
    }
}
integer test = 1;                       // $[E20009] unused
test() { }                              // $[E10001] already declared
default { state_entry() { test(); } }   // $[E10005] test is variable, not function
