key /*DOMAIN*/_0 = "MASA MUSIC SCRIPT";
list /*clips*/_4;
string /*total_time*/_17;
integer /*num_clips*/_b;
integer /*clip_playing*/_2;
integer /*clip_preloading*/_3;
integer /*preset_clips*/_10;
integer /*notecard_line*/_a;
integer /*disable_touch*/_7;
integer /*disable_text*/_6;
integer /*die_on_unlink*/_5;
/*say*/_14(string /*str*/_1a)
{
    llSay(0, /*str*/_1a);
}

string /*format_float*/_8(float /*num*/_1d, integer /*after_dec*/_1b, integer /*chop_dec*/_1c)
{
    string /*str*/_1f = "";
    list /*x*/_20 = llParseString2List((string)/*num*/_1d, ["."], []);
    /*str*/_1f += llList2String(/*x*/_20, 0);
    string /*decimal*/_1e = llList2String(/*x*/_20, 1);
    if ((integer)/*decimal*/_1e != 0 || !/*chop_dec*/_1c)
    {
        /*str*/_1f += ".";
        /*str*/_1f += llGetSubString(/*decimal*/_1e, 0, /*after_dec*/_1b - 1);
    }
    return /*str*/_1f;
}

/*set_text*/_16(string /*str*/_22, vector /*color*/_21)
{
    if (/*disable_text*/_6)
        return;
    llSetText(llGetObjectName() + "\n" + /*str*/_22, /*color*/_21, 1.00000);
}

integer /*check_control*/_1(integer /*num*/_23)
{
    integer /*i*/_24;
    if (/*disable_touch*/_7)
        return FALSE;
    if (1)
        return TRUE;
    for (/*i*/_24 = 0; /*i*/_24 < /*num*/_23; /*i*/_24++)
        if (llDetectedKey(/*i*/_24) == llGetOwner())
            return TRUE;
    return FALSE;
}

/*preload_next_clip*/_f(integer /*show_text*/_25)
{
    if (/*clip_preloading*/_3 < /*num_clips*/_b)
        llPreloadSound(llList2Key(/*clips*/_4, /*clip_preloading*/_3));
    if (/*show_text*/_25)
    {
        /*set_text*/_16("Preloading " + (string)(2 - /*clip_preloading*/_3) + " clip(s) " + "[" + /*format_float*/_8(4.50000 * (2 - /*clip_preloading*/_3), 1, 0) + " sec]\n" + "Click to start play immediately.", <0.00000, 0.00000, 1.00000>);
    }
    /*clip_preloading*/_3 += 1;
}

/*play_next_clip*/_c()
{
    llPlaySound(llList2Key(/*clips*/_4, /*clip_playing*/_2), 1.00000);
    /*clip_playing*/_2 += 1;
}

/*update_text*/_18()
{
    /*set_text*/_16("Playing: " + /*format_time*/_9((integer)llGetTime()) + "/" + /*total_time*/_17, <0.00000, 1.00000, 0.00000>);
}

string /*format_time*/_9(integer /*secs*/_26)
{
    return (string)((integer)(/*secs*/_26 / 60)) + ":" + llGetSubString("0" + (string)(/*secs*/_26 % 60), -2, -1);
}

/*send_message*/_15(integer /*msg*/_28, list /*data*/_27)
{
    llMessageLinked(LINK_SET, /*msg*/_28, llList2CSV(/*data*/_27), /*DOMAIN*/_0);
}

default
{
    state_entry()
    {
        if (1)
            llSetTextureAnim(FALSE, ALL_SIDES, 0, 0, 0, 0, 0);
        /*clip_playing*/_2 = 0;
        /*clip_preloading*/_3 = 0;
        /*num_clips*/_b = llGetListLength(/*clips*/_4);
        if (/*num_clips*/_b > 0)
        {
            /*preset_clips*/_10 = TRUE;
            /*total_time*/_17 = /*format_time*/_9((integer)(/*num_clips*/_b * 9.00000));
        }
        llStopSound();
        /*set_text*/_16("Stopped", <1.00000, 0.00000, 0.00000>);
        /*send_message*/_15(20100, []);
        if (llGetStartParameter() == 222646)
            /*die_on_unlink*/_5 = TRUE;
        else
            /*die_on_unlink*/_5 = FALSE;
    }

    on_rez(integer /*param*/_29)
    {
        state /*reset*/_13;
    }

    touch_start(integer /*num*/_2a)
    {
        if (/*check_control*/_1(/*num*/_2a))
        {
            if (/*preset_clips*/_10)
                state /*preload*/_e;
            else if (llGetInventoryKey("sounds") != NULL_KEY)
                state /*read_notecard*/_12;
            else if (llGetInventoryNumber(INVENTORY_SOUND) > 0)
                state /*read_inventory*/_11;
            else
                /*say*/_14("nothing to play!");
        }
    }

    link_message(integer /*sender*/_2e, integer /*msg*/_2d, string /*data*/_2b, key /*domain*/_2c)
    {
        if (/*domain*/_2c != /*DOMAIN*/_0)
            return;
        if (/*msg*/_2d == 10000)
        {
            if (/*preset_clips*/_10)
                state /*preload*/_e;
            else if (llGetInventoryKey("sounds") != NULL_KEY)
                state /*read_notecard*/_12;
            else if (llGetInventoryNumber(INVENTORY_SOUND) > 0)
                state /*read_inventory*/_11;
            else
                /*say*/_14("nothing to play!");
        }
        else if (/*msg*/_2d == 11000)
        {
            /*disable_touch*/_7 = (integer)/*data*/_2b;
        }
        else if (/*msg*/_2d == 12000)
        {
            /*disable_text*/_6 = (integer)/*data*/_2b;
            if (/*disable_text*/_6)
                llSetText("", <0.00000, 0.00000, 0.00000>, 0);
        }
    }

    changed(integer /*what*/_2f)
    {
        if (/*what*/_2f & CHANGED_LINK && llGetLinkNumber() == 0 && /*die_on_unlink*/_5)
            llDie();
    }
}
state /*reset*/_13
{
    state_entry()
    {
        /*disable_touch*/_7 = 0;
        /*disable_text*/_6 = 0;
        state default;
    }
}
state /*read_notecard*/_12
{
    state_entry()
    {
        /*notecard_line*/_a = 0;
        llGetNotecardLine("sounds", /*notecard_line*/_a++);
        /*send_message*/_15(10300, []);
        /*set_text*/_16("reading notecard", <0.00000, 0.00000, 1.00000>);
        llSetTimerEvent(5);
        /*clips*/_4 = [];
    }

    dataserver(key /*qid*/_31, string /*data*/_30)
    {
        if (/*data*/_30 == EOF)
        {
            /*num_clips*/_b = llGetListLength(/*clips*/_4);
            if (/*num_clips*/_b <= 0)
            {
                /*say*/_14("no clips");
                state default;
            }
            else
            {
                /*total_time*/_17 = /*format_time*/_9((integer)(/*num_clips*/_b * 9.00000));
                state /*preload*/_e;
            }
        }
        /*clips*/_4 += llCSV2List(/*data*/_30);
        llGetNotecardLine("sounds", /*notecard_line*/_a++);
        llResetTime();
    }

    timer()
    {
        if (llGetTime() > 5.00000)
        {
            /*say*/_14("dataserver timeout");
            state default;
        }
    }

    state_exit()
    {
        llSetTimerEvent(0);
    }

    changed(integer /*what*/_32)
    {
        if (/*what*/_32 & CHANGED_LINK && llGetLinkNumber() == 0 && /*die_on_unlink*/_5)
            llDie();
    }

    on_rez(integer /*param*/_33)
    {
        state /*reset*/_13;
    }

    link_message(integer /*sender*/_37, integer /*msg*/_36, string /*data*/_34, key /*domain*/_35)
    {
        if (/*domain*/_35 != /*DOMAIN*/_0)
            return;
        if (/*msg*/_36 == 10100)
            state default;
        else if (/*msg*/_36 == 11000)
            /*disable_touch*/_7 = (integer)/*data*/_34;
        else if (/*msg*/_36 == 12000)
            /*disable_text*/_6 = (integer)/*data*/_34;
    }
}
state /*read_inventory*/_11
{
    state_entry()
    {
        integer /*i*/_38;
        /*send_message*/_15(10300, []);
        /*set_text*/_16("reading inventory", <1.00000, 0.00000, 0.00000>);
        /*num_clips*/_b = llGetInventoryNumber(INVENTORY_SOUND);
        /*total_time*/_17 = /*format_time*/_9((integer)(/*num_clips*/_b * 9.00000));
        /*clips*/_4 = [];
        for (/*i*/_38 = 0; /*i*/_38 < /*num_clips*/_b; /*i*/_38++)
            /*clips*/_4 += [llGetInventoryName(INVENTORY_SOUND, /*i*/_38)];
        state /*preload*/_e;
    }

    changed(integer /*what*/_39)
    {
        if (/*what*/_39 & CHANGED_LINK && llGetLinkNumber() == 0 && /*die_on_unlink*/_5)
            llDie();
    }

    on_rez(integer /*param*/_3a)
    {
        state /*reset*/_13;
    }
}
state /*preload*/_e
{
    state_entry()
    {
        /*send_message*/_15(21000, [/*num_clips*/_b, 9.00000]);
        /*send_message*/_15(10200, []);
        /*preload_next_clip*/_f(TRUE);
        llSetTimerEvent(4.50000);
    }

    touch_start(integer /*num*/_3b)
    {
        if (/*check_control*/_1(/*num*/_3b))
            state /*playing*/_d;
    }

    timer()
    {
        if (/*clip_preloading*/_3 >= 2 || /*clip_preloading*/_3 >= /*num_clips*/_b)
            state /*playing*/_d;
        /*preload_next_clip*/_f(TRUE);
    }

    link_message(integer /*sender*/_3f, integer /*msg*/_3e, string /*data*/_3c, key /*domain*/_3d)
    {
        if (/*domain*/_3d != /*DOMAIN*/_0)
            return;
        if (/*msg*/_3e == 10000)
            state /*playing*/_d;
        else if (/*msg*/_3e == 10100)
            state default;
        else if (/*msg*/_3e == 11000)
            /*disable_touch*/_7 = (integer)/*data*/_3c;
        else if (/*msg*/_3e == 12000)
            /*disable_text*/_6 = (integer)/*data*/_3c;
    }

    state_exit()
    {
        llSetTimerEvent(0);
    }

    changed(integer /*what*/_40)
    {
        if (/*what*/_40 & CHANGED_LINK && llGetLinkNumber() == 0 && /*die_on_unlink*/_5)
            llDie();
    }

    on_rez(integer /*param*/_41)
    {
        state /*reset*/_13;
    }
}
state /*playing*/_d
{
    state_entry()
    {
        llSetSoundQueueing(TRUE);
        llSetTimerEvent(1);
        /*send_message*/_15(20000, []);
        llResetTime();
        if (1)
            llSetTextureAnim(35, ALL_SIDES, 0, 0, 0, TWO_PI, 30.0000);
        /*play_next_clip*/_c();
        /*preload_next_clip*/_f(FALSE);
        /*update_text*/_18();
        if (/*clip_playing*/_2 >= /*num_clips*/_b)
            state /*wind_down*/_19;
    }

    timer()
    {
        if ((integer)((llGetTime() + 1.00000) / 9.00000) >= /*clip_playing*/_2)
        {
            /*play_next_clip*/_c();
            /*preload_next_clip*/_f(FALSE);
            if (/*clip_playing*/_2 >= /*num_clips*/_b)
                state /*wind_down*/_19;
        }
        /*update_text*/_18();
    }

    touch_start(integer /*num*/_42)
    {
        if (/*check_control*/_1(/*num*/_42))
            state default;
    }

    link_message(integer /*sender*/_46, integer /*msg*/_45, string /*data*/_43, key /*domain*/_44)
    {
        if (/*domain*/_44 != /*DOMAIN*/_0)
            return;
        if (/*msg*/_45 == 10100)
            state default;
        else if (/*msg*/_45 == 11000)
            /*disable_touch*/_7 = (integer)/*data*/_43;
        else if (/*msg*/_45 == 12000)
            /*disable_text*/_6 = (integer)/*data*/_43;
    }

    state_exit()
    {
        llSetTimerEvent(0);
    }

    changed(integer /*what*/_47)
    {
        if (/*what*/_47 & CHANGED_LINK && llGetLinkNumber() == 0 && /*die_on_unlink*/_5)
            llDie();
    }

    on_rez(integer /*param*/_48)
    {
        state /*reset*/_13;
    }
}
state /*wind_down*/_19
{
    state_entry()
    {
        llSetTimerEvent(1);
    }

    timer()
    {
        if (llGetTime() >= (/*num_clips*/_b * 9.00000))
            state default;
        /*update_text*/_18();
    }

    touch_start(integer /*num*/_49)
    {
        if (/*check_control*/_1(/*num*/_49))
            state default;
    }

    link_message(integer /*sender*/_4d, integer /*msg*/_4c, string /*data*/_4a, key /*domain*/_4b)
    {
        if (/*domain*/_4b != /*DOMAIN*/_0)
            return;
        if (/*msg*/_4c == 10100)
            state default;
        else if (/*msg*/_4c == 11000)
            /*disable_touch*/_7 = (integer)/*data*/_4a;
        else if (/*msg*/_4c == 12000)
            /*disable_text*/_6 = (integer)/*data*/_4a;
    }

    state_exit()
    {
        llSetTimerEvent(0);
    }

    changed(integer /*what*/_4e)
    {
        if (/*what*/_4e & CHANGED_LINK && llGetLinkNumber() == 0 && /*die_on_unlink*/_5)
            llDie();
    }

    on_rez(integer /*param*/_4f)
    {
        state /*reset*/_13;
    }
}
