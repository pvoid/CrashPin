//+----------------------------------------------------------------------------+
//|                                                                            |
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
#include "crashpin.h"
#include <unistd.h>
#include <sys/wait.h>
#include <android/log.h>
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
#ifndef NULL
#define NULL 0
#endif
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
#ifndef PR_SET_DUMPABLE
#define PR_SET_DUMPABLE   4
#endif
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
bool SCrashPin::Initialize()
{
  struct sigaction handler;
///////
  handler.sa_sigaction = SigactionHandler;
  handler.sa_flags     = SA_SIGINFO;
///////
// We need make process dumpable
///////
  if(prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0)
  {
    __android_log_write(ANDROID_LOG_FATAL,"CrashPin","Can't set process dumpable");
    return(false);
  }
///////
// TODO: Save old handlers
//////
  sigaction(SIGSEGV, &handler, NULL);
  sigaction(SIGILL, &handler, NULL);
  sigaction(SIGSTKFLT, &handler, NULL);
  sigaction(SIGABRT, &handler, NULL);
  sigaction(SIGBUS, &handler, NULL);
  sigaction(SIGFPE, &handler, NULL);
//////
  return(true);
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
void SCrashPin::SigactionHandler(int sig, siginfo* info, void* reserved)
{
  pid_t pid = getpid();
  pid_t tid = gettid();
////////
  switch(fork())
  {
    case -1:
      kill(getpid(), SIGKILL);
      return;
    case 0:
      Mosquito(pid,tid);
      break;
    default:
      wait(NULL);
      break;
  }
////////
  kill(getpid(),SIGKILL);
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
void SCrashPin::Mosquito(pid_t pid, pid_t tid)
{
  __android_log_print(ANDROID_LOG_ERROR,"DEBUG","We crashed. Pid: %d, tid: %d. Self pid: %d",pid,tid,getpid());
}
//+----------------------------------------------------------------------------+
