//+----------------------------------------------------------------------------+
//|                                                                                                                                          |
//| Copyright(c) 2011, Dmitry "PVOID" Petuhov                                  |
//| All rights reserved.                                                       |
//|                                                                            |
//| Redistribution and use in source and binary forms, with or without         |
//| modification, are permitted provided that the following conditions are     |
//| met:                                                                       |
//|                                                                            |
//|   - Redistributions of source code must retain the above copyright notice, |
//|     this list of conditions and the following disclaimer.                  |
//|   - Redistributions in binary form must reproduce the above copyright      |
//|     notice, this list of conditions and the following disclaimer in the    |
//|     documentation and / or other materials provided with the distribution. |
//|                                                                            |
//| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS        |
//| "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED  |
//| TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR |
//| PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR          |
//| CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,      |
//| EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,         |
//| PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR         |
//| PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF     |
//| LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING        |
//| NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         |
//| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               |
//|                                                                            |
//+----------------------------------------------------------------------------+
#include "../../src/crashpin.h"
#include "../../src/dumbvector.h"
#include <jni.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <android/log.h>

typedef void (TestFunc)();

class A
{
public:
  //+----------------------------------------------------------------------------+
  //|                                                                            |
  //+----------------------------------------------------------------------------+
  static void crash()
  {
    *(int *)0 = 10;
    /*int a = 0xdeadbeaf;
    (*(TestFunc*)a)(); // Бабах!!!!*/
  }
  //+----------------------------------------------------------------------------+
  //|                                                                            |
  //+----------------------------------------------------------------------------+
  static void sayC()
  {
    crash();
  }
  //+----------------------------------------------------------------------------+
  //|                                                                            |
  //+----------------------------------------------------------------------------+
  static void sayB()
  {
    sayC();
  }
  //+----------------------------------------------------------------------------+
  //|                                                                            |
  //+----------------------------------------------------------------------------+
  static void sayA()
  {
    sayB();
  }
};

//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
extern "C" jint JNI_OnLoad(JavaVM *vm, void* /*reserved*/)
{
  //SCrashPin::Initialize();
  return JNI_VERSION_1_6;
}

int compare(const void *left, const void* right)
{
  int l = *(int *)left;
  int r = *(int *)right;
  return(l-r);
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
extern "C" void Java_com_github_pvoid_crashpin_TestActivity_doCrash(JNIEnv *env, jobject self)
{
  //A::sayA();
  TDumbVector<int> data;
  srand(time(NULL));
  for(size_t index=0;index<100;++index)
    data.Add(rand()%10000);
  data.Sort(compare);
  for(size_t index=0;index<100;++index)
    __android_log_print(ANDROID_LOG_DEBUG,"TEST","Item: %d",data[index]);
}
//+----------------------------------------------------------------------------+
