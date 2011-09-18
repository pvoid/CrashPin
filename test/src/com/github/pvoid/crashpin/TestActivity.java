package com.github.pvoid.crashpin;

import android.app.Activity;
import android.os.Bundle;

public class TestActivity extends Activity
{
  static
  {
    System.loadLibrary("crashpin");
  }

  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    doCrash();
  }

  private native void doCrash();
}