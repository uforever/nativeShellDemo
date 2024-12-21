package com.zhang3.outer;

import android.app.Application;
import android.content.Context;


public class OuterApp extends Application {
    static {
        System.loadLibrary("inner");
    }

    @Override
    public native void onCreate();

    @Override
    public native void attachBaseContext(Context base);
}
