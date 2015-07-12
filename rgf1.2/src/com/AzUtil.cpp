/* * * * *
 *  AzUtil.cpp 
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

#include <time.h>
#include <ctype.h>
#include "AzUtil.hpp"
#include "AzPrint.hpp"

static int th_autoSqueeze = 1024; 

/***************************************************************/
/***************************************************************/
/*-------------------------------------------------------------*/
AzFile::AzFile(const char *fn)
{
  fp = NULL; 
  str_fn = new AzBytArr(fn); 
}

/*-------------------------------------------------------------*/
AzFile::~AzFile()
{
  if (fp != NULL) {
    fclose(fp); 
    fp = NULL; 
  }
  delete str_fn; str_fn = NULL; 
}

/*-------------------------------------------------------------*/
void AzFile::reset(const char *fn)
{
  if (fp != NULL) {
    fclose(fp); 
    fp = NULL; 
  }
  delete str_fn; str_fn = NULL; 
  str_fn = new AzBytArr(fn); 
}

/*-------------------------------------------------------------*/
void AzFile::open(const char *inp_flags) 
{
  const char *eyec = "AzFile::open"; 

  const char *flags = inp_flags; 

  if (str_fn == NULL || str_fn->length() <= 0) {
    throw new AzException(eyec, "No filename"); 
  }

  if (strcmp(flags, "X") == 0) { /* for compatibility .. */
    flags = "rb";  
  }
  if ((fp = fopen(pointFileName(), flags)) == NULL) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "fopen"); 
  }
}

/*-------------------------------------------------------------*/
bool AzFile::isExisting(const char *fn)
{
  FILE *temp_fp = NULL; 
  if ((temp_fp = fopen(fn, "rb")) == NULL) {
    return false; 
  }
  else {
    fclose(temp_fp); 
    return true; 
  }
}

/*-------------------------------------------------------------*/
const char *AzFile::pointFileName() const
{
  if (str_fn == NULL) return ""; 
  return str_fn->c_str(); 
}

/*-------------------------------------------------------------*/
void AzFile::close(bool doCheckError) 
{
  const char *eyec = "AzFile::close"; 
  if (fp != NULL) {
    if (fclose(fp) != 0 && doCheckError) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "fclose"); 
    }
    fp = NULL; 
  }
}
 
/*-------------------------------------------------------------*/
AZint8 AzFile::size() 
{
  const char *eyec = "AzFile::size";

  if (fp == NULL) {
    throw new AzException("AzFile::size()", "file must be opened first"); 
  }

  /*-----  keep current offset  -----*/
  AZint8 offs = ftell(fp); 

  /*-----  seek to eof  -----*/
  if (fseek(fp, 0, SEEK_END) != 0) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "seek to end"); 
  }
  
  AZint8 size = ftell(fp); 
  if (size == -1L) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "ftell"); 
  }

  /*-----  seek it back  -----*/
  if (fseek(fp, offs, SEEK_SET) != 0) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "seek"); 
  }  

  return size; 
}

/*-------------------------------------------------------------*/
int AzFile::gets(AzByte *buff, int buffsize) 
{
  const char *eyec = "AzFile::gets";
  if (fgets((char *)buff, buffsize, fp) == NULL) {
    if (feof(fp)) {
      return 0; 
    }
    throw new AzException(AzFileIOError, eyec, pointFileName(), "fgets"); 
  }

  int len = Az64::cstrlen((char *)buff, "AzFile::gets"); 
  return len; 
}

/*-------------------------------------------------------------*/
void AzFile::seekReadBytes(AZint8 offs, AZint8 len, void *buff) 
{
  const char *eyec = "AzFile::seekReadBytes"; 
  /*-----  seek  -----*/
  if (offs >= 0) {
    if (fseek(fp, offs, SEEK_SET) != 0) { 
      throw new AzException(AzFileIOError, eyec, pointFileName(), "seek"); 
    }
  }

  /*-----  read  -----*/
  if (len > 0) {
    if (fread(buff, len, 1, fp) != 1) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "fread"); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzFile::seekReadBytes(AZint8 offs, AZint8 sz, AZint8 count, void *buff) 
{
  const char *eyec = "AzFile::seekReadBytes(offs,sz,count,buff)"; 
  /*-----  seek  -----*/
  if (offs >= 0) {
    if (fseek(fp, offs, SEEK_SET) != 0) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "seek"); 
    }
  }

  /*-----  read  -----*/   
  check_overflow(sz, "AzFile::seekReadBytes,sz"); 
  check_overflow(count, "AzFile::seekReadBytes,count");   
  if (sz > 0 && count > 0) {
    if (fread(buff, sz, count, fp) != count) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "fread"); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzFile::seek(AZint8 offs) 
{
  seekReadBytes(offs, 0, NULL); 
}

/*-------------------------------------------------------------*/
#define AZ_BIN_MARKER_INT 1
#define AZ_BIN_MARKER_DBL 1.0
void AzFile::writeBinMarker() 
{
  int int_val = AZ_BIN_MARKER_INT; 
  writeInt(int_val); 
  double dbl_val = AZ_BIN_MARKER_DBL; 
  writeDouble(dbl_val); 
}
void AzFile::checkBinMarker() 
{
  int int_val = 0;  
  double dbl_val = 0; 
  try {
    int_val = readInt(); 
    dbl_val = readDouble(); 
  }
  catch (AzException *e) {
    delete e; 
  }
  if (int_val != AZ_BIN_MARKER_INT || 
      dbl_val != AZ_BIN_MARKER_DBL) {
    throw new AzException(AzInputError, "AzFile::checkBinMarker", 
              "Binary file marker check failed: a broken file or endian mismatch:", 
              pointFileName()); 
  }
}

/* static */
/*------------------------------------------------------------------*/
void AzFile::scan(const char *fn, 
                  int buff_size, 
                  AzIntArr *ia_line_len, 
                  int max_line_num) /* -1: means don't care; added 06/15/2013 */
{
  AzFile file(fn); 
  file.open("rb"); 
  AzBytArr ba_buff; 
  AzByte *buff = ba_buff.reset(buff_size, 0); 
  
  /*---  find the number of lines and the maximum length of the lines  ---*/
  ia_line_len->reset(); 
  ia_line_len->prepare(1000000); 
  bool do_exit = false; 
  int remain = 0; 
  for ( ; ; ) {
    if (do_exit) break; 
    int len = file._readBytes(buff, buff_size); 
    if (len <= 0) break; 
    const char *wp = (char *)buff, *buff_end = (char *)(buff+len); 
    while(wp < buff_end) {
      const char *next_wp = strchr(wp, '\n'); 
      if (next_wp == NULL) {
        remain += (int)(buff_end - wp); 
        break; 
      }
      ++next_wp;  /* +1 for '\n' */
      
      int line_len = (int)(next_wp - wp) + remain; 
      ia_line_len->put(line_len);   
      remain = 0; 
      wp = next_wp; 
      if (max_line_num > 0 && ia_line_len->size() >= max_line_num) {
        do_exit = true; 
        break; 
      }  
    }
  }
  if (remain > 0) {
    ia_line_len->put(remain); 
  }
}

/***************************************************************/
/*              AzIIFarr (Int1, Int2, Floating-point)           */
/***************************************************************/
int az_compare_IIFarr_IntInt_A(const void *v1, const void *v2); 
int az_compare_IIFarr_IntInt_D(const void *v1, const void *v2); 
int az_compare_IIFarr_Int2Int1_A(const void *v1, const void *v2); 
int az_compare_IIFarr_Int2Int1_D(const void *v1, const void *v2); 
int az_compare_IIFarr_Float_A(const void *v1, const void *v2); 
int az_compare_IIFarr_Float_D(const void *v1, const void *v2); 
int az_compare_IIFarr_FloatInt1Int2_A(const void *v1, const void *v2); 
int az_compare_IIFarr_FloatInt1Int2_D(const void *v1, const void *v2); 
int az_compare_IIFarr_Int1_A(const void *v1, const void *v2);

/*------------------------------------------------------------------*/
int AzIIFarr::bsearch_Float(double key, bool isAscending) const
{
  if (ent_num <= 0 || ent == NULL) return -1; 

  AzIIFarrEnt *ptr = NULL; 
  if (isAscending) {
    ptr = (AzIIFarrEnt *)bsearch(&key, ent, ent_num, sizeof(ent[0]), 
                               az_compare_IIFarr_Float_A); 
  }
  else {
    ptr = (AzIIFarrEnt *)bsearch(&key, ent, ent_num, sizeof(ent[0]), 
                               az_compare_IIFarr_Float_D); 
  }
  if (ptr == NULL) return -1; 
  int ent_no = Az64::ptr_diff(ptr - ent, "AzIIFarr::bsearch_Float"); 
  return ent_no; 
}

/*------------------------------------------------------------------*/
double AzIIFarr::sum_Fval() const
{
  double sum = 0; 
  int ix; 
  for (ix = 0; ix < ent_num; ++ix) {
    sum += ent[ix].val; 
  }
  return sum; 
}

/*------------------------------------------------------------------*/
bool AzIIFarr::isSame(const AzIIFarr *iifq) const
{
  if (iifq->ent_num != ent_num) return false; 
  int ix; 
  for (ix = 0; ix < ent_num; ++ix) {
    if (ent[ix].int1 != iifq->ent[ix].int1 || 
        ent[ix].int2 != iifq->ent[ix].int2 || 
        ent[ix].val != iifq->ent[ix].val) {
      return false; 
    }
  }
  return true; 
}

/*------------------------------------------------------------------*/
int AzIIFarr::find(int int1, int int2, int first_ix) const
{
  int ix; 
  for (ix = first_ix; ix < ent_num; ++ix) {
    if (ent[ix].int1 == int1 && ent[ix].int2 == int2) {
      return ix; 
    }
  }
  return AzNone; 
}

/*-------------------------------------------------------------*/
double AzIIFarr::findMin(int *out_idx) const
{
  if (ent_num == 0) {
    if (out_idx != NULL) {
      *out_idx = AzNone; 
    }
    return -1; 
  }

  double min_val = ent[0].val; 
  int min_idx = 0; 
  int ix; 
  for (ix = 1; ix < ent_num; ++ix) {
    if (ent[ix].val < min_val) {
      min_idx = ix; 
      min_val = ent[ix].val; 
    }
  }
  if (out_idx != NULL) {
    *out_idx = min_idx; 
  }
  return min_val; 
}

/*-------------------------------------------------------------*/
double AzIIFarr::findMax(int *out_idx) const
{
  if (ent_num == 0) {
    if (out_idx != NULL) {
      *out_idx = AzNone; 
    }
    return -1; 
  }

  double max_val = ent[0].val; 
  int max_idx = 0; 
  int ix; 
  for (ix = 1; ix < ent_num; ++ix) {
    if (ent[ix].val > max_val) {
      max_idx = ix; 
      max_val = ent[ix].val; 
    }
  }
  if (out_idx != NULL) {
    *out_idx = max_idx; 
  }
  return max_val; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::concat(const AzIIFarr *iifq2) 
{
  const char *eyec = "AzIIFarr::concat"; 

  if (iifq2 == NULL || iifq2->ent_num <= 0) {
    return; 
  }

  int new_num = ent_num + iifq2->ent_num; 
  int ent_num_max = a.size(); 
  if (new_num > ent_num_max) {
    ent_num_max = new_num; 
    a.realloc(&ent, ent_num_max, eyec, "ent realloc"); 
  }

  memcpy(ent + ent_num, iifq2->ent, sizeof(ent[0]) * iifq2->ent_num);  
  ent_num = new_num; 
}

/*-------------------------------------------------------------*/
int AzIIFarr::getNum(double req_val) const
{
  int count = 0; 
  int ix; 
  for (ix = 0; ix < ent_num; ++ix) {
    if (ent[ix].val == req_val) ++count; 
  }
  return count; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::concat(const AzIIFarr *iifq2, double req_val) 
{
  const char *eyec = "AzIIFarr::concat(req_val)"; 

  if (iifq2 == NULL || iifq2->ent_num <= 0) {
    return; 
  }

  int new_num = ent_num + iifq2->ent_num; 
  int ent_num_max = a.size(); 
  if (new_num > ent_num_max) {
    ent_num_max = new_num; 
    a.realloc(&ent, ent_num_max, eyec, "ent realloc"); 
  }
  int ix; 
  for (ix = 0; ix < iifq2->ent_num; ++ix) {
    if (iifq2->ent[ix].val == req_val) {
      ent[ent_num] = iifq2->ent[ix]; 
      ++ent_num; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::update(int idx, 
                     int int1, int int2, 
                     double val)
{
  if (idx >= 0 && idx < ent_num) {
    ent[idx].int1 = int1; 
    ent[idx].int2 = int2; 
    ent[idx].val = val; 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::_read(AzFile *file) 
{
  const char *eyec = "AzIIFarr::_read (file)"; 
  ent_type = (AzIIFarrType)file->readInt(); 
  ent_num = file->readInt(); 
  a.alloc(&ent, ent_num, eyec, "ent"); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    AzIIFarrEnt *ep = &ent[ex]; 

    if (ent_type == AzIIFarr_II) {
      ep->int1 = file->readInt(); 
      ep->int2 = file->readInt(); 
      ep->val = 0; 
    }
    else if (ent_type == AzIIFarr_IF) {
      ep->int1 = file->readInt(); 
      ep->int2 = AzNone; 
      ep->val = file->readDouble(); 
    }
    else if (ent_type == AzIIFarr_IIF) {
      ep->int1 = file->readInt(); 
      ep->int2 = file->readInt(); 
      ep->val = file->readDouble(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::write(AzFile *file) 
{
  file->writeInt((int)ent_type); 
  file->writeInt(ent_num); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    const AzIIFarrEnt *ep = &ent[ex]; 
    if (ent_type == AzIIFarr_II) {
      file->writeInt(ep->int1); 
      file->writeInt(ep->int2); 
    }
    else if (ent_type == AzIIFarr_IF) {
      file->writeInt(ep->int1); 
      file->writeDouble(ep->val); 
    }
    else if (ent_type == AzIIFarr_IIF) {
      file->writeInt(ep->int1); 
      file->writeInt(ep->int2); 
      file->writeDouble(ep->val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::reset(int num, 
                    int int1, int int2, double val) 
{
  reset(); 
  prepare(num); 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    put(int1, int2, val); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::prepare(int num) 
{
  const char *eyec = "AzIIFarr::prepare";
  int ent_num_max = a.size(); 
  if (num >= ent_num && 
      num != ent_num_max) {
    ent_num_max = num; 
    a.realloc(&ent, ent_num_max, eyec, "ent"); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::cut(int new_num) 
{
  if (new_num < 0 || 
      ent_num <= new_num) {
    return; 
  }
  ent_num = new_num; 
  return; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::put(int int1, int int2, double val) 
{
  const char *eyec = "AzIIFarr::put";

  int ent_num_max = a.size(); 
  if (ent_num >= ent_num_max) {
    if (ent_num_max <= 0) 
      ent_num_max = 32; 
    else if (ent_num_max < 1024*1024) /* changed from 1024: 1/12/2014 */
      ent_num_max *= 2; 
    else
      ent_num_max += 1024*1024; /* changed from 1024: 1/12/2014 */

    a.realloc(&ent, ent_num_max, eyec, "2"); 
  }
  ent[ent_num].int1 = int1; 
  ent[ent_num].int2 = int2; 
  ent[ent_num].val = val; 
  ++ent_num; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::insert(int index, 
                    int int1, int int2, double fval)
{
  if (index < 0 || index > ent_num) {
    throw new AzException("AzIIFarr::insert", "invalid index"); 
  }

  /*---  append to the end to allocate memory if necessary  ---*/
  put(int1, int2, fval); 
  if (index == ent_num - 1) {
    return; /* already in position */
  }

  AzIIFarrEnt my_ent = ent[ent_num-1]; 

  /*---  make room for [index]  ---*/
  int ex; 
  for (ex = ent_num - 2; ex >= index; --ex) {
    ent[ex+1] = ent[ex]; 
  }

  /*---  insert  ---*/
  ent[index] = my_ent; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::int1(AzIntArr *ia_int1) const
{
  ia_int1->prepare(ent_num); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    ia_int1->put(ent[ex].int1); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::int2(AzIntArr *ia_int2) const
{
  ia_int2->prepare(ent_num); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    ia_int2->put(ent[ex].int2); 
  }
}

/*-------------------------------------------------------------*/
double AzIIFarr::get(int idx, int *int1, int *int2) const 
{
  if (idx < 0 || idx >= ent_num) {
    throw new AzException("AzIIFarr::get", "out of range"); 
  }
  if (int1 != NULL) {
    *int1 = ent[idx].int1; 
  }
  if (int2 != NULL) {
    *int2 = ent[idx].int2; 
  }

  return ent[idx].val; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Sum()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1 || 
          ent[ex].int2 != ent[ex1].int2) {
        break; 
      }
      ent[ex1].val += ent[ex].val; 
    }
    ++ex1; 
  }
  ent_num = ex1; 
}
 
/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Max()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }  
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1 || 
          ent[ex].int2 != ent[ex1].int2)  
        break; 

      ent[ex1].val = MAX(ent[ex1].val, ent[ex].val); 
    }
    ++ex1; 
  }
  ent_num = ex1; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Int1_Max()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int1_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }  
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1) 
        break; 

      if (ent[ex].val > ent[ex1].val) {
        ent[ex1] = ent[ex]; 
      }
    }
    ++ex1; 
  }
  ent_num = ex1; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Int1_Sum()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int1_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }  
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1) 
        break; 

      if (ent[ex].val > ent[ex1].val) {
        ent[ex1].int2 = ent[ex].int2; 
        ent[ex1].val += ent[ex].val; 
      }
    }
    ++ex1; 
  }
  ent_num = ex1; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_IntInt(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_D); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_Int2Int1(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int2Int1_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int2Int1_D); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_Float(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Float_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Float_D); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_FloatInt1Int2(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_FloatInt1Int2_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_FloatInt1Int2_D); 
  }
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_IntInt_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  if (p1->int2 < p2->int2) return -1; 
  if (p1->int2 > p2->int2) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_IntInt_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int1 > p2->int1) return -1; 
  if (p1->int1 < p2->int1) return 1; 
  if (p1->int2 > p2->int2) return -1; 
  if (p1->int2 < p2->int2) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Int2Int1_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int2 < p2->int2) return -1; 
  if (p1->int2 > p2->int2) return 1; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Int2Int1_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int2 > p2->int2) return -1; 
  if (p1->int2 < p2->int2) return 1; 
  if (p1->int1 > p2->int1) return -1; 
  if (p1->int1 < p2->int1) return 1; 
  return 0; 
}


/*-------------------------------------------------------------*/
int az_compare_IIFarr_Float_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val < p2->val) return -1; 
  if (p1->val > p2->val) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Float_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val > p2->val) return -1; 
  if (p1->val < p2->val) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_FloatInt1Int2_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val < p2->val) return -1; 
  if (p1->val > p2->val) return 1; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  if (p1->int2 < p2->int2) return -1; 
  if (p1->int2 > p2->int2) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_FloatInt1Int2_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val > p2->val) return -1; 
  if (p1->val < p2->val) return 1; 
  if (p1->int1 > p2->int1) return -1; 
  if (p1->int1 < p2->int1) return 1; 
  if (p1->int2 > p2->int2) return -1; 
  if (p1->int2 < p2->int2) return 1;
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Int1_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  return 0; 
}

/********************************************************************/
/*------------------------------------------------------------------*/
/*                             AzIIarr                              */
/*------------------------------------------------------------------*/
void AzIIarr::read(AzFile *file) 
{ 
  iifq.read(file); 
  if (iifq.get_ent_type() != AzIIFarr_II) {
    throw new AzException("AzIIarr::AzIIarr(file)", "type conflict");
  }
}

/*------------------------------------------------------------------*/
void AzIIarr::reset(const int pairs[][2], int pair_num)
{
  reset(); 
  prepare(pair_num); 
  int ix; 
  for (ix = 0; ix < pair_num; ++ix) {
    put(pairs[ix][0], pairs[ix][1]); 
  }
}

/*------------------------------------------------------------------*/
/*                             AzIFarr                              */
/*------------------------------------------------------------------*/
void AzIFarr::read(AzFile *file) 
{ 
  iifq.read(file); 
  if (iifq.get_ent_type() != AzIIFarr_IF) {
    throw new AzException("AzIFarr::_read", "type conflict");
  }
}

/********************************************************************/
/*                          AzIntArr                                */
/********************************************************************/
int az_compareInt_A(const void *v1, const void *v2); 
int az_compareInt_A(const void *v1, const void *v2); 

/*------------------------------------------------------------------*/
void AzIntArr::transfer_from(AzIntArr *inp)
{
  a.transfer_from(&inp->a, &ints, &inp->ints, "AzIntArr::transfer_from"); 
  num = inp->num; 
  inp->num = 0; 
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(int inp_num, int initial_value) 
{
  const char *eyec = "AzIntArr::initialize"; 

  if (inp_num <= 0) {
    return; 
  }
  a.alloc(&ints, inp_num, eyec, "ints"); 
  num = inp_num; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    ints[ix] = initial_value; 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::put(int int_val) 
{
  int num_max = a.size(); 
  if (num >= num_max) {
    _realloc(); 
  }
  ints[num] = int_val; 
  ++num; 
}

/*------------------------------------------------------------------*/
bool AzIntArr::toOnOff(const int *inp, int inp_num)
{
  bool isThereNegative = false; 
  int max_idx = -1; 
  int ix; 
  for (ix = 0; ix < inp_num; ++ix) {
    if (ix == 0 || inp[ix] > max_idx) max_idx = inp[ix]; 
    if (inp[ix] < 0) isThereNegative = true; 
  }
  if (max_idx < 0) {
    reset(); 
    return isThereNegative; 
  }

  reset(max_idx+1, false); 
  for (ix = 0; ix < inp_num; ++ix) {
    int idx = inp[ix]; 
    if (idx >= 0) {
      ints[idx] = true; 
    }
  }
  return isThereNegative; 
}

/*------------------------------------------------------------------*/
bool AzIntArr::toCount(AzIntArr *out_iq) const
{
  bool isThereNegative = false; 
  int max_idx = max(); 
  out_iq->reset(max_idx+1, 0); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int idx = ints[ix]; 
    if (idx < 0) {
      isThereNegative = true; 
      continue; 
    } 
    ++out_iq->ints[idx]; 
  }
  return isThereNegative; 
}

/*------------------------------------------------------------------*/
int AzIntArr::compare(const AzIntArr *iq1, const AzIntArr *iq2, 
                         int first_k)
{
  int cmp_num = MIN(iq1->num, iq2->num); 
  if (first_k > 0) {
    cmp_num = MIN(cmp_num, first_k); 
  }

  int ix; 
  for (ix = 0; ix < cmp_num; ++ix) {
    int cmp = iq1->ints[ix] - iq2->ints[ix]; 
    if (cmp != 0) {
      if (cmp < 0) return -1; 
      else if (cmp > 0) return 1; 
      return 0; 
    }
  }
  if (first_k > 0 && 
      cmp_num == first_k) { 
    return 0; 
  }
  int cmp = iq1->num - iq2->num; 
  if (cmp < 0) return -1; 
  else if (cmp > 0) return 1; 
  return 0; 
}

/*------------------------------------------------------------------*/
void AzIntArr::changeOrder(const AzIntArr *ia_old2new) 
{
  int old2new_num = ia_old2new->num; 
  const int *old2new = ia_old2new->ints; 
  if (old2new_num != num) {
    throw new AzException("AzIntArr::changeOrder", "number mismatch"); 
  }

  AzIntArr ia_org(this); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    ints[ix] = -1; 
  }
  for (ix = 0; ix < num; ++ix) {
    int new_ix = old2new[ix]; 
    ints[new_ix] = ia_org.ints[ix]; 
  }
}

/*------------------------------------------------------------------*/
double AzIntArr::average() const {
  if (num <= 0) return 0; 
  double sum = 0; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    sum += ints[ix]; 
  }
  return sum / (double)num; 
}

/*------------------------------------------------------------------*/
void AzIntArr::prepare(int prep_num)
{
  const char *eyec = "AzIntArr::prepare"; 
  int num_max = a.size(); 
  if (num_max < prep_num) {
    num_max = prep_num; 
    a.realloc(&ints, num_max, eyec, "ints"); 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(const AzIntArr *inp_ia) 
{
  if (inp_ia == NULL) return; 

  int inp_num; 
  const int *inp_ints = inp_ia->point(&inp_num); 
  initialize(inp_ints, inp_num); 
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(const int *inp_ints, int inp_ints_num) 
{
  if (inp_ints_num <= 0) {
    return; 
  }

  num = inp_ints_num; 
  if (num > 0) {
    a.alloc(&ints, num, "AzIntArr::initialize(array,num)", "ints"); 
  }
  memcpy(ints, inp_ints, sizeof(inp_ints[0]) * inp_ints_num); 
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(AzFile *file)  
{
  num = file->readInt(); 
  initialize(num, 0); 

  file->seekReadBytes(-1, sizeof(ints[0])*num, ints); 
  _swap(); 
}

/*------------------------------------------------------------------*/
void AzIntArr::_swap()
{
  if (!isSwapNeeded) return; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    AzFile::swap_int4(&ints[ix]); 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::write(AzFile *file) 
{
  file->writeInt(num); 
  _swap(); 
  file->writeBytes(ints, sizeof(ints[0])*num); 
  _swap(); 
}

/*------------------------------------------------------------------*/
void AzIntArr::reset(int num, int initial_value) 
{
  reset(); 
  initialize(num, initial_value); 
}

/*------------------------------------------------------------------*/
void AzIntArr::fill(int num, int first_value)
{
  reset(); 
  initialize(num, first_value); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    ints[ix] = first_value + ix; 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::cut(int new_num) 
{
  if (num > new_num) {
    num = new_num; 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::unique() 
{
  if (ints == NULL || num <= 0) {
    return; 
  }

  qsort(ints, num, sizeof(ints[0]), az_compareInt_A);   

  int ix1 = 0; 
  int ix; 
  for (ix = 0; ix < num; ) {
    if (ix != ix1) {
      ints[ix1] = ints[ix]; 
    }

    ++ix; 
    for ( ; ix < num; ++ix) {
      if (ints[ix] != ints[ix1]) 
        break;
    }

    ++ix1; 
  }
  num = ix1; 
}

/*------------------------------------------------------------------*/
void AzIntArr::increment(int index) 
{
  checkIndex(index, "AzIntArr::increment"); 
  ++ints[index]; 
}

/*------------------------------------------------------------------*/
void AzIntArr::_realloc()
{
  int num_max = a.size(); 
  if (num_max <= 0) {
    num_max = 32; 
  }
  else if (num_max < 1024 * 1024) {
    num_max *= 2; 
  }
  else {
    num_max += 1024 * 1024; 
  }

  a.realloc(&ints, num_max, "AzIntArr::_realloc", "ints"); 
}

/*------------------------------------------------------------------*/
void AzIntArr::concat(const AzIntArr *ia2) 
{
  concat(ia2->ints, ia2->num); 
}

/*------------------------------------------------------------------*/
void AzIntArr::concat(const int *ints2, int ints2_num)
{
  if (ints2 == NULL || ints2_num == 0) {
    return; 
  }

  int new_num = num + ints2_num; 
  int num_max = a.size(); 
  if (new_num > num_max) {
    num_max = (new_num+1023)/1024*1024; 
    a.realloc(&ints, num_max, "AzIntArr::concat", "ints"); 
  }

  int ix; 
  for (ix = 0; ix < ints2_num; ++ix) {
    ints[num + ix] = ints2[ix]; 
  }
  num = new_num; 
}

/*------------------------------------------------------------------*/
void AzIntArr::remove_by_value(int val)
{
  int ix1 = 0; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    if (ints[ix] != val) {
      if (ix1 != ix) ints[ix1] = ints[ix]; 
      ++ix1; 
    }
  }
  num = ix1; 
}

/*------------------------------------------------------------------*/
/* static */
int AzIntArr::count_nonnegative(const int *ints, int num) 
{
  int count = 0; 
  int ix; 
  for (ix = 0; ix < num; ++ix) if (ints[ix] >= 0) ++count; 
  return count; 
}

/*------------------------------------------------------------------*/
void AzIntArr::remove(int idx)
{
  if (idx < 0 || idx >= num) {
    return; 
  }

  int ix; 
  for (ix = idx + 1; ix < num; ++ix) {
    ints[ix-1] = ints[ix]; 
  }
  --num; 
}

/*------------------------------------------------------------------*/
void AzIntArr::insert(int idx, int val)
{
  int num_max = a.size(); 
  if (num >= num_max) {
    _realloc(); 
  }

  int ix; 
  for (ix = num - 1; ix >= idx; --ix) {
    ints[ix+1] = ints[ix]; 
  }

  ints[idx] = val; 
  ++num; 
}

/*------------------------------------------------------------------*/
int az_compareInt_A(const void *v1, const void *v2) 
{
  return (*((int *)v1) - *((int *)v2)); 
}

/*------------------------------------------------------------------*/
int az_compareInt_D(const void *v1, const void *v2) 
{
  return (*((int *)v2) - *((int *)v1)); 
}

/*------------------------------------------------------------------*/
void AzIntArr::sort(bool ascending)
{
  if (num <= 1) {
    return; 
  }

  if (ascending) {
    qsort(ints, num, sizeof(ints[0]), az_compareInt_A); 
  }
  else {
    qsort(ints, num, sizeof(ints[0]), az_compareInt_D); 
  }
}

/*------------------------------------------------------------------*/
int *AzIntArr::my_bsearch(int key)
{
  if (num <= 0 || ints == NULL) return NULL; 
  return (int *)bsearch(&key, ints, num, sizeof(ints[0]), az_compareInt_A); 
}

/*------------------------------------------------------------------*/
void AzIntArr::print(const AzOut &out, const char *header) const 
{
  if (out.isNull()) return; 
  AzPrint o(out); 
  o.printBegin(header, ""); 
  o.print("("); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    o.print(ints[ix]); 
    if (ix == 0) o.resetDlm(","); 
  }
  o.printEnd(")"); 
}

/*-------------------------------------------------------------*/
int AzIntArr::count(int int_val) const 
{
  if (ints == NULL) return 0; 
  int out = 0; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    if (ints[ix] == int_val) ++out; 
  }
  return out; 
}

/*-------------------------------------------------------------*/
int AzIntArr::replace(int from_val, int to_val) 
{
  if (ints == NULL) return 0; 
  int count = 0; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    if (ints[ix] == from_val) {
      ints[ix] = to_val; 
      ++count; 
    }
  }
  return count; 
}

/***************************************************************/
/***************************************************************/
/*                       AzBytArr                              */
/***************************************************************/
/*-------------------------------------------------------------*/
void AzBytArr::transfer_from(AzBytArr *inp)
{
  a.transfer_from(&inp->a, &bytes_long, &inp->bytes_long, "AzBytArr::transfer_from"); 
  bytes_long = inp->bytes_long; 
  memcpy(bytes_short, inp->bytes_short, sizeof(bytes_short)); 
  len = inp->len;  /* added 9/17/2011 */
}

/*-------------------------------------------------------------*/
void AzBytArr::concatInt(int number, 
                        int width, 
                        bool doFillWithZero)
{
  stringstream s; 
  if (width > 0) s.width(width); 
  if (doFillWithZero) s.fill('0'); 
  s << number; 
  if (width > 0) {
    concat(s.str().c_str()); 
    return; 
  }

  /*---  remove leading/tailing space though I don't know if there is any  ---*/
  if (getLen() == 0) {
    concat(s.str().c_str()); 
    strip(); 
  }
  else {
    AzBytArr str_temp(s.str().c_str()); 
    str_temp.strip(); 
    concat(&str_temp); 
  }
}

/*-------------------------------------------------------------*/
void AzBytArr::concatInt8(AZint8 number, 
                        int width, 
                        bool doFillWithZero)
{
  stringstream s; 
  if (width > 0) s.width(width); 
  if (doFillWithZero) s.fill('0'); 
  s << number; 
  if (width > 0) {
    concat(s.str().c_str()); 
    return; 
  }

  /*---  remove leading/tailing space though I don't know if there is any  ---*/
  if (getLen() == 0) {
    concat(s.str().c_str()); 
    strip(); 
  }
  else {
    AzBytArr str_temp(s.str().c_str()); 
    str_temp.strip(); 
    concat(&str_temp); 
  }
}

/*-------------------------------------------------------------*/
void AzBytArr::concatFloat(double number, 
                          int precision, 
                          bool doScientific)
{
  stringstream s; 
  if (precision > 0) {
    s.precision(precision); 
  }
  if (doScientific) s << scientific; 
  s << number; 

  /*---  remove leading/tailing space though I don't know if there is any  ---*/
  if (getLen() == 0) {
    concat(s.str().c_str()); 
    strip(); 
  }
  else {
    AzBytArr str_temp(s.str().c_str()); 
    str_temp.strip(); 
    concat(&str_temp); 
  }
}

/*--------------------------------------------------------*/
void AzBytArr::replace(const char *old_str, 
                       const char *new_str)
{
  const char *inp_str = c_str(); 
  const char *ptr = strstr(inp_str, old_str); 
  if (ptr == NULL) return; 

  int old_str_len = Az64::cstrlen(old_str); 
  int front_len = Az64::ptr_diff(ptr - inp_str, "AzBytArr::replace"); 
  int back_len = length() - front_len - old_str_len; 

  AzBytArr new_s; 
  new_s.concat(inp_str, front_len); 
  new_s.concat(new_str); 
  new_s.concat(ptr+old_str_len, back_len); 
  reset(&new_s); 
}

/* assume ascii */
/*-------------------------------------------------------------*/
void AzBytArr::strip()
{
  int len; 
  const AzByte *data = point(&len); 
  const AzByte *bp = data, *data_end = data + len; 
  for ( ; bp < data_end; ++bp) {
    if (*bp > 0x20) break; 
  }
  const AzByte *ep = data_end; 
  for ( ; ep-1 >= data; --ep) {
    if (*(ep-1) > 0x20) break; 
  }
  if (bp >= ep) {
    clear(); 
  }
  else if (bp == data && ep == data_end); /* nop */
  else {
    AzBytArr str_temp(bp, Az64::ptr_diff(ep - bp, "AzBytArr::strip")); 
    clear(); 
    concat(&str_temp); 
  }
}

/*-------------------------------------------------------------*/
void AzBytArr::print(const AzByte *inp_bytes, int inp_bytes_len, const AzOut &out) 
{
  if (out.isNull()) return; 
  int ix; 
  for (ix = 0; ix < inp_bytes_len; ++ix) {
    *out.o << inp_bytes[ix]; 
    /* fprintf(out.fp, "%c", inp_bytes[ix]); */
  }
}

/*-------------------------------------------------------------*/
void AzBytArr::print(const AzOut &out) const
{
  int my_bytes_len; 
  const AzByte *my_bytes = point(&my_bytes_len); 
  print(my_bytes, my_bytes_len, out); 
}
 
/*-------------------------------------------------------------*/
void AzBytArr::replace(char old_char, char new_char) 
{
  if (old_char == '\0') {
    throw new AzException("AzBytArr::replace", "can't replace null char"); 
  }
  if (bytes_long != NULL) {
    int ix; 
    for (ix = 0; ix < len; ++ix) {
      if (*(bytes_long+ix) == old_char) *(bytes_long+ix) = new_char; 
    }
  }
  else {
    int ix; 
    for (ix = 0; ix < MIN(AZI_BYTES_SHORT_LEN, len); ++ix) {
      if (bytes_short[ix] == old_char) bytes_short[ix] = new_char; 
    }
  }
}

/*-------------------------------------------------------------*/
AzBytArr::AzBytArr()
{
  initialize(); 
}

/*-------------------------------------------------------------*/
AzBytArr::AzBytArr(const AzBytArr *inp_byteq) 
{
  initialize(); 
  initialize(inp_byteq); 
}

/*-------------------------------------------------------------*/
AzBytArr::AzBytArr(const AzBytArr &inp_byteq) 
{
  initialize(); 
  initialize(&inp_byteq); 
}

/*-------------------------------------------------------------*/
void AzBytArr::initialize(const AzBytArr *inp_byteq) 
{
  if (inp_byteq == NULL) {
    return; 
  }

  int bytes_len; 
  const AzByte *bytes = inp_byteq->point(&bytes_len); 
  initialize(bytes, bytes_len); 
}

/*-------------------------------------------------------------*/
AzBytArr::AzBytArr(const AzByte *bytes, int bytes_len) 
{
  initialize(); 
  initialize(bytes, bytes_len); 
}

/*-------------------------------------------------------------*/
AzBytArr::AzBytArr(const char *cstr) 
{
  initialize(); 
  const AzByte *bytes = NULL; 
  int bytes_len = 0; 
  if (cstr != NULL) {
    bytes = (AzByte *)cstr; 
    bytes_len = Az64::cstrlen(cstr, "AzBytArr::AzBytArr"); 
  }

  initialize(bytes, bytes_len); 
}

/*-------------------------------------------------------------*/
AzBytArr::AzBytArr(int number)
{
  initialize(); 
  concatInt(number); 
}

/*-------------------------------------------------------------*/
void AzBytArr::initialize()
{
  len = 0; 
  bytes_long = NULL; 
  bytes_short[0] = 0; 
}

/*-------------------------------------------------------------*/
void AzBytArr::initialize(const AzByte *bytes, int bytes_len) 
{
  const char *eyec = "AzBytArr::initialize"; 

  if (bytes == NULL || bytes_len <= 0) {
    return; 
  }

  len = bytes_len; 
  if (len <= AZI_BYTES_SHORT_LEN) {
    memcpy(bytes_short, bytes, bytes_len); 
    *(bytes_short + bytes_len) = 0; 
  }
  else {
    a.alloc(&bytes_long, len + 1, eyec, "bytes_long"); 
    memcpy(bytes_long, bytes, bytes_len);
    *(bytes_long + bytes_len) = 0; 
  }
}

/*-------------------------------------------------------------*/
AzByte *AzBytArr::reset(int bytes_len, AzByte val) 
{
  const char *eyec = "AzBytArr::reset(len,val)"; 

  reset(); 
  if (bytes_len <= 0) {
    return point_u(); 
  }

  len = bytes_len; 
  if (len <= AZI_BYTES_SHORT_LEN) {
    memset(bytes_short, val, len); 
    *(bytes_short + len) = 0; 
  }
  else {
    a.alloc(&bytes_long, len + 1, eyec, "bytes_long"); 
    memset(bytes_long, val, len);
    *(bytes_long + len) = 0; 
  }
  return point_u(); 
}

/*-------------------------------------------------------------*/
void AzBytArr::update(int offs, AzByte new_val) 
{
  if (offs < 0 || offs >= len) {
    throw new AzException("AzBytArr::update", "out of range"); 
  }

  if (bytes_long != NULL) {
    bytes_long[offs] = new_val; 
  }
  else {
    bytes_short[offs] = new_val; 
  }
}

/*-------------------------------------------------------------*/
AzBytArr::AzBytArr(AzFile *file) 
{
  initialize(); 
  _read(file); 
}

/*-------------------------------------------------------------*/
void AzBytArr::read(AzFile *file)
{
  reset(); 
  _read(file); 
}

/*-------------------------------------------------------------*/
void AzBytArr::_read(AzFile *file) 
{
  len = file->readInt(); 
  if (len > 0) {
    if (len <= AZI_BYTES_SHORT_LEN) {
      file->seekReadBytes(-1, len + 1, bytes_short); 
    }
    else {
      a.alloc(&bytes_long, len + 1, "AzBytArr::_read", "bytes_long"); 
      file->seekReadBytes(-1, len + 1, bytes_long); 
      /* bytes_long = (AzByte *)file->readBytes(len + 1); */
    }
  }
}

/*-------------------------------------------------------------*/
void AzBytArr::write(AzFile *file) 
const
{
  file->writeInt(len); 

  if (len > 0) {
    if (bytes_long != NULL) {
      file->writeBytes(bytes_long, len + 1); 
    }
    else {
      file->writeBytes(bytes_short, len + 1); 
    }
  }
}

/*-------------------------------------------------------------*/
int AzBytArr::compare(const AzBytArr *byteq2) const
{
  int bytes2_len; 
  const AzByte *bytes2 = byteq2->point(&bytes2_len); 
  return compare(bytes2, bytes2_len); 
}

/*-------------------------------------------------------------*/
int AzBytArr::compare(const AzByte *bytes2, int bytes2_len) const
{
  int bytes1_len; 
  const AzByte *bytes1 = point(&bytes1_len); 
  int cmp = memcmp(bytes1, bytes2, MIN(bytes1_len, bytes2_len)); 
  if (cmp != 0) return cmp; 

  if      (bytes1_len < bytes2_len) return -1; 
  else if (bytes1_len > bytes2_len) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
bool AzBytArr::beginsWith(AzBytArr *byteq2) const
{
  if (byteq2 == NULL) {
    return true; 
  }

  int bytes1_len, bytes2_len; 
  const AzByte *bytes1 = point(&bytes1_len); 
  const AzByte *bytes2 = byteq2->point(&bytes2_len); 
  if (bytes2_len == 0) {
    true; 
  }

  if (bytes1_len < bytes2_len) {
    return false; 
  }

  if (memcmp(bytes1, bytes2, bytes2_len) == 0) {
    return true; 
  }
  return false; 
}

/*-------------------------------------------------------------*/
bool AzBytArr::endsWith(AzBytArr *byteq2) const
{
  if (byteq2 == NULL) {
    return true; 
  }

  int bytes1_len, bytes2_len;  
  const AzByte *bytes1 = point(&bytes1_len); 
  const AzByte *bytes2 = byteq2->point(&bytes2_len); 
  if (bytes2_len <= 0) {
    return true; 
  }

  if (bytes1_len < bytes2_len) {
    return false; 
  }

  if (memcmp(bytes1 + bytes1_len - bytes2_len, bytes2, bytes2_len) == 0) {
    return true; 
  }
  return false; 
}

/*-------------------------------------------------------------*/
bool AzBytArr::contains(const char *str) const
{
  const char *str0 = c_str();  
  if (strstr(str0, str) != NULL) return true; 
  return false; 
}

/*-------------------------------------------------------------*/
bool AzBytArr::contains(AzByte ch) const
{
  const char *str0 = c_str();  
  if (strchr(str0, ch) != NULL) return true; 
  return false; 
}

/*-------------------------------------------------------------*/
void AzBytArr::prepare_inc(int inc)
{
  int max_len = a.size() + MAX(0, inc); 
  a.realloc(&bytes_long, max_len, "AzBytArr::prepare_inc", "bytes_long"); 
}

/*-------------------------------------------------------------*/
void AzBytArr::concat(const AzByte *inp_bytes, int inp_len) 
{
  const char *eyec = "AzBytArr::concat"; 

  if (inp_bytes == NULL || inp_len <= 0) 
    return; 

  if (len + inp_len <= AZI_BYTES_SHORT_LEN) {
    memcpy(bytes_short + len, inp_bytes, inp_len); 
    *(bytes_short + len + inp_len) = 0; 
  }
  else {
    int max_len = a.size(); 
    int req_len = len + inp_len + 1; 
    if (max_len < req_len) {
      int my_len; 
      if (req_len < 4096) my_len = MAX(req_len, MIN(4096, max_len*2)); 
      else                my_len = (req_len+4095)/4096*4096; 
      a.realloc(&bytes_long, my_len, eyec, "bytes_long"); 
    }

    if (len <= AZI_BYTES_SHORT_LEN) {
      memcpy(bytes_long, bytes_short, len); 
    }
    memcpy(bytes_long + len, inp_bytes, inp_len);     
    *(bytes_long + len + inp_len) = 0; 
  }
  len += inp_len; 
}

/*-------------------------------------------------------------*/
void AzBytArr::concat(AzByte one_byte)
{
  concat(&one_byte, 1); 
}

/*-------------------------------------------------------------*/
AzByte *AzBytArr::point_u()
{
  if (bytes_long != NULL) {
    return bytes_long; 
  }
  else {
    return bytes_short; 
  }
}

/*-------------------------------------------------------------*/
const AzByte *AzBytArr::point() const
{
  if (bytes_long != NULL) {
    return bytes_long; 
  }
  else {
    return bytes_short; 
  }
}

/*-------------------------------------------------------------*/
const AzByte *AzBytArr::point(int *out_len) const
{
  *out_len = len; 

  if (bytes_long != NULL) {
    return bytes_long; 
  }
  else {
    return bytes_short; 
  }
}

/*-------------------------------------------------------------*/
int AzBytArr::getLen() const
{
  return len; 
}

/*-------------------------------------------------------------*/
AzBytArr::~AzBytArr()
{
  a.free(&bytes_long); len = 0; 
}

/*-------------------------------------------------------------*/
void AzBytArr::reset()
{
  a.free(&bytes_long); 
  bytes_short[0] = 0; 
  len = 0; 
}

/*-------------------------------------------------------------*/
void AzBytArr::lwr() 
{
  char *c;
  if (bytes_long != NULL) {
    c = (char *)bytes_long;
  }
  else {
    c = (char *)bytes_short; 
  }
  while (*c) {
    *c = (char)tolower((int)*c);
    c++;
  }
}

/***************************************************************/
/*                      AzTimeLog                             */
/***************************************************************/
void AzTimeLog::print(const char *msg, const AzOut &out) 
{
  if (out.isNull()) return; 
  _printTime(out); 
  AzPrint::writeln(out, msg); 
  out.flush(); 
}

/*-------------------------------------------------------------*/
void AzTimeLog::print(const char *msg1, const char *msg2, const AzOut &out) 
{
  if (out.isNull()) return; 
  _printTime(out); 
  AzPrint::write(out, msg1); 
  AzPrint::writeln(out, msg2); 
  out.flush(); 
}

/*-------------------------------------------------------------*/
void AzTimeLog::_printTime(const AzOut &out)
{
  if (out.isNull()) return; 
  time_t ltime; 
  time(&ltime); 
  AzBytArr str_time(ctime(&ltime)); 
  str_time.strip(); /* remove new line in the end */
  str_time.c(": "); 
  AzPrint::write(out, str_time); 
}
