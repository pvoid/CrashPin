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
#include "stacktrace.h"
#include <android/log.h>
#include <string.h>
#include <strings.h>
//+----------------------------------------------------------------------------+
//| Constructor                                                                |
//+----------------------------------------------------------------------------+
CStackTrace::CStackTrace(pid_t tid) : m_map(tid), m_stack_first(NULL), m_stack_last(NULL)
{
  _uw pc;
    
  if(ptrace(PTRACE_GETREGS, tid, 0, &m_regs)!=0)
     memset(&m_regs,0,sizeof(m_regs));
  pc = m_regs.ARM_pc;
 ///////// Checking if app is failed because it called trash
  if(m_map.GetEntry(pc)==NULL)
  {
    AddStackRecord(m_regs.ARM_pc,"<trash>","<trash>");
////////// We'll try to get stacktrace from caller address
    pc = m_regs.ARM_lr;
  }
  
}
//+----------------------------------------------------------------------------+
//| Add new record to stack list                                              |
//+----------------------------------------------------------------------------+
bool CStackTrace::AddStackRecord(_uw address, const char* function_name, const char* module_name)
{
  StackItem *item;
  uint mname_length;
////////// Cheking for parameters  
  if(function_name==NULL || module_name==NULL)
    return false;
  mname_length = strlen(module_name);
////////// Alloating memmory and saving params  
  item = (StackItem *)malloc(sizeof(StackItem)+mname_length+1);
  bzero(item,sizeof(StackItem)+mname_length+1);
//////////  
  item->address = address;
  strncpy(item->function,function_name,sizeof(item->function));
  strcpy(item->module,module_name);
////////// Saving line  
  if(m_stack_first==NULL)
    m_stack_first = m_stack_last = item;
  else
  {
    m_stack_last->next = item;
    m_stack_last = item;
  }
  return true;
}
#ifndef NDEBUG
//+----------------------------------------------------------------------------+
//| Dump stack items to devie log. For debug only                             |
//+----------------------------------------------------------------------------+
void CStackTrace::Dump()
{
  StackItem *item = m_stack_first;
  uint index = 0;
  while(item!=NULL)
  {
    __android_log_print(ANDROID_LOG_DEBUG,"CrashPin","#%02d 0x%08x %s<%s>",index,item->address,item->module,item->function);
    ++index;
    item = item->next;
  }
}
#endif