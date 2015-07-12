/* * * * *
 *  AzIntPool.cpp 
 *  Copyright (C) 2011-2014 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */


#include "AzUtil.hpp"
#include "AzIntPool.hpp"
#include "AzPrint.hpp"

int az_compare_IP_ent(const void *v1, const void *v2); 

/*------------------------------------------------------------------*/
void AzIntPool::_read(AzFile *file) 
{
  const char *eyec = "AzIntPool::read(file)"; 

  ent_num = file->readInt(); 
  data_num = file->readInt8(); 
  isCommitted = file->readBool(); 

  a_ent.alloc(&ent, ent_num, eyec, "ent"); 
  a_data.alloc(&data, data_num, eyec, "data"); 

  file->seekReadBytes(-1, sizeof(data[0])*data_num, data); 
  file->seekReadBytes(-1, sizeof(ent[0])*ent_num, ent); 
  _swap(); 

  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    AzIpEnt *ep = &ent[ex]; 
    ep->ints = data + ep->offs; 
  }
}

/*------------------------------------------------------------------*/
void AzIntPool::_swap()
{
  if (!isSwapNeeded) {
    return; 
  }
  int ex; 
  for (ex = 0; ex < data_num; ++ex) {
    AzFile::swap_int4(&data[ex]); 
  }

  for (ex = 0; ex < ent_num; ++ex) {
    AzIpEnt *ep = &ent[ex]; 
    AzFile::swap_int8(&ep->offs); 
    AzFile::swap_int4(&ep->num); 
    AzFile::swap_int8(&ep->count); 
    AzFile::swap_int4(&ep->value); 
  }
}

/*------------------------------------------------------------------*/
void AzIntPool::write(AzFile *file) 
{
  file->writeInt(ent_num); 
  file->writeInt8(data_num); 
  file->writeBool(isCommitted); 

  _swap(); 
  file->writeBytes(data, sizeof(data[0])*data_num); 
  file->writeBytes(ent, sizeof(ent[0])*ent_num); 
  _swap(); 
}

/*------------------------------------------------------------------*/
void AzIntPool::update(int ex, 
                       const int *ints,         
                       int ints_num, 
                       AZint8 count, 
                       int val)
{
  const char *eyec = "AzIntPool::update"; 
  checkRange(ex, eyec); 
  isCommitted = false; 

  AZint8 data_num_max = a_data.size(); 
  if (data_num + ints_num > data_num_max) {   
    data_num_max += inc_data(data_num+ints_num-data_num_max); 
    a_data.realloc(&data, data_num_max, eyec, "data"); 
  }

  AzIpEnt *ep = &ent[ex]; 
  ep->ints = NULL;  /* set this after committing */
  ep->offs = data_num; 
  ep->num = ints_num; 
  ep->count = count; 
  ep->value = val; 
  memcpy(data + ep->offs, ints, 
         ints_num * sizeof(ints[0])); 
  data_num += ints_num; 
}

/*------------------------------------------------------------------*/
int AzIntPool::put(const int *ints,         
                   int ints_num, 
                   AZint8 count, 
                   int val)
{
  const char *eyec = "AzIntPool::put"; 

  if (isCommitted) {
    int ex = find(ints, ints_num); 
    if (ex != AzNone) {
      ent[ex].count += count; 
      if (val != ent[ex].value) {
        throw new AzException(eyec, "value conflict"); 
      }
      return ex; 
    }
  }

  isCommitted = false; 

  int ent_num_max = a_ent.size(); 
  if (ent_num >= ent_num_max) {
    ent_num_max += inc_ent(); 
    a_ent.realloc(&ent, ent_num_max, eyec, "ent"); 
  }

  AZint8 data_num_max = a_data.size(); 
  if (data_num + ints_num > data_num_max) {   
    data_num_max += inc_data(data_num+ints_num-data_num_max); 
    a_data.realloc(&data, data_num_max, eyec, "data"); 
  }

  AzIpEnt *ep = &ent[ent_num]; 
  ep->ints = NULL;  /* set this after committing */
  ep->offs = data_num; 
  ep->num = ints_num; 
  ep->count = count; 
  ep->value = val; 
  memcpy(data + ep->offs, ints, 
         ints_num * sizeof(ints[0])); 
  data_num += ints_num; 
  ++ent_num; 
  return (ent_num - 1); 
}

/*------------------------------------------------------------------*/
void AzIntPool::shorten(int idx, 
                         int new_len)
{
  checkRange(idx, "AzIntPool::shorten");   
  if (new_len < 0 || new_len > ent[idx].num) {
    throw new AzException("AzIntPool::shorten", "len is out of range"); 
  }
  ent[idx].num = new_len; 
}

/*------------------------------------------------------------------*/
const int *AzIntPool::point(int ent_no, int *out_ints_num) const
{
  checkRange(ent_no, "AzIntPool::point");  

  if (out_ints_num != NULL) {
    *out_ints_num = ent[ent_no].num; 
  }
  return data + ent[ent_no].offs; 
}

/*------------------------------------------------------------------*/
AZint8 AzIntPool::getCount(int ent_no) const
{
  checkRange(ent_no, "AzIntPool::getCount"); 
  return ent[ent_no].count; 
}

/*------------------------------------------------------------------*/
void AzIntPool::setCount(int ent_no, AZint8 new_count) 
{
  checkRange(ent_no, "AzIntPool::setCount"); 
  ent[ent_no].count = new_count; 
}

/*------------------------------------------------------------------*/
void AzIntPool::commit()  
{
  const char *eyec = "AzIntPool::commit"; 

  isCommitted = true; 
  if (ent_num == 0) {
    return; 
  }

  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    ent[ex].ints = data + ent[ex].offs; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IP_ent); 

  AZint8 out_num = 0; 
  int new_id = 0; 
  for (ex = 0; ex < ent_num; ) {
    out_num += ent[ex].num; 
    if (new_id != ex) {
      ent[new_id] = ent[ex]; 
    }
    AZint8 count = 0; 
    for ( ; ex < ent_num; ++ex) {
      if (az_compare_IP_ent(&ent[ex], &ent[new_id]) != 0) {
        break;
      }

      count += ent[ex].count; 
      if (ent[ex].value != ent[new_id].value) {
        throw new AzException(eyec, "value conflict"); 
      }
    }
    ent[new_id].count = count; 
    ++new_id; 
  }
  ent_num = new_id; 

  /*-----  -----*/
  if (out_num > 0) {
    int *new_data = NULL; 
    AzBaseArray<int,AZint8> a_new_data; 
    a_new_data.alloc(&new_data, out_num, eyec, "new_data"); 

    AZint8 new_data_num = 0; 
    for (ex = 0; ex < ent_num; ++ex) {
      AzIpEnt *ep = &ent[ex]; 
      memcpy(new_data + new_data_num, 
             ep->ints, ep->num * sizeof(ep->ints[0])); 

      ep->offs = new_data_num;  
      ep->ints = new_data + ep->offs;  

      new_data_num += ep->num; 
    }

    a_data.transfer_from(&a_new_data, &data, &new_data); 
    data_num = new_data_num; 
  }
  else {
    a_data.free(&data); 
    data_num = 0; 
  }
}

/*------------------------------------------------------------------*/
int AzIntPool::find(const int *ints, 
                         int ints_num) const
{
  const char *eyec = "AzIntPool::find"; 

  if (!isCommitted) {
    throw new AzException(eyec, "Commit first"); 
  }

  AzIpEnt inp; 
  inp.ints = ints; 
  inp.num = ints_num; 

  int lx = 0; 
  int hx = ent_num - 1; 

  int found = AzNone; 

  for ( ; ; ) {
    if (lx > hx) break;
    int mx = (lx + hx) / 2; 
    int cmp = az_compare_IP_ent(&inp, &ent[mx]); 

    if (cmp < 0) hx = mx - 1; 
    else if (cmp > 0) lx = mx + 1; 
    else {
      found = mx; 
      break;
    }
  }
  return found; 
}

/*------------------------------------------------------------------*/
int az_compare_IP_ent(const void *v1, const void *v2) 
{
  AzIpEnt *p1 = (AzIpEnt *)v1; 
  AzIpEnt *p2 = (AzIpEnt *)v2; 

  int cmp_num = MIN(p1->num, p2->num); 
  int ix; 
  for (ix = 0; ix < cmp_num; ++ix) {
    int cmp = p1->ints[ix] - p2->ints[ix]; 
    if (cmp != 0) {
      return (cmp < 0) ? -1 : 1; 
    }
  }
  if (p1->num < p2->num) return -1; 
  if (p1->num > p2->num) return 1; 
  return 0; 
}

/*------------------------------------------------------------------*/
/* this must match aj_compare_IP_ent */
int AzIntPool::compare(const int *ints1, int num1, 
                       const int *ints2, int num2) 
{
  int cmp_num = MIN(num1, num2); 
  int ix; 
  for (ix = 0; ix < cmp_num; ++ix) {
    int cmp = ints1[ix] - ints2[ix]; 
    if (cmp != 0) return (cmp < 0) ? -1 : 1; 
  }
  int cmp = num1 - num2; 
  return cmp; 
}

/*------------------------------------------------------------------*/
void AzIntPool::dump(const AzOut &out, const char *header) const
{
  if (out.isNull()) return; 

  AzPrint o(out); 
  if (header != NULL) {
    o.writeln(header); 
  }

  int num = size(); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    o.printBegin("", ", "); 
    /* fprintf(out.fp, "[%5ld],%4ld,", ix, getCount(ix)); */
    o.inBrackets(ix,5); o.print(getCount(ix),4); 

    int ints_num; 
    const int *ints = point(ix, &ints_num); 
    AzBytArr s("("); 
    int jx; 
    for (jx = 0; jx < ints_num; ++jx) {
      if (jx > 0) {
        s.c(","); 
      }
      s.cn(ints[jx]); 
    }
    s.c(")"); 
    o.print(s); 
    o.printEnd(); 
  }
}

/*------------------------------------------------------------------*/
int AzIntPool::inc_ent()
{
  return MIN(1024*1024, MAX(1024, ent_num)); 
}

/*------------------------------------------------------------------*/
AZint8 AzIntPool::inc_data(AZint8 min_inc)
{
  AZint8 inc = MIN(1024*1024*2, MAX(2048, data_num)); 
  return MAX(inc, min_inc); 
}

/*------------------------------------------------------------------*/
void AzIntPool::reset(const AzIntPool *ip)
{
  const char *eyec = "AzIntPool::reset"; 
  reset(); 
  a_ent.alloc(&ent, ip->ent_num, eyec, "ent"); 
  ent_num = ip->ent_num; 

  a_data.alloc(&data, ip->data_num, eyec, "data"); 
  data_num = ip->data_num; 
  
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    ent[ex] = ip->ent[ex]; 
    ent[ex].ints = NULL; 
  }
  memcpy(data, ip->data, data_num * sizeof(int)); 

  isCommitted = false; 

  if (ip->isCommitted) {
    commit(); 
  } 
}

/*------------------------------------------------------------------*/
void AzIntPool::reduce(int min_count)
{
  if (min_count <= 0) return; 
  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    if (ent[ex].count >= min_count) {
      if (ex != ex1) ent[ex1] = ent[ex]; 
      ++ex1; 
    }
  }
  ent_num = ex1; 
}
