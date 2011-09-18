package com.github.pvoid.crashpin;

import android.app.Activity;
import android.os.Bundle;

public class TestActivity extends Activity
{
  static
  {
    System.loadLibrary("crashpin");
  }

  Thread _mThread = new Thread()
  {
    @Override
    public void run()
    {
      doCrash();
    }
  };

  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    doCrash();
    _mThread.start();
  }

  private native void doCrash();
}