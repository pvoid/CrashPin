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
#ifndef STACKTRACE_MARK
#define STACKTRACE_MARK
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
#include <unistd.h>
#include "map.h"
#include <sys/ptrace.h>
#include <stdio.h>
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
struct StackItem
{
  struct StackItem  *next;
  _uw                address;
  char               function[64];
  char               module[];
};
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
class CStackTrace
{
private:
  enum
  {
    CODE_FINISH = 0xb0
  };
  enum EnRegisters
  {
    R_IP = 12,
    R_SP = 13,
    R_LR = 14,
    R_PC = 15
  };
private:
  typedef struct { _uw r[16]; } CoreRegs;
  typedef struct { _uw64 d[16]; _uw pad; } VFPRegs;
  typedef struct { _uw64 d[16]; } VFPv3Regs;
  typedef struct { _uw w[3]; } FPAReg;
  typedef struct { FPAReg f[8]; } FPARegs;
  typedef struct { _uw64 wd[16]; } WmmxdRegs;
  typedef struct { _uw wc[4]; } WmmxcRegs;

  typedef struct
     {
      /* The first fields must be the same as a phase2_vrs.  */
      _uw demand_save_flags;
      CoreRegs core;
      _uw prev_sp; /* Only valid during forced unwinding.  */
      VFPRegs vfp;
      VFPv3Regs vfp_regs_16_to_31;
      FPARegs fpa;
      WmmxdRegs wmmxd;
      WmmxcRegs wmmxc;
     } Phase1Vars;
public:
                      CStackTrace(pid_t tid);
  bool                BackTrace();
#ifndef NDEBUG
  void                Dump();
#endif
private:
  pid_t               m_tid;
  CMap                m_map;
  StackItem*          m_stack_first;
  StackItem*          m_stack_last;
  pt_regs             m_regs;
private:
  bool                AddStackEntry(_uw address, const char* function_name, const char* module_name);
  _Unwind_Reason_Code AddStackEntry(Phase1Vars &context);
  _Unwind_Reason_Code UnwindCommon(_Unwind_Control_Block &ucb, Phase1Vars &context, int id);
  _Unwind_Reason_Code UnwindExecute(Phase1Vars& context, __gnu_unwind_state& uws);
  _uw8                UnwindNextByte(__gnu_unwind_state& uws);
  _Unwind_VRS_Result  UnwindVrsPop(Phase1Vars& context, _Unwind_VRS_RegClass regclass, _uw discriminator, _Unwind_VRS_DataRepresentation representation);
};
#endif