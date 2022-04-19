default {
    state_entry() {
        if (llFrand(1 + 1.))
            return;
        else
            return;
        // no return should be injected here!
    }
}
