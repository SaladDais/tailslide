key foo = "foobar";
list bar = [foo];
default
{
    state_entry()
    {
        list baz = (list)foo;
        print(foo);
        print(bar);
        print(baz);
        if (foo)
            ;
        while (foo)
            ;
        for (foo, foo; foo; foo)
            ;
        llGetOwnerKey("foobar");
    }
}
