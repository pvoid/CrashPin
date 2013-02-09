/*
 * Copyright(c) 2013, Dmitry "PVOID" Petuhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   - Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation 
 *     and / or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef DUMBVECTOR_H
#define	DUMBVECTOR_H

#include <sys/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <android/log.h>

template<typename T>
class TDumbVector
{
private:
  enum constants
  {
    STEP = 16,
  };
public:
  typedef int (SortFunc)(const void *left, const void *right);
private:
  T*                m_data;
  size_t            m_total;
  size_t            m_size;
public:
                    TDumbVector();
                   ~TDumbVector();
  void              Shutdown();
  bool              Add(const T& item);
  void              Sort(SortFunc comparator);
  
  size_t            Size()                   { return(m_total);       }
  T&                operator[](size_t index) { return(m_data[index]); }
  
private:
  bool              Realloc();
};
//+----------------------------------------------------------------------------+
//| Constructor                                                                |
//+----------------------------------------------------------------------------+
template<typename T>
TDumbVector<T>::TDumbVector() : m_data(NULL), m_total(0), m_size(0)
{
}
//+----------------------------------------------------------------------------+
//| Destructor                                                                 |
//+----------------------------------------------------------------------------+
template<typename T>
TDumbVector<T>::~TDumbVector()
{
  Shutdown();
}
//+----------------------------------------------------------------------------+
//| Clear resources                                                            |
//+----------------------------------------------------------------------------+
template<typename T>
void TDumbVector<T>::Shutdown()
{
  if(m_data!=NULL)
  {
    delete m_data;
    m_data = NULL;
  }
  m_size = m_total = 0;
}
//+----------------------------------------------------------------------------+
//| Add new item to end of vector                                              |
//+----------------------------------------------------------------------------+
template<typename T>
bool TDumbVector<T>::Add(const T& item)
{
  if(sizeof(T)*(m_total+1)>m_size && !Realloc()) return(false);
  memmove(m_data + m_total,&item,sizeof(T));
  ++m_total;
  return(true);
}
//+----------------------------------------------------------------------------+
//| Change size of vector                                                      |
//+----------------------------------------------------------------------------+
template<typename T>
bool TDumbVector<T>::Realloc()
{
  size_t size;
  T*     data = NULL;
//// First allocation?
  if(m_data==NULL)
  {
    m_size = STEP * sizeof(T);
    if((m_data = (T *)malloc(m_size))==NULL)
      {
        m_size = 0;
        return(false);
      }
    return(true);
  }
//// New size of data
  size = m_size + STEP * sizeof(T);
//// Let's try to realloc our mem
  if((data = (T *)realloc(m_data,size))!=NULL)
  {
    m_data = data;
    m_size = size;
    return(true);
  }
//// Ok, lets try to allocate new block
  if((data = (T*)malloc(size))==NULL)
    return(false);
//// Copy old data
  memcpy(data,m_data,m_size);
  free(m_data);
  m_data = data;
  m_size = size;
//// Ok
  return(true);
}
//+----------------------------------------------------------------------------+
//| Sorting vector                                                             |
//+----------------------------------------------------------------------------+
template<typename T>
void TDumbVector<T>::Sort(TDumbVector<T>::SortFunc comparator)
{
  if(m_data==NULL) return;
//// Sort data
  qsort(m_data,m_total,sizeof(T),comparator);
}
#endif	/* DUMBVECTOR_H */
