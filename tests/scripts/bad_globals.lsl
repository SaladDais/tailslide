// different kinds of bad declarations that generate error tokens
// and have unique parser branches
string foo = 1+1;
string baz = "";
string quux = ;

default {
    state_entry() {
        llOwnerSay(baz);
        <;
    }
}
