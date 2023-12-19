package com.linelancer.game;
public class NativeLoader extends android.app.NativeActivity {
    static {
        System.loadLibrary("lancer");
    }
}
