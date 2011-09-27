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
//+----------------------------------------------------------------------------+
//| Constructor. Load module symbols                                           |
//+----------------------------------------------------------------------------+
CMap::CMap(pid_t pid) : m_maps(NULL)
{
  char data[1024];
  FILE *map = NULL;
  MapInfo *info = NULL;
/////////
  sprintf(data, "/proc/%d/maps", pid);
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
  line[--length] = 0;
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
