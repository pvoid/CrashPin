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
///////////
#ifndef MAPINFO_MARK
#define MAPINFO_MARK
/////////
#include <unistd.h>
#include <unwind.h>
/////////
#ifndef NULL
#define NULL 0
#endif
/////////
class CMap
{
public:
                     CMap(pid_t tid);
                    ~CMap();
////////////////////////////////////////
#ifndef NDEBUG
  void               Dump();
#endif
////////////////////////////////////////
  _Unwind_Reason_Code GetEntry(_Unwind_Control_Block &ucblok, _uw return_address);
  bool                IsValidAddress(_uw address);
  bool                GetNames(_uw address, const char *mname, uint mname_maxlen, const char *fname, uint fname_maxlen);
private:
  enum EnConstants
  {
    UINT32_HIGHBIT = (((_uw)1) << 31)
  };
private:
  struct Symbol
  {
    uint32_t addr;
    uint32_t size;
    char *name;
  };
////////////////////////////////////////
  struct SymbolTable
  {
    struct Symbol *symbols;
    int num_symbols;
    char *name;
  };
////////////////////////////////////////
  struct MapInfo
  {
    struct MapInfo *next;
    uint32_t start;
    uint32_t end;
    uint32_t exidx_start;
    uint32_t exidx_end;
    struct SymbolTable *symbols;
    char name[];
  };
////////////////////////////////////////
  struct TableEntry
  {
    _uw function_offset;
    _uw function_content;
  };
private:
  pid_t              m_tid;
  MapInfo*           m_maps;
private:
  MapInfo*           ParseLine(char *line);
  _uw                RelativeOffset(const _uw* address);
  const TableEntry*  GetEntry(_uw address);
  const TableEntry*  FindFunction(const TableEntry* table, int nrec, _uw return_address);
private:
  //static _Unwind_Reason_Code UnwindCppPr0WithPtrace(_Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context, pid_t pid)
};
#endif
