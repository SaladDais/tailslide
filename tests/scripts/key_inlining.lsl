key foo = "foobar";
list bar = [foo];

default {
    state_entry() {
        list baz = (list)foo;
        print(foo);
        print(bar);
        print(baz);
        if (foo) ; // $[E20007]
        while (foo) ; // $[E20014]
        for (foo,foo;foo;foo) ;  // $[E20014]
        llGetOwnerKey(foo);
    }
}
