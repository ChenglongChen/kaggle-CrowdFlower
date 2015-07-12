/* * * * *
 *  AzSmat.cpp 
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
#include "AzSmat.hpp"
#include "AzPrint.hpp"

#define AzVectSmall 32

/*-------------------------------------------------------------*/
void AzSmat::initialize(int inp_row_num, int inp_col_num, bool asDense) 
{
  const char *eyec = "AzSmat::initialize (asDense)"; 
  if (inp_col_num < 0 || inp_row_num < 0) {
    throw new AzException(eyec, "#column and #row must be non-negative"); 
  }
  if (column != NULL || col_num > 0) {
    throw new AzException(eyec, "_release() must be called before this"); 
  }
  col_num = inp_col_num; 
  row_num = inp_row_num; 
  a.alloc(&column, col_num, eyec, "column"); 
  if (asDense) {
    int cx; 
    for (cx = 0; cx < col_num; ++cx) {
      column[cx] = new AzSvect(this->row_num, asDense); 
    }
  }
  dummy_zero.reform(row_num); 
}

/*-------------------------------------------------------------*/
void AzSmat::initialize(const AzSmat *inp) 
{
  if (inp == NULL) {
    throw new AzException("AzSmat::initialize(AzSmat*)", "null input"); 
  }
  bool asDense = false; 
  initialize(inp->row_num, inp->col_num, asDense); 
  if (inp->column != NULL) {
    int cx;
    for (cx = 0; cx < this->col_num; ++cx) {
      if (inp->column[cx] != NULL) {
        column[cx] = new AzSvect(inp->column[cx]); 
      }
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::reform(int row_num, int col_num, bool asDense)
{
  _release(); 
  initialize(row_num, col_num, asDense); 
}

/*-------------------------------------------------------------*/
void AzSmat::resize(int new_col_num) 
{
  const char *eyec = "AzSmat::resize"; 
  if (new_col_num == col_num) {
    return; 
  } 
  if (new_col_num < 0) {
    throw new AzException(eyec, "new #columns must be positive"); 
  }
  a.realloc(&column, new_col_num, eyec, "column"); 
  col_num = new_col_num; 
}

/*-------------------------------------------------------------*/
void AzSmat::resize(int new_row_num, int new_col_num) 
{
  resize(new_col_num); 

  if (column != NULL) {
    int col; 
    for (col = 0; col < col_num; ++col) {
      if (column[col] != NULL) {
        column[col]->resize(new_row_num); 
      } 
    }
  }
  dummy_zero.resize(new_row_num); 
  row_num = new_row_num; 
}

/*-------------------------------------------------------------*/
void AzSvect::resize(int new_row_num) 
{
  if (new_row_num < row_num) {
    throw new AzException("AzSvect::resize", "no support for shrinking"); 
  }
  row_num = new_row_num; 
}

/*-------------------------------------------------------------*/
void AzSmat::set(const AzReadOnlyMatrix *inp) 
{
  _release(); 
  bool asDense = false; 
  initialize(inp->rowNum(), inp->colNum(), asDense); 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    delete column[cx]; column[cx] = NULL; 
    const AzReadOnlyVector *v = inp->col(cx); 
    if (!v->isZero()) {
      column[cx] = new AzSvect(v); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::set(const AzSmat *inp)  
{
  if (inp->row_num != this->row_num || 
      inp->col_num != this->col_num) {
    reform(inp->row_num, inp->col_num); 
  }
  int cx; 
  for (cx = 0; cx < this->col_num; ++cx) {
    if (inp->column[cx] == NULL) {
      delete column[cx]; 
      column[cx] = NULL; 
    }
    else {
      if (column[cx] == NULL) {
        column[cx] = new AzSvect(row_num); 
      }
      column[cx]->set(inp->column[cx]); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::set(const AzSmat *inp, int col0, int col1)  
{
  if (col0 < 0 || col1 < 0 || col1 > inp->col_num || col0 >= col1) {
    throw new AzException("AzSmat::set(inp,col0,col1)", "out of range"); 
  }
  if (inp->row_num != this->row_num || 
      inp->col_num != col1-col0) {
    reform(inp->row_num, col1-col0); 
  }
  int cx; 
  for (cx = col0; cx < col1; ++cx) {
    int my_cx = cx - col0; 
    if (inp->column[cx] == NULL) {
      delete column[my_cx]; 
      column[my_cx] = NULL; 
    }
    else {
      if (column[my_cx] == NULL) {
        column[my_cx] = new AzSvect(row_num); 
      }
      column[my_cx]->set(inp->column[cx]); 
    }
  }
}

/*-------------------------------------------------------------*/
int AzSmat::set(const AzSmat *inp, const int *cols, int cnum,  /* new2old */
                bool do_zero_negaindex) 
{
  if (row_num != inp->row_num || col_num != cnum) {
    reform(inp->row_num, cnum); 
  }
  int negaindex = 0; 
  int my_col; 
  for (my_col = 0; my_col < cnum; ++my_col) {
    int col = cols[my_col]; 
    if (col < 0 && do_zero_negaindex) {
      delete column[my_col]; 
      column[my_col] = NULL;     
      ++negaindex; 
      continue; 
    }   
    if (col < 0 || col >= inp->col_num) {
      throw new AzException("AzSmat::set(inp,cols,cnum)", "invalid col#"); 
    }
    if (inp->column[col] == NULL) {
      delete column[my_col]; 
      column[my_col] = NULL; 
    }
    else {
      if (column[my_col] == NULL) {
        column[my_col] = new AzSvect(row_num); 
      }
      column[my_col]->set(inp->column[col]); 
    }
  }
  return negaindex; 
}

/*-------------------------------------------------------------*/
void AzSmat::set(int col0, int col1, const AzSmat *inp, int i_col0)
{
  const char *eyec = "AzSmat::set(col0,col1,inp)"; 
  if (col0 < 0 || col1-col0 <= 0 || col1 > col_num) {
    throw new AzException(eyec, "requested columns are out of range"); 
  }
  int i_col1 = i_col0 + (col1-col0); 
  if (i_col0 < 0 || i_col1 > inp->col_num) {
    throw new AzException(eyec, "requested columns are out of range in the input matrix"); 
  }
  if (row_num != inp->row_num) {
    throw new AzException(eyec, "#rows mismatch"); 
  }
  int i_col = i_col0; 
  int col; 
  for (col = col0; col < col1; ++col, ++i_col) {
    if (inp->column[i_col] == NULL) {
      delete column[col]; column[col] = NULL; 
    }
    else {
      if (column[col] == NULL) {
        column[col] = new AzSvect(row_num); 
      }
      column[col]->set(inp->column[i_col]);       
    }
  }
}             
                
/*-------------------------------------------------------------*/
void AzSmat::reduce(const int *cols, int cnum)  /* new2old; must be sorted */
{
  if (column == NULL) {
    reform(row_num, cnum); 
    return; 
  }

  int negaindex = 0; 
  int new_col; 
  for (new_col = 0; new_col < cnum; ++new_col) {
    int old_col = cols[new_col]; 
    if (old_col < 0 || old_col >= col_num) {
      throw new AzException("AzSmat::reduce(cols,cnum)", "invalid col#"); 
    }
    if (new_col > 0 && old_col <= cols[new_col-1]) {
      throw new AzException("AzSmat::reduce(cols,cnum)", "column# must be sorted"); 
    }
    
    if (old_col == new_col) {}
    else if (column[old_col] == NULL) {
      delete column[new_col]; 
      column[new_col] = NULL; 
    }
    else {
      if (column[new_col] == NULL) {
        column[new_col] = new AzSvect(row_num); 
      }
      column[new_col]->set(column[old_col]); 
    }
  }
  resize(cnum); 
}

/*-------------------------------------------------------------*/
void AzSmat::transpose(AzSmat *m_out, 
                       int col_begin, int col_end) const
{
  int col_b = col_begin, col_e = col_end; 
  if (col_b < 0) {
    col_b = 0; 
    col_e = col_num; 
  }
  else {
    if (col_b >= col_num || 
        col_e < 0 || col_e > col_num || 
        col_e - col_b <= 0) {
      throw new AzException("AzSmat::transpose", "column range error"); 
    }
  }

  _transpose(m_out, col_b, col_e); 
}

/*-------------------------------------------------------------*/
void AzSmat::_transpose(AzSmat *m_out, 
                        int col_begin, 
                        int col_end) const
{
  int row_num = rowNum(); 

  m_out->reform(col_end - col_begin, row_num); 

  AzIntArr ia_row_count; 
  ia_row_count.reset(row_num, 0); 
  int *row_count = ia_row_count.point_u(); 

  int cx; 
  for (cx = col_begin; cx < col_end; ++cx) {
    /* rewind(cx); */
    AzCursor cursor; 
    for ( ; ; ) {
      double val; 
      int rx = next(cursor, cx, val); 
      if (rx < 0) break;

      ++row_count[rx]; 
    }
  }
  int rx; 
  for (rx = 0; rx < row_num; ++rx) {
    if (row_count[rx] > 0) {
      m_out->col_u(rx)->clear_prepare(row_count[rx]); 
    }
  }

  for (cx = col_begin; cx < col_end; ++cx) {
    /* rewind(cx); */
    AzCursor cursor; 
    for ( ; ; ) {
      double val; 
      int rx = next(cursor, cx, val); 
      if (rx < 0) break;

      m_out->col_u(rx)->set_inOrder(cx - col_begin, val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::clear_prepare(int num)
{
  if (num > row_num) {
    throw new AzException("AzSvect::prepare", "input is too large"); 
  }  
  clear(); 

  elm_num = 0; 

  if (num > 0) {
    int elm_num_max = num; 
    a.alloc(&elm, elm_num_max, "AzSvect::prepare", "elm"); 
  }  
}

/*-------------------------------------------------------------*/
void AzSmat::_read(AzFile *file) 
{
  const char *eyec = "AzSmat::_read(file)"; 
  if (col_num > 0 || column != NULL) {
    throw new AzException(eyec, "occupied"); 
  }
  col_num = file->readInt(); 
  row_num = file->readInt(); 
  if (col_num > 0) {
    a.alloc(&column, col_num, eyec, "column"); 
    int cx; 
    for (cx = 0; cx < col_num; ++cx) {
      column[cx] = AzObjIOTools::read<AzSvect>(file); 
    }
  }
  dummy_zero.reform(row_num); 
}

/*-------------------------------------------------------------*/
void AzSmat::write(AzFile *file) 
{
  file->writeInt(col_num); 
  file->writeInt(row_num); 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    AzObjIOTools::write(column[cx], file); 
  }
}

/*-------------------------------------------------------------*/
bool AzSmat::isZero() const
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL && 
        !column[cx]->isZero()) {
      return false; 
    }
  }
  return true; 
}

/*-------------------------------------------------------------*/
bool AzSmat::isZero(int col_no) const
{
  if (col_no < 0 || col_no >= col_num || 
      column[col_no] == NULL) {
    return true; 
  }
  return column[col_no]->isZero();
}

/*-------------------------------------------------------------*/
double AzSmat::max(int *out_row, int *out_col, 
                   bool ignoreZero) const
{
  int max_row = -1, max_col = -1; 
  double max_val = 0; 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    double local_max = 0; 
    int local_rx = -1; 
    if (column[cx] == NULL) {
      if (!ignoreZero) {
        local_max = 0; 
        local_rx = 0; 
      }
    }
    else {
      local_max = column[cx]->max(&local_rx, ignoreZero); 
    }
    if (local_rx >= 0) {
      if (max_col < 0 || local_max > max_val) {
        max_col = cx; 
        max_row = local_rx; 
        max_val = local_max; 
      }
    }
  }
  if (out_row != NULL) {
    *out_row = max_row; 
  }
  if (out_col != NULL) {
    *out_col = max_col; 
  }
  return max_val; 
}

/*-------------------------------------------------------------*/
double AzSmat::min(int *out_row, int *out_col, 
                   bool ignoreZero) const
{
  int min_row = -1, min_col = -1; 
  double min_val = 0; 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    double local_min = 0; 
    int local_rx = -1; 
    if (column[cx] == NULL) {
      if (!ignoreZero) {
        local_min = 0; 
        local_rx = 0; 
      }
    }
    else {
      local_min = column[cx]->min(&local_rx, ignoreZero); 
    }
    if (local_rx >= 0) {
      if (min_col < 0 || local_min < min_val) {
        min_col = cx; 
        min_row = local_rx; 
        min_val = local_min; 
      }
    }
  }
  if (out_row != NULL) {
    *out_row = min_row; 
  }
  if (out_col != NULL) {
    *out_col = min_col; 
  }
  return min_val; 
}

/*-------------------------------------------------------------*/
void AzSmat::set(int row_no, int col_no, double val) 
{
  const char *eyec = "AzSmat::set (row, col, val)"; 
  if (col_no < 0 || col_no >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }
  if (column[col_no] == NULL) {
    column[col_no] = new AzSvect(row_num); 
  }
  column[col_no]->set(row_no, val); 
}

/*-------------------------------------------------------------*/
void AzSmat::add(int row_no, int col_no, double val) 
{
  const char *eyec = "AzSmat::add"; 
  if (col_no < 0 || col_no >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }
  if (val == 0) return; 
  if (column[col_no] == NULL) {
    column[col_no] = new AzSvect(row_num); 
  }
  column[col_no]->add(row_no, val); 
}

/*-------------------------------------------------------------*/
void AzSmat::multiply(int row_no, int col_no, double val) 
{
  const char *eyec = "AzSmat::multiply (row, col, val)"; 
  if (col_no < 0 || col_no >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }
  if (column[col_no] == NULL) {
    return; 
  }
  column[col_no]->multiply(row_no, val); 
}

/*-------------------------------------------------------------*/
void AzSmat::multiply(double val) 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->multiply(val); 
    }
  }
}

/*-------------------------------------------------------------*/
double AzSmat::get(int row_no, int col_no) const
{
  const char *eyec = "AzSmat::get"; 
  if (col_no < 0 || col_no >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }
  if (column[col_no] == NULL) {
    return 0; 
  }
  return column[col_no]->get(row_no); 
}

/*-------------------------------------------------------------*/
void AzSmat::dump(const AzOut &out, const char *header, 
                  const AzStrArray *sp_row, 
                  const AzStrArray *sp_col, 
                  int cut_num) const
{
  if (out.isNull()) return; 

  AzPrint o(out); 

  const char *my_header = ""; 
  if (header != NULL) my_header = header; 
  o.printBegin(my_header, ",", "="); 
  /* (row,col)\n */
  o.pair_inBrackets(row_num, col_num, ","); 
  o.printEnd(); 

  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] == NULL) {
      continue; 
    }

    /* column=cx (col_header) */
    o.printBegin("", " ", "="); 
    o.print("column", cx); 
    if (sp_col != NULL) {
      o.inParen(sp_col->c_str(cx)); 
    }
    o.printEnd(); 
    column[cx]->dump(out, "", sp_row, cut_num); 
  }
  o.flush(); 
}

/*-------------------------------------------------------------*/
void AzSmat::normalize() 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {  
      column[cx]->normalize(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::normalize1() 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->normalize1(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::clear() 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      delete column[cx]; 
      column[cx] = NULL; 
    }
  }
}

/*-------------------------------------------------------------*/
int AzSmat::next(AzCursor &cursor, int col, double &out_val) const
{
  if (col < 0 || col >= col_num) {
    throw new AzException("AzSmat::next", "col# is out of range"); 
  }
  if (column[col] == NULL) {
    out_val = 0; 
    return AzNone; 
  }
  return column[col]->next(cursor, out_val); 
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
void AzSvect::reform(int inp_row_num, bool asDense) 
{
  _release(); 
  initialize(inp_row_num, asDense); 
}

/*-------------------------------------------------------------*/
void AzSvect::initialize(int inp_row_num, bool asDense) 
{
  const char *eyec = "AzSvect::initialize"; 
  if (inp_row_num < 0) {
    throw new AzException(eyec, "#row must be non-negative"); 
  }
  if (elm != NULL || elm_num > 0) {
    throw new AzException(eyec, "occupied"); 
  }
  row_num = inp_row_num; 
  if (asDense) {
    a.alloc(&elm, row_num, eyec, "elm"); 
    elm_num = row_num; 
    int ex; 
    for (ex = 0; ex < elm_num; ++ex) {
      elm[ex].no = ex; 
      elm[ex].val = 0; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::set(const AzSvect *vect1, double coefficient) 
{
  if (vect1 == NULL) {
    throw new AzException("AzSvect::set(Svect)", "null pointer"); 
  }
  if (row_num != vect1->row_num) {
    reform(vect1->row_num); 
  }
  clear_prepare(vect1->nonZeroRowNum()); 

  AzCursor cursor; 
  for ( ; ; ) {
    double val; 
    int rx = vect1->next(cursor, val); 
    if (rx < 0) break; 
    if (coefficient != 1) val *= coefficient; 
    set_inOrder(rx, val); 
  }
}

/*-------------------------------------------------------------*/
void AzSvect::set(const AzReadOnlyVector *inp, double coefficient) 
{
  if (inp == NULL) {
    throw new AzException("AzSvect::set(readonly)", "null pointer"); 
  }
  if (row_num != inp->rowNum()) {
    reform(inp->rowNum()); 
  }
  clear_prepare(inp->nonZeroRowNum()); 

  /* inp->rewind(); */
  AzCursor cursor; 
  for ( ; ; ) {
    double val; 
    int row = inp->next(cursor, val); 
    if (row < 0) break; 
    set_inOrder(row, val); 
  }
  if (coefficient != 1) multiply(coefficient); 
}

/*-------------------------------------------------------------*/
void AzSvect::_read(AzFile *file) 
{
  const char *eyec = "AzSvect::_read(file)"; 
  if (elm != NULL) {
    throw new AzException(eyec, "occupied"); 
  }
  row_num = file->readInt(); 
  elm_num = file->readInt(); 

  a.alloc(&elm, elm_num, eyec, "elm"); 

  file->seekReadBytes(-1, sizeof(elm[0])*elm_num, elm); 
  _swap(); 
}

/*-------------------------------------------------------------*/
void AzSvect::_swap()
{
  if (!isSwapNeeded) {
    return; 
  }

  AZI_VECT_ELM dummy; 
  if (sizeof(dummy.val) != sizeof(double)) {
    throw new AzException("AzSvect::_swap", "value is not double?!"); 
  }

  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    AZI_VECT_ELM *ep = &elm[ex];  
    AzFile::swap_int4(&ep->no); 
    AzFile::swap_double(&ep->val); 
  }
}

/*-------------------------------------------------------------*/
void AzSvect::write(AzFile *file) 
{
  file->writeInt(row_num); 
  file->writeInt(elm_num); 

  _swap(); 
  file->writeBytes(elm, sizeof(elm[0])*elm_num); 
  _swap(); 
}

/*-------------------------------------------------------------*/
void AzSvect::filter(const AzIntArr *ia_sorted, /* filter: must be sorted */
                     /*---  output  ---*/
                     AzIFarr *ifa_nonzero, 
                     AzIntArr *ia_zero)
const
{
  int idx_num; 
  const int *idx = ia_sorted->point(&idx_num); 
  if (idx_num <= 0) return; 

  int ex = 0; 
  int xx; 
  for (xx = 0; xx < idx_num; ++xx) {
    if (xx > 0 && idx[xx] < idx[xx-1]) {
      throw new AzException("AzSvect::filter", "filter must be sorted"); 
    }
    for ( ; ex < elm_num; ++ex) {
      if (elm[ex].no == idx[xx]) {
        if (elm[ex].val != 0) {
          if (ifa_nonzero != NULL) {
            ifa_nonzero->put(elm[ex].no, elm[ex].val); 
          }
        }
        else {
          if (ia_zero != NULL) {
            ia_zero->put(elm[ex].no); 
          }
        }
        break; 
      }
      else if (elm[ex].no > idx[xx]) {
        if (ia_zero != NULL) {
          ia_zero->put(idx[xx]); 
        }
        break; 
      }
    }
    if (ex >= elm_num) {
      if (ia_zero == NULL) break; 
      ia_zero->put(idx[xx]); 
    }
  }    
}

/* !!!!! Assuming there is no duplicated data index */
/*-------------------------------------------------------------*/
void AzSvect::nonZero(AzIFarr *ifa, 
                      const AzIntArr *ia_sorted) /* filter: must be sorted */
const
{
  int idx_num; 
  const int *idx = ia_sorted->point(&idx_num); 
  if (idx_num <= 0) return; 
  int xx = 0; 

  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      for ( ; xx < idx_num; ++xx) {
        if (xx > 0 && idx[xx] <= idx[xx-1]) {
          throw new AzException("AzSvect::nonZero", "filter must be sorted"); 
        }
        if (idx[xx] == elm[ex].no) {
          ifa->put(elm[ex].no, elm[ex].val); 
          ++xx; 
          break; 
        }
        else if (idx[xx] > elm[ex].no) {
          break; 
        }
      }
      if (xx >= idx_num) {
        break; 
      }
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::nonZero(AzIFarr *ifa) const
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      ifa->put(elm[ex].no, elm[ex].val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::all(AzIFarr *ifa) const
{
  ifa->prepare(row_num); 
  int last_no = -1; 
  int ex, rx; 
  for (ex = 0; ex < elm_num; ++ex) {
    for (rx = last_no+1; rx < elm[ex].no; ++rx) ifa->put(rx, 0); 
    ifa->put(elm[ex].no, elm[ex].val); 
    last_no = elm[ex].no; 
  }
  for (rx = last_no+1; rx < row_num; ++rx) ifa->put(rx, 0);   
  if (ifa->size() != row_num) {
    throw new AzException("AzSvect::all", "something is wrong"); 
  }
}

/*-------------------------------------------------------------*/
void AzSvect::zeroRowNo(AzIntArr *ia) const
{
  ia->reset(); 
  int last_no = -1; 
  int ex, rx; 
  for (ex = 0; ex < elm_num; ++ex) {
    for (rx = last_no+1; rx < elm[ex].no; ++rx) ia->put(rx); 
    if (elm[ex].val == 0) {
      ia->put(elm[ex].no); 
    }
    last_no = elm[ex].no; 
  }
  for (rx = last_no+1; rx < row_num; ++rx) ia->put(rx);   
}

/* returns the first one */
/*-------------------------------------------------------------*/
int AzSvect::nonZeroRowNo() const
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      return elm[ex].no; 
    }
  }
  return -1; 
}

/*-------------------------------------------------------------*/
void AzSvect::nonZeroRowNo(AzIntArr *intq) const
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      intq->put(elm[ex].no); 
    }
  }
}

/*-------------------------------------------------------------*/
int AzSmat::nonZeroRowNo(AzIntArr *ia_nzrows) const 
{
  ia_nzrows->reset(); 
  if (column == NULL) return 0; 
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      AzIntArr ia; column[col]->nonZeroRowNo(&ia); 
      ia_nzrows->concat(&ia); 
    }
  }
  int nz = ia_nzrows->size(); 
  ia_nzrows->unique(); 
  return nz; 
}

/*-------------------------------------------------------------*/
int AzSvect::nonZeroRowNum() const
{
  int count = 0; 
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      ++count;  
    }
  }
  return count; 
}

/*-------------------------------------------------------------*/
bool AzSvect::isZero() const
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      return false; 
    }
  }
  return true; 
}

/*-------------------------------------------------------------*/
void AzSmat::log_of_plusone()
{
  if (column == NULL) return; 
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      column[col]->log_of_plusone(); 
    }
  }
}
/*-------------------------------------------------------------*/
void AzSvect::log_of_plusone()
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val < 0) {
      throw new AzException("AzSvect::log_of_plusone", "components must be non-negative"); 
    }
    else if (elm[ex].val > 0) {
      elm[ex].val = log(elm[ex].val+1); 
    }
  }
}

/*-------------------------------------------------------------*/
double AzSvect::max(int *out_row_no, 
                    bool ignoreZero) const
{
  double max_val = -1; 
  int max_row = -1; 
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (ignoreZero && elm[ex].val == 0) {
      continue; 
    }
    if (max_row < 0 || elm[ex].val > max_val) {
      max_val = elm[ex].val; 
      max_row = elm[ex].no;  
    }
  }
  if (!ignoreZero && max_val < 0 && elm_num < row_num) {
    max_val = 0; 
    for (ex = 0; ex < elm_num; ++ex) {
      if (elm[ex].no != ex) break;
    }
    if (ex == 0) {
      max_row = 0; 
    }
    else {
      max_row = elm[ex - 1].no + 1; 
    }
  }
  if (out_row_no != NULL) {
    *out_row_no = max_row; 
  }

  if (max_row < 0 && ignoreZero) {
    max_val = 0; 
  }

  return max_val; 
}

/*-------------------------------------------------------------*/
double AzSvect::maxAbs(int *out_row_no, 
                       double *out_real_val) const
{
  double real_val = -1, max_val = -1; 
  int max_row = -1; 
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    double abs_val = fabs(elm[ex].val); 
    if (max_row < 0 || abs_val > max_val) {
      max_val = abs_val; 
      max_row = elm[ex].no;  
      real_val = elm[ex].val; 
    }
  }
  if (max_row < 0) {
    max_val = 0; 
    max_row = 0; 
    real_val = 0; 
  }
  if (out_row_no != NULL) {
    *out_row_no = max_row; 
  }
  if (out_real_val != NULL) {
    *out_real_val = real_val; 
  }

  return max_val; 
}

/*-------------------------------------------------------------*/
double AzSvect::min(int *out_row_no, 
                    bool ignoreZero) const
{
  double min_val = 1.0; 
  int min_row = -1; 

  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (ignoreZero && elm[ex].val == 0) {
      continue; 
    }

    if (min_row < 0 || elm[ex].val < min_val) {
      min_val = elm[ex].val; 
      min_row = elm[ex].no;  
    }
  }
  if (!ignoreZero && min_val > 0 && elm_num < row_num) {
    min_val = 0; 
    for (ex = 0; ex < elm_num; ++ex) {
      if (elm[ex].no != ex) {
        break;
      }
    }
    if (ex == 0) {
      min_row = 0; 
    }
    else {
      min_row = elm[ex - 1].no + 1; 
    }
  }
  if (out_row_no != NULL) {
    *out_row_no = min_row; 
  }

  if (ignoreZero && min_row < 0) {
    min_val = 0; 
  }

  return min_val; 
}

/*-------------------------------------------------------------*/
double AzSvect::minPositive(int *out_row_no) const
{
  double min_val = 0; 
  int min_row = -1; 

  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val > 0) {
      if (min_row < 0 || elm[ex].val < min_val) {
        min_val = elm[ex].val; 
        min_row = elm[ex].no;  
      }
    }
  }

  if (out_row_no != NULL) {
    *out_row_no = min_row; 
  }
  return min_val; 
}

/*-------------------------------------------------------------*/
void AzSvect::set(int row_no, double val) 
{
  const char *eyec = "AzSvect::set"; 
  if (row_no < 0 || row_no >= row_num) {
    throw new AzException(eyec, "row# is out of range"); 
  }
  _checkVal(val); 

  int where = to_insert(row_no); 
  elm[where].val = (AZ_MTX_FLOAT)val; 
}


/*-------------------------------------------------------------*/
void AzSmat::set(double val)
{
  if (val == 0) {
    zeroOut(); 
    return; 
  }
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] == NULL) {
      column[col] = new AzSvect(row_num); 
    } 
    column[col]->set(val); 
  }
}

/*-------------------------------------------------------------*/
void AzSvect::set(double val) 
{
  _checkVal(val); 

  int rx; 
  if (elm_num == row_num) {
    for (rx = 0; rx < row_num; ++rx) {
      elm[rx].no = rx; 
      elm[rx].val = (AZ_MTX_FLOAT)val; 
    }
  }
  else {
    const char *eyec = "AzSvect::set (all)"; 
    a.free(&elm); 
    a.alloc(&elm, row_num, eyec, "elm"); 
    elm_num = row_num; 
    int ex; 
    for (ex = 0; ex < elm_num; ++ex) {
      elm[ex].no = ex; 
      elm[ex].val = (AZ_MTX_FLOAT)val; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::change_rowno(int new_row_num, const AzIntArr *ia_old2new, 
                           bool do_zero_negaindex)
{
  const char *eyec = "AzSvect::change_rowno"; 
  if (ia_old2new->size() != row_num) {
    throw new AzException(eyec, "insufficient index"); 
  }
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    int new_row = ia_old2new->get(elm[ex].no); 
    if (new_row == elm[ex].no) continue; 
    if (new_row < 0) {
      if (do_zero_negaindex) elm[ex].val = 0; 
      else                   throw new AzException(eyec, "negative index"); 
    }
    else if (new_row >= new_row_num) {
      throw new AzException(eyec, "new row# is out of range"); 
    }
    elm[ex].no = new_row; 
  }
  row_num = new_row_num; 
}

/*-------------------------------------------------------------*/
void AzSmat::change_rowno(int new_row_num, const AzIntArr *ia_old2new, 
                           bool do_zero_negaindex)
{
  if (column == NULL) {
    row_num = new_row_num; 
    return; 
  }
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      column[col]->change_rowno(new_row_num, ia_old2new, do_zero_negaindex);
    }
  }
  row_num = new_row_num; 
}                           

/*-------------------------------------------------------------*/
void AzSvect::add(int row_no, double val) 
{
  const char *eyec = "AzSvect::add"; 
  if (row_no < 0 || row_no >= row_num) {
    throw new AzException(eyec, "row# is out of range"); 
  }
  if (val == 0) {
    return; 
  }
  int where = to_insert(row_no); 
  double new_val = (double)elm[where].val + val; 
  _checkVal(new_val); 
  elm[where].val = (AZ_MTX_FLOAT)new_val; 
}

/*-------------------------------------------------------------*/
void AzSvect::plus_one_log()
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val == 0); 
    else if (elm[ex].val < 0) {
      throw new AzException("AzSvect::plus_one_log", "Negative input"); 
    }
    else elm[ex].val = log(elm[ex].val+1); 
  }
}
/*-------------------------------------------------------------*/
void AzSmat::plus_one_log()
{
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) column[col]->plus_one_log(); 
  }
}

/*-------------------------------------------------------------*/
double AzSvect::sum() const
{
  double sum = 0; 
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    sum += (double)elm[ex].val; 
  }
  return sum; 
}

/*-------------------------------------------------------------*/
double AzSvect::absSum() const
{
  double sum = 0; 
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    sum += fabs((double)elm[ex].val); 
  }
  return sum; 
}

/*-------------------------------------------------------------*/
void AzSvect::multiply(int row_no, double val) 
{
  const char *eyec = "AzSvect::multiply"; 
  if (row_no < 0 || row_no >= row_num) {
    throw new AzException(eyec, "row# is out of range"); 
  }
  if (val == 1) {
    return; 
  }
  
  int where = find(row_no); 
  if (where < 0) {
    return; 
  }

  for ( ; where < elm_num; ++where) {
    if (elm[where].no == row_no) {
      double new_val = (double)elm[where].val * val; 
      _checkVal(new_val); 
      elm[where].val = (AZ_MTX_FLOAT)new_val; 
    }
    else if (elm[where].no > row_no) {
      return; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::multiply(double val) 
{
  if (val == 1) {
    return; 
  }

  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    double new_val = (double)elm[ex].val * val; 
    _checkVal(new_val); 
    elm[ex].val = (AZ_MTX_FLOAT)((double)elm[ex].val * val); 
  }
}

/*-------------------------------------------------------------*/
void AzSvect::scale(const double *vect1)
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    double new_val = (double)elm[ex].val * vect1[elm[ex].no]; 
    _checkVal(new_val); 
    elm[ex].val = (AZ_MTX_FLOAT)(new_val); 
  }
}

/*-------------------------------------------------------------*/
int AzSvect::to_insert(int row_no) 
{
  const char *eyec = "AzSvect::to_insert"; 

  int where = 0; 
  if (elm_num != 0) {
    where = find_forRoom(row_no); 
    for ( ; where < elm_num; ++where) {
      if (elm[where].no == row_no) {
        return where;  
      } 
      else if (elm[where].no > row_no) {
        break;
      }
    }
  }

  int elm_num_max = a.size(); 
  if (elm_num >= elm_num_max) {
    elm_num_max += inc(); 
    elm_num_max = MIN(elm_num_max, row_num);

    a.realloc(&elm, elm_num_max, eyec, "elm"); 
  }

  int ex; 
  for (ex = elm_num - 1; ex >= where; --ex) {
    elm[ex + 1] = elm[ex]; 
  }
  ++elm_num; 

  /*-----  initialize the new element  -----*/
  elm[where].no = row_no; 
  elm[where].val = 0; 

  return where; 
}

/*-------------------------------------------------------------*/
double AzSvect::get(int row_no) const
{
  const char *eyec = "AzSvect::get"; 
  if (row_no < 0 || row_no >= row_num) {
    throw new AzException(eyec, "row# is out of range"); 
  }
  int where = find(row_no); 
  if (where < 0) {
    return 0; 
  }

  for ( ; where < elm_num; ++where) {
    if (elm[where].no == row_no) {
      return elm[where].val; 
    }
    else if (elm[where].no > row_no) {
      return 0; 
    }
  }
  return 0; 
}

/*-------------------------------------------------------------*/
int AzSvect::find(int row_no, 
                  int from_this) const
{
  bool isFound; 
  int where = find_forRoom(row_no, from_this, &isFound); 
  if (!isFound) {
    return -1; 
  }
  return where; 
}

/*-------------------------------------------------------------*/
int AzSvect::find_forRoom(int row_no, 
                          int from_this, 
                          bool *out_isFound) const
{
  if (out_isFound != NULL) {
    *out_isFound = false; 
  }

  if (elm_num == row_num) {  
    if (out_isFound != NULL) {
      *out_isFound = true; 
    }
    return row_no; 
  }

  if (elm_num == 0) {
    return 0; /* failed */
  }

  /*---  Decide where to search  ---*/
  int lx, hx; 
  lx = 0; 
  if (from_this >= 0) {
    lx = from_this; 
  }
  hx = elm_num - 1; 

  /*---  Just scan if not many  ---*/
  if (hx - lx <= AzVectSmall) {
    for ( ; lx <= hx; ++lx) {
      if (elm[lx].no == row_no) {
        if (out_isFound != NULL) {
          *out_isFound = true; 
        }
        return lx; 
      }
      else if (elm[lx].no > row_no) {
        return lx; /* failed */
      }
    }
  }

  /*---  Do binary search if many  ---*/
  for ( ; ; ) {
    if (lx > hx) 
      return lx; 
    int mx = (lx + hx) / 2; 
    int cmp = row_no - elm[mx].no; 
    if (cmp < 0) hx = mx - 1; 
    else if (cmp > 0) lx = mx + 1; 
    else {
      if (out_isFound != NULL) {
        *out_isFound = true; 
      }
      return mx; 
    }
  }
}

/*-------------------------------------------------------------*/
double AzSvect::selfInnerProduct() const
{
  double n2 = 0; 
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      n2 += (double)elm[ex].val * (double)elm[ex].val; 
    }
  }
  return n2; 
}

/*-------------------------------------------------------------*/
double AzSvect::normalize()
{
  int ex; 
  double norm2 = 0; 
  for (ex = 0; ex < elm_num; ++ex) {
    norm2 += (elm[ex].val * elm[ex].val); 
  }

  if (norm2 != 0) {
    norm2 = sqrt(norm2); 

    for (ex = 0; ex < elm_num; ++ex) {
      double new_val = (double)elm[ex].val / norm2; 
      _checkVal(new_val); 
      elm[ex].val = (AZ_MTX_FLOAT)new_val; 
    }
  }
  return norm2; 
}

/*-------------------------------------------------------------*/
double AzSvect::normalize1()
{
  int ex; 
  double sum = 0; 
  for (ex = 0; ex < elm_num; ++ex) {
    sum += elm[ex].val; 
  }

  if (sum != 0) {
    for (ex = 0; ex < elm_num; ++ex) {
      double new_val = (double)elm[ex].val / sum; 
      _checkVal(new_val); 
      elm[ex].val = (AZ_MTX_FLOAT)((double)elm[ex].val / sum); 
    }
  }
  return sum; 
}

/*-------------------------------------------------------------*/
void AzSvect::clear()
{
  a.free(&elm); 
  elm_num = 0; 
}

/*-------------------------------------------------------------*/
void AzSvect::zeroOut()
{
  elm_num = 0; 
}

/*-------------------------------------------------------------*/
int AzSvect::next(AzCursor &cursor, double &out_val) const 
{
  int nonzero_ex = MAX(cursor.get(), 0); 
  for ( ; nonzero_ex < elm_num; ++nonzero_ex) {
    if (elm[nonzero_ex].val != 0) {
      break;
    }
  }

  cursor.set(nonzero_ex + 1);  /* prepare for next "next" */

  if (nonzero_ex < elm_num) {
    out_val = elm[nonzero_ex].val; 
    return elm[nonzero_ex].no; 
  }

  /*---  end of the elements  ---*/
  out_val = 0; 
  return AzNone; 
} 

/*-------------------------------------------------------------*/
void AzSvect::dump(const AzOut &out, const char *header, 
                     const AzStrArray *sp_row, 
                     int cut_num) const
{
  if (out.isNull()) return; 

  const char *my_header = ""; 
  if (header != NULL) my_header = header; 

  AzPrint o(out); 
  int indent = 3; 
  o.printBegin(my_header, ",", "=", indent); 
  o.print("elm_num", elm_num); 
  o.printEnd(); 

  if (cut_num > 0) {
    _dump(out, sp_row, cut_num); 
    return; 
  }

  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val == 0) {
      continue;  
    }
    o.printBegin("", " ", "=", indent); 
    o.inBrackets(elm[ex].no, 4); 
    o.print(elm[ex].val, 5, true); 
    if (sp_row != NULL) {
      const char *row_header = sp_row->c_str(elm[ex].no); 
      o.inParen(row_header); 
    }
    o.printEnd(); 
  }
  o.flush(); 
}

/*-------------------------------------------------------------*/
void AzSvect::_dump(const AzOut &out, 
                     const AzStrArray *sp_row, 
                     int cut_num) const
{
  AzIFarr ifa_nz; 
  nonZero(&ifa_nz); 

  ifa_nz.sort_Float(false); 
  ifa_nz.cut(cut_num); 
  int num = ifa_nz.size(); 

  AzPrint o(out); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int row_no; 
    double val = ifa_nz.get(ix, &row_no); 

    int indent = 3; 
    o.printBegin("", " ", "=", indent); 
    o.inBrackets(row_no, 4); 
    o.print(val, 5, true); 
    if (sp_row != NULL) {
      const char *row_header = sp_row->c_str(row_no); 
      o.inParen(row_header); 
    }
    o.printEnd();
  }
  o.newLine(); 
  o.flush();  
}

/*-------------------------------------------------------------*/
void AzSvect::zerooutNegative()
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) if (elm[ex].val < 0) elm[ex].val = 0; 
}

/*-------------------------------------------------------------*/
void AzSvect::cut(double min_val)
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (fabs(elm[ex].val) < min_val) {
      elm[ex].val = 0; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::cut(double min_val)
{
  if (column == NULL)  return; 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) if (column[cx] != NULL) column[cx]->cut(min_val); 
}

/*-------------------------------------------------------------*/
void AzSvect::only_keep(int num)
{
  if (num <= 0) throw new AzException("AzSvect::only_keep", "Expected a positive number"); 
  int nz = nonZeroRowNum(); 
  if (nz <= num) return; 
 
  AzIFarr ifa; ifa.prepare(elm_num); 
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      ifa.put(ex, fabs(elm[ex].val)); 
    }
  }
  ifa.sort_Float(false); 
  ifa.cut(num); 
  AzIFarr ifa_val; ifa_val.prepare(ifa.size()); 
  int ix; 
  for (ix = 0; ix < ifa.size(); ++ix) {
    ifa.get(ix, &ex); 
    ifa_val.put(elm[ex].no, elm[ex].val); 
  }
  ifa_val.sort_Int(true); 
  load(&ifa_val); 
}

/*-------------------------------------------------------------*/
void AzSmat::only_keep(int num) 
{
  if (column == NULL)  return; 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) if (column[cx] != NULL) column[cx]->only_keep(num); 
}

/*-------------------------------------------------------------*/
void AzSmat::zerooutNegative()
{
  if (column == NULL) return;
  int cx; 
  for (cx = 0; cx < col_num; ++cx) if (column[cx] != NULL) column[cx]->zerooutNegative();
}

/*-------------------------------------------------------------*/
void AzSmat::zeroOut()
{
  if (column == NULL) {
    return; 
  }
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->zeroOut(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::load(const AzIntArr *ia_row, double val, 
                   bool do_ignore_negative)
{
  int num; 
  const int *row = ia_row->point(&num); 
  AzIFarr ifa_row_val; 
  ifa_row_val.prepare(num); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    if (!do_ignore_negative || row[ix] >= 0) ifa_row_val.put(row[ix], val); 
  }
  load(&ifa_row_val); 
}

/* must be called in the order of the rows */
/*-------------------------------------------------------------*/
void AzSvect::set_inOrder(int row_no, double val)
{
  const char *eyec = "AzSvect::set_inOrder"; 

  if (row_no < 0 || row_no >= row_num) {
    throw new AzException(eyec, "row# is out of range"); 
  }
  if (elm_num > 0 && 
      elm[elm_num-1].no >= row_no) {
    throw new AzException(eyec, "input is not in the order"); 
  }

  int where = elm_num; 

  int elm_num_max = a.size(); 
  if (elm_num >= elm_num_max) {
    elm_num_max += inc(); 
    elm_num_max = MIN(elm_num_max, row_num);
    a.realloc(&elm, elm_num_max, eyec, "elm"); 
  }

  ++elm_num; 

  elm[where].no = row_no; 
  elm[where].val = (AZ_MTX_FLOAT)val; 
}

/*-------------------------------------------------------------*/
void AzSvect::load(const AzIFarr *ifa_row_val) 
{  
  const char *eyec = "AzSvect::load"; 
  int num = ifa_row_val->size(); 
  
  clear_prepare(num); 

  int prev_row = -1; 

  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int row; 
    double val = ifa_row_val->get(ix, &row);     
    if (row < 0 || row >= row_num || /* out of range */
        row <= prev_row) { /* out of order */
      cout << "row=" << row << " row_num=" << row_num << " prev_row=" << prev_row << endl; 
      throw new AzException("AzSvect::load", "Invalid input"); 
    }

    elm[elm_num].no = row; 
    _checkVal(val); 
    elm[elm_num].val = (AZ_MTX_FLOAT)val; 
    ++elm_num; 
    prev_row = row;  /* corrected on 2/18/2014 */    
  }
}

/*-------------------------------------------------------------*/
bool AzSmat::isSame(const AzSmat *inp) const
{
  if (col_num != inp->col_num) return false; 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    bool isZero1 = isZero(cx); 
    bool isZero2 = inp->isZero(cx); 
    if (isZero1 != isZero2) return false; 
    if (isZero1) continue; 
    if (!column[cx]->isSame(inp->column[cx])) return false; 
  }
  return true; 
}

/*-------------------------------------------------------------*/
bool AzSvect::isSame(const AzSvect *inp) const
{
  if (row_num != inp->row_num) return false; 

  int ex = 0, ex1 = 0; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val == 0) continue;  /* ignore zero */
    for ( ; ex1 < inp->elm_num; ++ex1) {
      if (inp->elm[ex1].val == 0) continue; /* ignore zero */
      if (inp->elm[ex1].no != elm[ex].no || 
          inp->elm[ex1].val != elm[ex].val) {
        return false; /* different! */
      }
      break; /* matched */
    }
    if (ex1 >= inp->elm_num) {
      return false; 
    }
    ++ex1; /* matched */
  }

  for ( ; ex1 < inp->elm_num; ++ex1) {
    if (inp->elm[ex1].val != 0) {
      return false;  /* extra non-zero components */
    }
  }
  return true; 
}

/*-------------------------------------------------------------*/
double AzSmat::nonZeroNum(double *ratio) const
{
  if (ratio != NULL) {
    *ratio = 0; 
  }
  double out = 0; 
  if (column == NULL) return out; 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      out += (double)column[cx]->nonZeroRowNum(); 
    }
  }
  if (ratio != NULL && out != 0) {
    *ratio = (double)out/(double)((double)row_num*(double)col_num); 
  }
  return out; 
}

/*-------------------------------------------------------------*/
int AzSmat::nonZeroColNum() const
{
  if (column == NULL) return 0; 
  int num = 0; 
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL && 
        !column[col]->isZero()) {
      ++num; 
    }
  }
  return num; 
}

/*-------------------------------------------------------------*/
void AzSmat ::cap(double cap_val) {
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      column[col]->cap(cap_val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::cap(double cap_val) {
  if (cap_val <= 0) {
    throw new AzException("AzSvect::cap", "cap value must be non-negative"); 
  }
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val > cap_val) {
      elm[ex].val = cap_val;      
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::rbind(const AzSmat *m1)
{
  const char *eyec = "AzSmat::rbind"; 
  if (colNum() == 0) {
    set(m1); 
    return; 
  }
  
  if (m1->colNum() != colNum()) {
    throw new AzException(eyec, "#col mismatch"); 
  }
  int offs = rowNum(); 
  resize(offs+m1->rowNum(), colNum()); 
  int cx; 
  for (cx = 0; cx < colNum(); ++cx) {
    const AzSvect *v0 = col(cx); 
    const AzSvect *v1 = m1->col(cx); 
    AzIFarr ifa; 
    ifa.prepare(v0->nonZeroRowNum()+v1->nonZeroRowNum());
    v0->nonZero(&ifa);  
    AzCursor cur;     
    for ( ; ; ) {
      double val;
      int row = v1->next(cur, val); 
      if (row < 0) break; 
      ifa.put(row+offs, val); 
    }
    col_u(cx)->load(&ifa); 
  }
}

/*-------------------------------------------------------------*/
void AzSmat::cbind(const AzSmat *m1)
{
  const char *eyec = "AzSmat::cbind"; 
  if (colNum() <= 0 || rowNum() <= 0) {
    set(m1); 
    return; 
  }
  if (m1->rowNum() != rowNum()) {
    throw new AzException(eyec, "#row mismatch"); 
  }
  int offs = colNum(); 
  resize(m1->rowNum(), offs+m1->colNum()); 
  int cx; 
  for (cx = 0; cx < m1->colNum(); ++cx) {
    col_u(offs+cx)->set(m1->col(cx)); 
  }
}

#if 0 
/*------------------------------------------------------------*/ 
void AzSvect::polarize() 
{
  if (min() >= 0) {
    resize(row_num*2); 
    return; 
  }

  AzIFarr ifa; 
  ifa.prepare(elm_num*2); 
  int ex;
  for (ex = 0; ex < elm_num; ++ex) { 
    const AZI_VECT_ELM *ep = &elm[ex]; 
    if (ep->val > 0) {
      ifa.put(ep->no, ep->val); 
    }
    else if (ep->val < 0) {
      ifa.put(ep->no + row_num, -ep->val); 
    }
  }
  resize(row_num*2); 
  load(&ifa); 
}
#endif 

/*------------------------------------------------------------*/ 
void AzSvect::polarize() 
{
  if (min() >= 0) {
    resize(row_num*2); 
    return; 
  }

  int org_row_num = row_num; 
  int org_elm_num = elm_num; 
  resize(row_num*2); 
  int ex;
  for (ex = 0; ex < org_elm_num; ++ex) { 
    if (elm[ex].val < 0) {
      set_inOrder(elm[ex].no+org_row_num, -elm[ex].val); 
      elm[ex].val = 0; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::binarize() {
  if (column == NULL) return; 
  int col; 
  for (col = 0; col < col_num; ++col) if (column[col] != NULL) column[col]->binarize(); 
}

/*-------------------------------------------------------------*/
void AzSmat::binarize1() {
  if (column == NULL) return; 
  int col; 
  for (col = 0; col < col_num; ++col) if (column[col] != NULL) column[col]->binarize1(); 
}

/*-------------------------------------------------------------*/
void AzSvect::binarize()
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val > 0) {
      elm[ex].val = 1; 
    }
    else if (elm[ex].val < 0) {
      elm[ex].val = -1; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSvect::binarize1()
{
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) {
    if (elm[ex].val != 0) {
      elm[ex].val = 1; 
    }
  }
}
/*-------------------------------------------------------------*/
bool AzSvect::isOneOrZero() const {
  int ex; 
  for (ex = 0; ex < elm_num; ++ex) if (elm[ex].val != 0 && elm[ex].val != 1) return false; 
  return true; 
}
/*-------------------------------------------------------------*/
bool AzSmat::isOneOrZero() const {
  if (column == NULL) return true; 
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      bool ret = column[col]->isOneOrZero();
      if (!ret) return false; 
    }
  }
  return true; 
}
/*-------------------------------------------------------------*/
bool AzSmat::isOneOrZero(int col) const {
  if (column == NULL) return true; 
  if (column[col] == NULL) return true;  
  return column[col]->isOneOrZero(); 
}

/*------------------------------------------------*/
/*------------------------------------------------*/
void AzSmatVar::set(const AzSmatVar *mv0, 
                    const int *dxs0, /* mv0's data points */
                    int dxs0_num)   /* size of dxs0 */
{
  const char *eyec = "AzSmatVar::set(mv0,dxs,num)"; 
  int dnum = dxs0_num; 
  data_num = dnum; 

  /*---  generate column index  ---*/
  ia_dcolind.reset(dnum*2, 0); 
  
  int offs = 0; 
  AzIntArr ia_cols; 
  int *h_dcolind = ia_dcolind.point_u(); 
  int ix; 
  for (ix = 0; ix < dxs0_num; ++ix) {
    int dx0 = dxs0[ix]; 
    int col0 = mv0->col_begin(dx0); 
    int col1 = mv0->col_end(dx0); 

    h_dcolind[ix*2] = offs; 
    offs += (col1 - col0);  
    h_dcolind[ix*2+1] = offs; 
    int col; 
    for (col = col0; col < col1; ++col) {
      ia_cols.put(col); 
    }
  }

  m.set(mv0->data(), ia_cols.point(), ia_cols.size()); 
}  

/*------------------------------------------------*/
/* static */
void AzSmatVar::rbind_cbind(const AzSmatVar *mv0,
                          const AzSmatVar *mv1, 
                          AzSmatVar *m_out)
{
  const char *eyec = "AzSmatVar::rbind_cbind"; 
  int data_num = mv0->dataNum(); 
  if (data_num != mv1->dataNum()) {
    throw new AzException(eyec, "#data conflict"); 
  }
  AzSmat m_new(mv0->rowNum()+mv1->rowNum(), mv0->colNum()+mv1->colNum()); 
  AzIntArr ia_new_dcolind; 
  int col = 0; 
  int dx; 
  for (dx = 0; dx < data_num; ++dx) {
    ia_new_dcolind.put(col); 
    int col0_0 = mv0->col_begin(dx), col1_0 = mv0->col_end(dx); 
    int col0_1 = mv1->col_begin(dx), col1_1 = mv1->col_end(dx); 
      
    AzSmat m00; m00.set(mv0->data(), col0_0, col1_0); 
    AzSmat m10(mv1->rowNum(), col1_0-col0_0); 
    m00.rbind(&m10); 
      
    AzSmat m01(mv0->rowNum(), col1_1-col0_1); 
    AzSmat m11; m11.set(mv1->data(), col0_1, col1_1);
    m01.rbind(&m11); 
      
    m00.cbind(&m01); 
    int new_col0 = col, new_col1 = new_col0+m00.colNum(); 
    m_new.set(new_col0, new_col1, &m00); 
      
    col = new_col1; 
    ia_new_dcolind.put(col); 
  }
  m_out->reset(&m_new, &ia_new_dcolind); 
}  