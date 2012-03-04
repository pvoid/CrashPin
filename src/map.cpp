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
#include "map.h"
#include "stdio.h"
#include "stdlib.h"
#include <sys/ptrace.h>
#include <errno.h>
//+----------------------------------------------------------------------------+
//| Constructor. Load module symbols                                           |
//+----------------------------------------------------------------------------+
CMap::CMap(pid_t tid) : m_tid(tid), m_maps(NULL)
{
  char data[1024];
  FILE *map = NULL;
  MapInfo *info = NULL;
/////////
  sprintf(data, "/proc/%d/maps", tid);
  if((map = fopen(data, "r")) != NULL)
  {
   while(fgets(data, 1024, map))
    {
     if(info = ParseLine(data))
      {
       info->next = m_maps;
       m_maps = info;
      }
    }
   fclose(map);
  }
}
//+----------------------------------------------------------------------------+
//| Destructor. Clears map info.                                               |
//+----------------------------------------------------------------------------+
CMap::~CMap()
{
  MapInfo *tmp = NULL;
  while(m_maps!=NULL)
  {
    tmp = m_maps->next;
    free(m_maps);
    m_maps = tmp;
  }
}
#ifndef NDEBUG
#include <android/log.h>
//+----------------------------------------------------------------------------+
//| Dumps map info in system log. For debug only                               |
//+----------------------------------------------------------------------------+
void CMap::Dump()
{
  MapInfo *first = m_maps;
  while(first!=NULL)
  {
    __android_log_print(ANDROID_LOG_DEBUG,"CrashPin","%s start: %d end %d",first->name,first->start,first->end);
    first = first->next;
  }
}
#endif
//+----------------------------------------------------------------------------+
//|                                                                            |
//| Parse line from maps in proc file. Map usally looks like this one          |
//|                                                                            |
//| address perms offset dev inode pathname                                    |
//| 08048000 - 08056000 r - xp 00000000 03:0c 64593 /usr/sbin/gpm              |
//|                                                                            |
//+----------------------------------------------------------------------------+
CMap::MapInfo* CMap::ParseLine(char* line)
{
  MapInfo *mi = NULL;
  uint length = 0;
/////////
  if(line==NULL)
    return(NULL);
/////////
  length = strlen(line);
//////// Skip invalid lines and not exutable entries if they could be so
  if(length<1 || length<50 || line[20]!='x')
    return NULL;
//////// Clear line break
  line[--length] = '\0';
///////// Alloc strcuture + name length
 mi = (MapInfo*)malloc(sizeof(MapInfo) + (length - 47));
 if(mi == 0)
   return 0;
 mi->start = strtoull(line, 0, 16);
 mi->end = strtoull(line + 9, 0, 16);
 mi->exidx_start = mi->exidx_end = 0;
 mi->symbols = 0;
 mi->next = 0;
 strcpy(mi->name, line + 49);
 return mi;
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
const void* CMap::GetEntry(_uw address)
{
  MapInfo *temp = m_maps;
/////////
  if(address>2)
    address-=2;
/////////
  while(temp!=NULL)
  {
///////// Address somewhere in current module. Try to find function
    if(address>=temp->start && address<=temp->end)
      return FindFunction((TableEntry *)temp->exidx_start, (temp->exidx_end - temp->exidx_start) / sizeof(TableEntry), address);
    temp = temp->next;
  }
/////////
  return NULL;
}
//+----------------------------------------------------------------------------+
//| Returns relative address                                                   |
//+----------------------------------------------------------------------------+
_uw CMap::RelativeOffset(const _uw* address)
{
///////// Peek offset from child memmory 
  _uw offset = ptrace(PTRACE_PEEKTEXT, m_tid, (void*)address, NULL);
  if(offset==-1 && errno)
    return(NULL);
/////////
  if(offset & (1 << 30))
    offset |= 1u << 31;
  else
    offset &= ~(1u << 31);
/////////
  return offset + (_uw)address;
}
//+----------------------------------------------------------------------------+
//| Search for function by address                                             |
//+----------------------------------------------------------------------------+
const CMap::TableEntry* CMap::FindFunction(const TableEntry* table, int nrec, _uw return_address)
{
  _uw next_fn;
  _uw this_fn;
  int n, left, right;
  if(nrec == 0)
    return NULL;
  left  = 0;
  right = nrec - 1;
/////////
  while(true)
  {
    n = (left + right) / 2;
    this_fn = RelativeOffset(&table[n].function_offset);
    if(n != nrec - 1)
      next_fn = RelativeOffset(&table[n + 1].function_offset) - 1;
    else
      next_fn = (_uw)0 - 1;

    if(return_address < this_fn)
    {
      if(n == left)
        return NULL;
      right = n - 1;
    }
    else if(return_address <= next_fn)
      return &table[n];
    else
      left = n + 1;
  }
}

