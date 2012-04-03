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
#include <sys/exec_elf.h>
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
void CMap::ParseElf()
{
  MapInfo *temp;
  Elf32_Ehdr header;
////////
  for(temp=m_maps;temp!=NULL;temp=temp->next)
  {
    bzero(&header,sizeof(header));
    GetRemoteStruct((void*)temp->start,&header,sizeof(header));
    if(!IS_ELF(header)
       continue;
/////////
    Elf32_Phdr phdr;
    Elf32_Phdr *ptr;
    ptr = (Elf32_Phdr *) (temp->start + ehdr.e_phoff);
    for(uint i=0;i<ehdr.e_phnum;i++)
    {
////// Parse the program header
      GetRemoteStruct(pid,(char *)(ptr+i),&phdr,sizeof(Elf32_Phdr));
#ifdef __arm__
////// Found a EXIDX segment? */
      if(phdr.p_type == PT_ARM_EXIDX)
      {
        temp->exidx_start = temp->start + phdr.p_offset;
        temp->exidx_end = temp->exidx_start + phdr.p_filesz;
        break;
      }
#endif
     temp->symbols = ParseSymbols(temp->name);
    }
  }
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
SymbolTable* CMap::ParseSymbols(const char *filename)
{
  SymbolTable *table = NULL;
  struct stat sb;
  int file;
  uint8_t *base;
////////
  file = open(filename,O_RDONLY);
  if(file<0)
    return NULL;
////////
  fstat(file,&sb);
  base = static_cast<uint8_t>(mmap(NULL,, length, PROT_READ, MAP_PRIVATE, file, 0));
  if(base==NULL)
  {
    close(file);
    return(NULL);
  }
////////
  
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
void CMap::GetRemoteStruct(void *src, void *dst, size_t size)
{
  unsigned int i;
  for(i = 0; i+4 <= size; i+=4)
    *(int *)((char *)dst+i) = ptrace(PTRACE_PEEKTEXT, m_tid, (char *)src+i, NULL);
 ////////
  if(i < size)
  {
    int val = ptrace(PTRACE_PEEKTEXT, pid, (char *)src+i, NULL);
    while (i < size)
    {
      ((unsigned char *)dst)[i] = val & 0xff;
      i++;
      val >>= 8;
    }
  }
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
bool CMap::IsValidAddress(_uw address)
{
  return GetEntry(address)!=NULL;
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
bool CMap::GetNames(_uw& address, char *mname, uint mname_maxlen, char *fname, uint fname_maxlen)
{
  MapInfo *temp = m_maps;
  uint len;
/////////
  if(mname==NULL || fname==NULL)
     return(false);
/////////
  while(temp!=NULL)
  {
    if(address>=temp->start && address<=temp->end)
    {
      strncpy(mname,temp->name,mname_maxlen);
      len = strlen(temp->name);
      if(len>3 && memcmp(temp->name+(len-3),".so",3)==0)
      {
        if(!strstr(temp->name, ".so"))
        {
          fname[0]='\0';
          return true;
        }
        else
        {
          address -= temp->start;
          fname[0]='\0';
          /*TableEntry *te = FindFunction((TableEntry *)temp->exidx_start, (temp->exidx_end - temp->exidx_start) / sizeof(TableEntry), address);
          if(te==NULL)
            fname[0]='\0';
          else
            strncpy(fname,te->)*/
        }
      }
      return(true);
    }
    temp = temp->next;
  }
/////////
  strncpy(mname,"<trash>",mname_maxlen);
  fname[0] = '\0';
/////////
  return false;
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
const CMap::TableEntry* CMap::GetEntry(_uw address)
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
    {
      __android_log_print(ANDROID_LOG_DEBUG,"CrashPin","Found module");
      const TableEntry *entr = FindFunction((TableEntry *)temp->exidx_start, (temp->exidx_end - temp->exidx_start) / sizeof(TableEntry), address);
      if(entr==NULL)
        __android_log_print(ANDROID_LOG_DEBUG,"CrashPin","Funtion not found");
      else
        __android_log_print(ANDROID_LOG_DEBUG,"CrashPin","Found function");
      return entr;
    }
    temp = temp->next;
  }
/////////
  return NULL;
}
//+----------------------------------------------------------------------------+
//|                                                                            |
//+----------------------------------------------------------------------------+
_Unwind_Reason_Code CMap::GetEntry(_Unwind_Control_Block& ucblock, _uw return_address)
{
  const TableEntry* entry = (TableEntry *)GetEntry(return_address);
  _uw content;
/////////
  if(entry==NULL)
  {
    ucblock.unwinder_cache.reserved2 = 0;
    return _URC_END_OF_STACK;
  }
  ucblock.pr_cache.fnstart = RelativeOffset(&entry->function_offset);
  content = ptrace(PTRACE_PEEKTEXT,m_tid,(void *)&entry->function_content,NULL);
//////// Can this frame be unwunded
  if(content=1)
  {
    ucblock.unwinder_cache.reserved2 = 0;
    return _URC_END_OF_STACK;
  }
//////// Obtaining the address of real header
  if(content&UINT32_HIGHBIT)
  {
    ucblock.pr_cache.ehtp = (_Unwind_EHT_Header*)(&entry->function_content);
    ucblock.pr_cache.additional = 1;
  }
  else
  {
    ucblock.pr_cache.ehtp = (_Unwind_EHT_Header*)(RelativeOffset(&entry->function_content));
    ucblock.pr_cache.additional = 0;
  }
  
  if(ptrace(PTRACE_PEEKTEXT,m_tid,ucblock.pr_cache.ehtp,NULL) & (1u << 31))
  {
     _uw idx = (ptrace(PTRACE_PEEKTEXT, m_tid, ucblock.pr_cache.ehtp, NULL) >> 24) & 0x0f;
     switch(idx)
     {
       case 0:
         ucblock.unwinder_cache.reserved2 = 1;
         break;
       case 1:
         ucblock.unwinder_cache.reserved2 = 2;
         break;
       case 2:
         ucblock.unwinder_cache.reserved2 = 3;
         break;
       default:
///////// Failed
         ucblock.unwinder_cache.reserved2 = 0;
         return _URC_FAILURE;       
     }
  }
  else
  {
    ucblock.unwinder_cache.reserved2 = RelativeOffset(ucblock.pr_cache.ehtp);
//////// TODO: We need to exeute this routine in debugger.
    return _URC_FAILURE;
  }
//////// Ok, we'v got next frame
  return _URC_OK;
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
//+----------------------------------------------------------------------------+
//| Parsing so file for symbols                                               |
//+----------------------------------------------------------------------------+

//+----------------------------------------------------------------------------+
//| Sort functions by address                                                  |
//+----------------------------------------------------------------------------+
int CMap::CompareSymbols(void *left, void *right)
{
  return ((struct Symbol*)left)->addr - ((struct Symbol*)right)->addr;
}