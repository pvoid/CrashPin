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
CStackTrace::CStackTrace(pid_t tid) : m_tid(tid), m_map(tid), m_stack_first(NULL), m_stack_last(NULL)
{
}

bool CStackTrace::BackTrace()
{
  _uw pc;
  _Unwind_Reason_Code code = _URC_OK;
  _Unwind_Control_Block ucb;
  Phase1Vars context = {0};
//////////
  if(ptrace(PTRACE_GETREGS, m_tid, 0, &m_regs)!=0)
     memset(&m_regs,0,sizeof(m_regs));
  pc = m_regs.ARM_pc;
 ///////// Checking if app is failed because it called trash
  if(!m_map.IsValidAddress(pc))
  {
    AddStackEntry(context);
////////// We'll try to get stacktrace from caller address
    pc = m_regs.ARM_lr;
  }
  do
  {
    code = m_map.GetEntry(ucb, pc);
    if(code!=_URC_OK)
      break;
//////////
    _Unwind_SetGR((_Unwind_Context *)&context, 12, (_Unwind_Ptr)&ucb);
    AddStackEntry(context);
    switch(ucb.unwinder_cache.reserved2)
    {
      case 1:
        code = UnwindCommon(ucb, context, 0);
        break;
      case 2:
        code = UnwindCommon(ucb, context, 1);
        break;
      case 3:
        code = UnwindCommon(ucb, context, 2);
        break;
      default:
        code = _URC_FAILURE;
        break;
    }
  }
  while(code!=_URC_END_OF_STACK && code!=_URC_FAILURE); // TODO: Stack depth control
  return true;
}
//+----------------------------------------------------------------------------+
//|                                                                           |
//+----------------------------------------------------------------------------+
_Unwind_Reason_Code CStackTrace::UnwindCommon(_Unwind_Control_Block &ucb, Phase1Vars &context, int id)
{
  _uw *data = (_uw *)ucb.pr_cache.ehtp;
  __gnu_unwind_state ustate;
  int state = (_Unwind_State)(_US_VIRTUAL_UNWIND_FRAME | _US_FORCE_UNWIND) & _US_ACTION_MASK;
//////////
  ustate.data = ptrace(PTRACE_PEEKTEXT, m_tid, data, NULL);
  ++data;
  ustate.next = data;
  if(id == 0)
  {
    ustate.data <<= 8;
    ustate.words_left = 0;
    ustate.bytes_left = 3;
  }
  else
  {
    ustate.words_left = (ustate.data >> 16) & 0xff;
    ustate.data <<= 16;
    ustate.bytes_left = 2;
    data += ustate.words_left;
  }
  
  if(state == _US_UNWIND_FRAME_RESUME)
    data = (_uw *)&ucb.cleanup_cache.bitpattern[0];
    
  if(ucb.pr_cache.additional&1==0 && ptrace(PTRACE_PEEKTEXT,m_tid,data,NULL))
    {
      // Exception;
      return _URC_FAILURE;
    }
    
  if(UnwindExecute(context, ustate)!=_URC_OK)
    return(_URC_FAILURE);

  return(_URC_CONTINUE_UNWIND);
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
_Unwind_Reason_Code CStackTrace::UnwindExecute(Phase1Vars& context, __gnu_unwind_state& uws)
{
  _uw op;
  _uw registry;
  _uw offset, mask, shift;
  bool pc_setted = false;
  while(true)
  {
     op = UnwindNextByte(uws);
     if(op==CODE_FINISH)
     {
//////// if pc wasn't setted, we will take it from LR
       if(!pc_setted)
       {
          _Unwind_VRS_Get((_Unwind_Context *)&context,_UVRSC_CORE,R_LR,_UVRSD_UINT32,&registry);
          _Unwind_VRS_Set((_Unwind_Context *)&context,_UVRSC_CORE,R_PC,_UVRSD_UINT32,&registry);
          pc_setted = true;
       }
       break;
     }
////////
     if((op & 0xf0) == 0x80)
     {
        offset = ((op & 0x3f) << 2) + 4;
        _Unwind_VRS_Get((_Unwind_Context *)&context,_UVRSC_CORE,R_SP,_UVRSD_UINT32,&registry);
        if(op & 0x40)
           registry -= offset;
        else
           registry += offset;
        _Unwind_VRS_Set((_Unwind_Context *)&context,_UVRSC_CORE, R_SP,_UVRSD_UINT32,&registry);
     }
////////
     if((op & 0xf0) == 0x80)
     {
       op = (op << 8) | UnwindNextByte(uws);
//////// Refuse to unwind
       if(op == 0x8000)
       {
         return _URC_FAILURE;
       }
//////// Pop r4-r15 under mask.
       op = (op << 4) & 0xfff0;
       if(UnwindVrsPop(context, _UVRSC_CORE, op, _UVRSD_UINT32)!= _UVRSR_OK)
         return _URC_FAILURE;
       if(op & (1 << R_PC))
         pc_setted = true;
       continue;
     }
////////
     if((op & 0xf0) == 0x90)
     {
       op &= 0xf;
//////// Reserved??
       if(op == 13 || op == 15)
       {
         return _URC_FAILURE;
       }
       _Unwind_VRS_Get ((_Unwind_Context *)&context, _UVRSC_CORE, op, _UVRSD_UINT32, &registry);
       _Unwind_VRS_Set ((_Unwind_Context *)&context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &registry);
        continue;
       }
////////
     if((op & 0xf0) == 0xa0)
       {
        mask = (0xff0 >> (7 - (op & 7))) & 0xff0;
        if(op & 8)
           mask |= (1 << R_LR);
        if(UnwindVrsPop(context, _UVRSC_CORE, mask, _UVRSD_UINT32) != _UVRSR_OK)
           return _URC_FAILURE;
        continue;
       }
/////////
     if((op & 0xf0) == 0xb0)
     {
        if(op == 0xb1)
        {
          op = UnwindNextByte(uws);
          if(op == 0 || ((op & 0xf0) != 0))
          {
            return _URC_FAILURE;
          }
//////////// Pop r0-r4 under mask.
          if(UnwindVrsPop(context, _UVRSC_CORE, op,_UVRSD_UINT32)!= _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
/////////
        if(op == 0xb2)
        {
          _Unwind_VRS_Get((_Unwind_Context *)&context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &registry);
          op = UnwindNextByte(uws);
          shift = 2;
          while(op & 0x80)
          {
            registry += ((op & 0x7f) << shift);
            shift += 7;
            op = UnwindNextByte(uws);
          }
          registry += ((op & 0x7f) << shift) + 0x204;
          _Unwind_VRS_Set((_Unwind_Context *)&context, _UVRSC_CORE, R_SP, _UVRSD_UINT32, &registry);
          continue;
        }
/////////
        if(op == 0xb3)
        {
////////// Pop VFP registers with fldmx.
          op = UnwindNextByte(uws);
          op = ((op & 0xf0) << 12) | ((op & 0xf) + 1);
          if(UnwindVrsPop(context, _UVRSC_VFP, op, _UVRSD_VFPX) != _UVRSR_OK)
            return _URC_FAILURE;
          continue;
        }
/////////
        if((op & 0xfc) == 0xb4)
        {
//////// Pop FPA E[4]-E[4+nn].
           op = 0x40000 | ((op & 3) + 1);
           if(UnwindVrsPop(context, _UVRSC_FPA, op, _UVRSD_FPAX) != _UVRSR_OK)
              return _URC_FAILURE;
           continue;
        }
        /* op & 0xf8 == 0xb8.  */
        /* Pop VFP D[8]-D[8+nnn] with fldmx.  */
        op = 0x80000 | ((op & 7) + 1);
        if(UnwindVrsPop(context, _UVRSC_VFP, op, _UVRSD_VFPX) != _UVRSR_OK)
           return _URC_FAILURE;
        continue;
     }
/////////
     if((op & 0xf0) == 0xc0)
       {
        if(op == 0xc6)
          {
///////////// Pop iWMMXt D registers.
           op = UnwindNextByte(uws);
           op = ((op & 0xf0) << 12) | ((op & 0xf) + 1);
           if(UnwindVrsPop(context, _UVRSC_WMMXD, op, _UVRSD_UINT64) != _UVRSR_OK)
              return _URC_FAILURE;
           continue;
          }
///////////
        if(op == 0xc7)
          {
           op = UnwindNextByte(uws);
/////////////// Spare.
           if(op == 0 || (op & 0xf0) != 0)
              return _URC_FAILURE;
/////////////// Pop iWMMXt wCGR{3,2,1,0} under mask.
           if(UnwindVrsPop(context, _UVRSC_WMMXC, op,_UVRSD_UINT32) != _UVRSR_OK)
              return _URC_FAILURE;
           continue;
          }
////////////
        if((op & 0xf8) == 0xc0)
          {
/////////////// Pop iWMMXt wR[10]-wR[10+nnn].
           op = 0xa0000 | ((op & 0xf) + 1);
           if(UnwindVrsPop(context, _UVRSC_WMMXD, op, _UVRSD_UINT64) != _UVRSR_OK)
              return _URC_FAILURE;
           continue;
          }
////////////
        if(op == 0xc8)
          {
#ifndef __VFP_FP__
/////////////// Pop FPA registers.
            op = NextUnwindByte(uws, pid);
            op = ((op & 0xf0) << 12) | ((op & 0xf) + 1);
            if(UnwindVrsPop(context, _UVRSC_FPA, op, _UVRSD_FPAX) != _UVRSR_OK)
               return _URC_FAILURE;
            continue;
#else
/////////////// Pop VFPv3 registers D[16+ssss]-D[16+ssss+cccc] with vldm.
            op = UnwindNextByte(uws);
            op = (((op & 0xf0) + 16) << 12) | ((op & 0xf) + 1);
            if(UnwindVrsPop(context, _UVRSC_VFP, op, _UVRSD_DOUBLE) != _UVRSR_OK)
               return _URC_FAILURE;
            continue;
#endif
           }
////////////
        if(op == 0xc9)
          {
////////////// Pop VFP registers with fldmd.
           op = UnwindNextByte(uws);
           op = ((op & 0xf0) << 12) | ((op & 0xf) + 1);
           if(UnwindVrsPop(context, _UVRSC_VFP, op, _UVRSD_DOUBLE) != _UVRSR_OK)
              return _URC_FAILURE;
           continue;
          }
////////////// Spare.
        return _URC_FAILURE;
       }
///////////
     if((op & 0xf8) == 0xd0)
       {
////////////// Pop VFP D[8]-D[8+nnn] with fldmd.
        op = 0x80000 | ((op & 7) + 1);
        if(UnwindVrsPop(context, _UVRSC_VFP, op, _UVRSD_DOUBLE) != _UVRSR_OK)
           return _URC_FAILURE;
        continue;
       }
     //--- Spare.
     return _URC_FAILURE;
    }
   return _URC_OK;
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
_uw8 CStackTrace::UnwindNextByte(__gnu_unwind_state& uws)
{
  _uw8 result;
///////// if something left?
  if(uws.bytes_left == 0)
  {
///////// load another word
    if(uws.words_left == 0)
      return CODE_FINISH;
    uws.words_left--;
    uws.data = ptrace(PTRACE_PEEKTEXT, m_tid, uws.next, NULL);
    uws.next++;
    uws.bytes_left = 3;
  }
  else
    uws.bytes_left--;
/////// Extract the most significant byte.
  result = (uws.data >> 24) & 0xff;
  uws.data <<= 8;
  return(result);
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
_Unwind_VRS_Result CStackTrace::UnwindVrsPop(Phase1Vars& context, _Unwind_VRS_RegClass regclass, _uw discriminator, _Unwind_VRS_DataRepresentation representation)
{
  Phase1Vars &vrs = *(Phase1Vars*)&context;
  switch (regclass)
  {
     case _UVRSC_CORE:
     {
       _uw *ptr;
       _uw mask;
       int i;
///////////
       if(representation != _UVRSD_UINT32)
         return _UVRSR_FAILED;
       mask = discriminator & 0xffff;
       ptr = (_uw *)vrs.core.r[R_SP];
/////////// Pop the requested registers.
       for (i = 0; i < 16; i++)
       {
         if (mask & (1 << i)) 
         {
           vrs.core.r[i] = ptrace(PTRACE_PEEKTEXT, m_tid, ptr, NULL);
           ptr++;
         }
       }
/////////// Writeback the stack pointer value if it wasn't restored.
       if((mask & (1 << R_SP)) == 0)
         vrs.core.r[R_SP] = (_uw) ptr;
///////////
       return _UVRSR_OK;
     }
     default:
       return _UVRSR_FAILED;
  }
}
//+----------------------------------------------------------------------------+
//|                                                                           |
//+----------------------------------------------------------------------------+
_Unwind_Reason_Code CStackTrace::AddStackEntry(Phase1Vars &context)
{
  _uw pc = context.core.r[R_PC];
  _uw prev_word;
  char function[64];
  char module[128];
/////
  if(m_stack_first==NULL)
    pc &= ~0x01;
  else
  {
////// Thumb ??
    if(pc&0x01)
    {
       pc &= ~1;
       prev_word = ptrace(PTRACE_PEEKTEXT, m_tid, (char *)pc-4, NULL);
       if((prev_word & 0xf0000000) == 0xf0000000 && (prev_word & 0x0000e000) == 0x0000e000)
         pc -= 4;
       else
         pc -= 2;
    }
    else
       pc -= 0x04;
  }
///////
  if(m_map.GetNames(pc,module,sizeof(module),function,sizeof(function)))
    AddStackEntry(pc,function,module);
}
//+----------------------------------------------------------------------------+
//| Add new record to stack list                                              |
//+----------------------------------------------------------------------------+
bool CStackTrace::AddStackEntry(_uw address, const char* function_name, const char* module_name)
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