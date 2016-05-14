/* * * * *
 *  AzDmat.cpp 
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

#include "AzDmat.hpp"
#include "AzPrint.hpp"

/*-------------------------------------------------------------*/
void AzDmat::_reform(int new_row_num, int new_col_num, 
                     bool do_zeroOut) 
{
  const char *eyec = "AzDmat::_reform";  
  checkLock(eyec); 
  if (row_num == new_row_num && col_num == new_col_num) {
    if (do_zeroOut) zeroOut(); 
    return; 
  }
 
  _release(); 
  if (new_col_num < 0 || new_row_num < 0) {
    throw new AzException(eyec, "# columns or row must be non-negative"); 
  }
  col_num = new_col_num; 
  row_num = new_row_num; 
  a.alloc(&column, col_num, eyec, "column"); 
  dummy_zero.reform(row_num);   
  
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    column[cx] = new AzDvect(this->row_num); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::_set(const AzDmat *m_inp)
{
  const char *eyec = "AzDmat::set(Dmat)";   
  checkLock(eyec); 
  if (row_num != m_inp->row_num || col_num != m_inp->col_num) {
    _release(); 
    if (m_inp->col_num < 0 || m_inp->row_num < 0) {
      throw new AzException(eyec, "# columns or row must be non-negative"); 
    }
    col_num = m_inp->col_num; 
    row_num = m_inp->row_num; 
    a.alloc(&column, col_num, eyec, "column"); 
    dummy_zero.reform(row_num);   
  }
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    delete column[cx]; 
    column[cx] = new AzDvect(m_inp->col(cx)); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::initialize(const AzReadOnlyMatrix *inp)
{
  const char *eyec = "AzDmat::initialize(inp)"; 
  checkLock(eyec); 

  if (inp == NULL) {
    throw new AzException(eyec, "null input"); 
  }
  col_num = inp->colNum(); 
  row_num = inp->rowNum(); 

  a.alloc(&column, col_num, eyec, "column"); 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    column[cx] = new AzDvect(inp->col(cx)); 
  }
  dummy_zero.reform(row_num); 
}

/*-------------------------------------------------------------*/
void AzDmat::resize(int new_row_num, int new_col_num) 
{
  if (new_col_num != col_num) {
    resize(new_col_num); 
  }
  if (new_row_num != row_num) {
    int col; 
    for (col = 0; col < col_num; ++col) {
      column[col]->resize(new_row_num); 
    }
    row_num = new_row_num; 
    dummy_zero.reform(row_num); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::resize(int new_col_num) 
{
  const char *eyec = "AzDmat::resize"; 
  checkLock(eyec); 
  if (new_col_num == col_num) {
    return; 
  } 
  if (new_col_num < 0) {
    throw new AzException(eyec, "new #columns must be non-negative"); 
  }

  int old_col_num = col_num; 
  a.realloc(&column, new_col_num, eyec, "column"); 
  int cx; 
  for (cx = old_col_num; cx < new_col_num; ++cx) {
    column[cx] = new AzDvect(row_num); 
  }
  col_num = new_col_num; 
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
void AzDmat::transpose(AzDmat *m_out, 
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
      throw new AzException("AzDmat::transpose", "column range error"); 
    }
  }

  _transpose(m_out, col_b, col_e); 
}

/*-------------------------------------------------------------*/
void AzDmat::_transpose(AzDmat *m_out, 
                         int col_begin, 
                         int col_end) const
{
  m_out->reform(col_end - col_begin, row_num); 

  int cx; 
  for (cx = col_begin; cx < col_end; ++cx) {
    if (column[cx] == NULL) {
      continue; 
    }

    int rx; 
    for (rx = 0; rx < row_num; ++rx) {
      double val = column[cx]->get(rx); 
      if (val != 0) {
        m_out->set(cx - col_begin, rx, val); 
      }
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::transpose_from(const AzSmat *m_inp)
{
  reform(m_inp->colNum(), m_inp->rowNum()); 
  int cx; 
  for (cx = 0; cx < m_inp->colNum(); ++cx) {
    const AzSvect *v_inp = m_inp->col(cx); 
    AzCursor cursor; 
    for ( ; ; ) {
      double val; 
      int rx = v_inp->next(cursor, val); 
      if (rx < 0) break; 
      set(cx, rx, val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::_read(AzFile *file) 
{
  const char *eyec = "AzDmat::_read"; 
  checkLock(eyec); 
  col_num = file->readInt(); 
  row_num = file->readInt(); 
  if (col_num > 0) {
    a.alloc(&column, col_num, eyec, "column"); 
    int cx; 
    for (cx = 0; cx < col_num; ++cx) {
      column[cx] = AzObjIOTools::read<AzDvect>(file); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::write(AzFile *file) 
{
  file->writeInt(col_num); 
  file->writeInt(row_num); 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    AzObjIOTools::write(column[cx], file); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::set(int row, int col, double val) 
{
  const char *eyec = "AzDmat::set (row, col, val)"; 
  if (col < 0 || col >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }

  if (column[col] == NULL) {
    column[col] = new AzDvect(row_num); 
  }
  column[col]->set(row, val); 
}

/*-------------------------------------------------------------*/
void AzDmat::add(int row, int col, double val) 
{
  const char *eyec = "AzDmat::add"; 
  if (col < 0 || col >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }

  if (val == 0) return; 

  if (column[col] == NULL) {
    column[col] = new AzDvect(row_num); 
  }
  column[col]->add(row, val); 
}

/*-------------------------------------------------------------*/
void AzDmat::add(const AzDmat *inp, double coeff)  
{
  const char *eyec = "AzDmat::add (matrix, coeff)"; 

  if (inp->row_num != this->row_num || 
      inp->col_num != this->col_num) {
    throw new AzException(eyec, "Shape doesn't match"); 
  }

  int cx; 
  for (cx = 0; cx < this->col_num; ++cx) {
    if (inp->column[cx] == NULL) {
      continue; 
    }

    if (column[cx] == NULL) {
      column[cx] = new AzDvect(this->row_num); 
    }
    column[cx]->add(inp->column[cx], coeff); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::add(const AzSmat *inp, double coeff)
{
  const char *eyec = "AzDmat::add (matrix, coeff)"; 

  if (inp->rowNum() != this->row_num || 
      inp->colNum() != this->col_num) {
    throw new AzException(eyec, "Shape doesn't match"); 
  }

  int cx; 
  for (cx = 0; cx < this->col_num; ++cx) {
    if (inp->isZero(cx)) {
      continue; 
    }

    if (column[cx] == NULL) {
      column[cx] = new AzDvect(this->row_num); 
    }
    column[cx]->add(inp->col(cx), coeff); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::multiply(int row, int col, double val) 
{
  const char *eyec = "AzDmat::multiply (row, col, val)"; 

  if (col < 0 || col >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }
  if (column[col] == NULL) {
    return; 
  }

  column[col]->multiply(row, val); 
}

/*-------------------------------------------------------------*/
void AzDmat::multiply(double val) 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->multiply(val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::add(double val) 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->add(val); 
    }
  }
}

/*-------------------------------------------------------------*/
double AzDmat::get(int row, int col) const
{
  const char *eyec = "AzDmat::get"; 

  if (col < 0 || col >= col_num) {
    throw new AzException(eyec, "col# is out of range"); 
  }
  if (column[col] == NULL) {
    return 0; 
  }

  return column[col]->get(row); 
}

#if 0 
/*-------------------------------------------------------------*/
void AzDmat::dump(const AzOut &out, const char *header, 
                     const AzStrArray *sp_row, 
                     const AzStrArray *sp_col, 
                     int cut_num) const
{
  if (out.isNull()) return; 

  AzPrint o(out); 

  const char *my_header = ""; 
  if (header != NULL) my_header = header; 
  o.writeln(my_header); 

  /* (row,col)=(r,c)\n */
  o.printBegin("", ""); 
  o.print("(row,col)="); 
  o.pair_inParen(row_num, col_num, ","); 
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
#endif 
 
/*-------------------------------------------------------------*/
void AzDmat::dump(const AzOut &out, const char *header, 
                  int max_col, 
                  const AzStrArray *sp_row, 
                  const AzStrArray *sp_col, 
                  int cut_num) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 

  const char *my_header = ""; 
  if (header != NULL) my_header = header; 
  o.writeln(my_header); 

  /* (row,col)=(r,c)\n */
  int c_num = col_num; 
  if (max_col > 0 && max_col < col_num) {
    c_num = max_col; 
  }
  o.printBegin("", ""); 
  o.print("(row,col)="); 
  o.pair_inParen(row_num, col_num, ","); 
  if (c_num < col_num) {
    AzBytArr s(" Showing first "); s.cn(c_num); s.c(" columns only"); 
    o.print(s); 
  }
  o.printEnd(); 

  int cx; 
  for (cx = 0; cx < c_num; ++cx) {
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
  o.writeln(""); /* blank line */
  o.flush(); 
}

/*-------------------------------------------------------------*/
void AzDmat::normalize() 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {  
      column[cx]->normalize(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::normalize1() 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->normalize1(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::binarize()
{
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      column[col]->binarize(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::binarize1()
{
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      column[col]->binarize1(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::cut(double min_val)
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->cut(min_val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::zeroOut()
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->zeroOut(); 
    }
  }
}

/*-------------------------------------------------------------*/
double AzDmat::max(int *out_row, int *out_col) const
{
  int max_row = -1, max_col = -1; 

  double max_val = 0; 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    double local_max; 
    int local_rx; 
    if (column[cx] == NULL) {
      local_max = 0; 
      local_rx = 0; 
    }
    else {
      local_max = column[cx]->max(&local_rx); 
    }
    if (max_col < 0 || local_max > max_val) {
      max_col = cx; 
      max_row = local_rx; 
      max_val = local_max; 
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
void AzDmat::scale(const AzDmat *m, bool isInverse) 
{
  if (m->rowNum() != row_num || m->colNum() != col_num) {
    throw new AzException("AzDmat::scale(dmat)", "shape mismatch"); 
  }
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] == NULL) continue; 
    if (m->column[cx] != NULL) column[cx]->scale(m->column[cx], isInverse); 
    else                       column[cx]->zeroOut(); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::scale(const AzSvect *vect1, bool isInverse) 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->scale(vect1, isInverse); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::scale(const AzDvect *vect1, bool isInverse) 
{
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    if (column[cx] != NULL) {
      column[cx]->scale(vect1, isInverse); 
    }
  }
}

/*-------------------------------------------------------------*/
bool AzDmat::isZero() const
{
  if (column == NULL) return true; 
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
bool AzDmat::isZero(int col) const
{
  if (col < 0 || col >= col_num) {
    throw new AzException("AzDmat::isZero(col)", "out of range"); 
  }
  if (column == NULL) return true; 
  if (column[col] != NULL && 
      !column[col]->isZero()) { 
    return false; 
  }
  return true; 
}

/*-------------------------------------------------------------*/
void AzDmat::convert(AzSmat *m_out)
{
  m_out->reform(row_num, col_num); 
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      AzIFarr ifq; 
      column[col]->nonZero(&ifq); 
      if (ifq.size() > 0) {
        m_out->col_u(col)->load(&ifq); 
      }
    }
  }
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
void AzDvect::set(const AzReadOnlyVector *v_inp) 
{
  const char *eyec = "AzDvect::set(AzReadOnlyVector)"; 
  if (v_inp == NULL) {
    throw new AzException(eyec, "null input"); 
  }
  reform(v_inp->rowNum()); 
  if (num > 0) {
    AzCursor cursor; 
    for ( ; ; ) {
      double val; 
      int ex = v_inp->next(cursor, val); 
      if (ex < 0) break; 
      elm[ex] = val;  
    } 
  }
}

/*-------------------------------------------------------------*/
/* WARNING: not tested */
void AzDvect::set(const AzSvect *v_inp) 
{
  const char *eyec = "AzDvect::set(Svect)"; 
  if (v_inp == NULL) {
    throw new AzException(eyec, "null input"); 
  }
  reform(v_inp->row_num); 
  if (num > 0) {
    int ix; 
    for (ix = 0; ix < v_inp->elm_num; ++ix) {
      elm[v_inp->elm[ix].no] = v_inp->elm[ix].val; 
    } 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::resize(int new_num) 
{
  const char *eyec = "AzDvect::resize"; 
  if (new_num < 0) {
    throw new AzException(eyec, "can't resize to negatve size"); 
  }

  int old_num = num;
  num = new_num;  
  if (new_num != old_num) {
    a.realloc(&elm, new_num, eyec, "elm"); 
  }
  int ex; 
  for (ex = old_num; ex < new_num; ++ex) {
    elm[ex] = 0; 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::_read(AzFile *file) 
{
  const char *eyec = "AzDvect::_read"; 
  if (elm != NULL || num != 0) {
    throw new AzException(eyec, "(elm=NULL,num=0) was expected"); 
  }
  num = file->readInt(); 
  a.alloc(&elm, num, eyec, "elm"); 
  file->seekReadBytes(-1, sizeof(elm[0])*num, elm); 
  _swap(); 
}

/*-------------------------------------------------------------*/
void AzDvect::_swap()
{
  if (!isSwapNeeded) return; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    AzFile::swap_double(&elm[ex]); 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::write(AzFile *file) 
{
  file->writeInt(num); 
  if (num > 0) {
    _swap(); 
    file->writeBytes(elm, sizeof(elm[0])*num); 
    _swap(); 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::binarize()
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] > 0) {
      elm[ex] = 1; 
    }
    else if (elm[ex] < 0) {
      elm[ex] = -1; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDvect::binarize1()
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != 0) {
      elm[ex] = 1; 
    }
  }
}

/*-------------------------------------------------------------*/
bool AzDvect::isZero() const
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != 0) {
      return false; 
    }
  }
  return true; 
}

/*-------------------------------------------------------------*/
void AzDvect::values(const int exs[], 
                     int ex_num, 
                     AzIFarr *ifa_ex_value) /* output */
{
  ifa_ex_value->prepare(ex_num); 
  int ix; 
  for (ix = 0; ix < ex_num; ++ix) {
    int ex = exs[ix]; 
    if (ex < 0 || ex >= num) {
      throw new AzException("AzDvect:;getValues", "Out of range"); 
    }
    ifa_ex_value->put(ex, elm[ex]); 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::nonZeroRowNo(AzIntArr *iq) const
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != 0) {
      iq->put(ex); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDvect::nonZero(AzIFarr *ifq) const
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != 0) {
      ifq->put(ex, elm[ex]); 
    }
  }
}

/*-------------------------------------------------------------*/
int AzDvect::nonZeroRowNum() const
{
  int count = 0; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != 0) {
      ++count;  
    }
  }
  return count; 
}

/*-------------------------------------------------------------*/
double AzDvect::max(const AzIntArr *ia_dx, int *out_no) const
{
  const int *dxs = NULL; 
  int dx_num = num; 
  if (ia_dx != NULL) {
    dxs = ia_dx->point(&dx_num); 
  }

  double max_val = -1; 
  int max_row = -1; 
  int ix; 
  for (ix = 0; ix < dx_num; ++ix) {
    int ex = ix; 
    if (dxs != NULL) {
      ex = dxs[ix]; 
    }
    if (max_row < 0 || elm[ex] > max_val) {
      max_val = elm[ex]; 
      max_row = ex; 
    }
  }
  if (out_no != NULL) {
    *out_no = max_row; 
  }
  return max_val; 
}

/*-------------------------------------------------------------*/
double AzDvect::max(int *out_no) const
{
  double max_val = -1; 
  int max_row = -1; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (max_row < 0 || elm[ex] > max_val) {
      max_val = elm[ex]; 
      max_row = ex; 
    }
  }
  if (out_no != NULL) {
    *out_no = max_row; 
  }
  return max_val; 
}

/*-------------------------------------------------------------*/
double AzDvect::maxAbs(int *out_row, 
                               double *out_real_val) const
{
  double real_val = -1, max_val = -1; 
  int max_row = -1; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    double abs_val = fabs(elm[ex]); 
    if (max_row < 0 || abs_val > max_val) {
      max_val = abs_val; 
      max_row = ex;  
      real_val = elm[ex]; 
    }
  }
  if (max_row < 0) {
    max_val = 0; 
    max_row = 0; 
    real_val = 0; 
  }
  if (out_row != NULL) {
    *out_row = max_row; 
  }
  if (out_real_val != NULL) {
    *out_real_val = real_val; 
  }

  return max_val; 
}

/*-------------------------------------------------------------*/
double AzDvect::min(const AzIntArr *ia_dx, int *out_no) const
{
  const int *dxs = NULL; 
  int dx_num = num; 
  if (ia_dx != NULL) {
    dxs = ia_dx->point(&dx_num); 
  }
  double min_val = -1; 
  int min_row = -1; 
  int ix; 
  for (ix = 0; ix < dx_num; ++ix) {
    int ex = ix; 
    if (dxs != NULL) {
      ex = dxs[ix]; 
    }
    if (min_row < 0 || elm[ex] < min_val) {
      min_val = elm[ex]; 
      min_row = ex; 
    }
  }
  if (out_no != NULL) {
    *out_no = min_row; 
  }
  return min_val; 
}

/*-------------------------------------------------------------*/
double AzDvect::min(int *out_no) const
{
  double min_val = -1; 
  int min_row = -1; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (min_row < 0 || elm[ex] < min_val) {
      min_val = elm[ex]; 
      min_row = ex; 
    }
  }
  if (out_no != NULL) {
    *out_no = min_row; 
  }
  return min_val; 
}

/*-------------------------------------------------------------*/
void AzDvect::set(double val) 
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    this->elm[ex] = val; 
  }
}

/*-------------------------------------------------------------*/
double AzDvect::sum() const
{
  double sum = 0; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    sum += elm[ex]; 
  }
  return sum; 
}

/*-------------------------------------------------------------*/
double AzDvect::sum(const int *row, int r_num) const
{
  double sum = 0; 
  int ix; 
  for (ix = 0; ix < r_num; ++ix) {
    int ex = row[ix]; 
    if (ex < 0 || ex >= num) {
      throw new AzException("AzDvect::sum", "out of range"); 
    }
    sum += elm[ex]; 
  }
  return sum; 
}

/*-------------------------------------------------------------*/
double AzDvect::absSum(const int *row, int r_num) const
{
  double sum = 0; 
  int ix; 
  for (ix = 0; ix < r_num; ++ix) {
    int ex = row[ix]; 
    if (ex < 0 || ex >= num) {
      throw new AzException("AzDvect::sum", "out of range"); 
    }
    sum += fabs(elm[ex]); 
  }
  return sum; 
}

/*-------------------------------------------------------------*/
double AzDvect::absSum() const
{
  double sum = 0; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    sum += fabs(elm[ex]); 
  }
  return sum; 
}

/*--------------------------------------------------------*/
void AzDvect::add(double val, 
                  const int *row, int r_num) 
{
  if (row == NULL) return; 
  int ix; 
  for (ix = 0; ix < r_num; ++ix) {
    int ex = row[ix]; 
    if (ex < 0 || ex >= num) {
      throw new AzException("AzDvet::add(val,ia)", "index is out of range"); 
    }
    elm[ex] += val; 
  }
}

/*--------------------------------------------------------*/
void AzDvect::add(double val) 
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    elm[ex] += val; 
  }
}

/*--------------------------------------------------------*/
void AzDvect::add_nochk(double val, 
                  const int *row, int r_num) 
{
  int ix; 
  for (ix = 0; ix < r_num; ++ix) {
    int ex = row[ix]; 
    elm[ex] += val; 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::max_abs(const AzDvect *v) 
{
  const char *eyec = "AzDvect::max_abs"; 
  if (num != v->rowNum()) {
    throw new AzException(eyec, "shape mismatch"); 
  }
  int ex;
  for (ex = 0; ex < num; ++ex) {
    double val = v->elm[ex]; 
    val = (val > 0) ? val : -val; 
    elm[ex] = MAX(elm[ex], val);     
  }
}

#if 0 
/*-------------------------------------------------------------*/
void AzDvect::max_abs(const AzReadOnlyVector *v) 
{
  const char *eyec = "AzDvect::max_abs"; 
  if (num != v->rowNum()) {
    throw new AzException(eyec, "shape mismatch"); 
  }
  AzCursor cur; 
  for ( ; ; ) {
    double val; 
    int ex = v->next(cur, val); 
    if (ex < 0) break; 
    val = (val > 0) ? val : -val; 
    elm[ex] = MAX(elm[ex], val);     
  }
}
#endif 
 
/*-------------------------------------------------------------*/
void AzDvect::add_abs(const AzDvect *v) 
{
  const char *eyec = "AzDvect::add_abs"; 
  if (num != v->rowNum()) {
    throw new AzException(eyec, "shape mismatch"); 
  }
  int ex;
  for (ex = 0; ex < num; ++ex) {
    if (v->elm[ex] > 0) {
      elm[ex] += v->elm[ex]; 
    }
    else {
      elm[ex] -= v->elm[ex]; 
    }
  }
}

#if 0 
/*-------------------------------------------------------------*/
void AzDvect::add_abs(const AzReadOnlyVector *v) 
{
  const char *eyec = "AzDvect::add_abs"; 
  if (num != v->rowNum()) {
    throw new AzException(eyec, "shape mismatch"); 
  }
  AzCursor cur; 
  for ( ; ; ) {
    double val; 
    int ex = v->next(cur, val); 
    if (ex < 0) break; 
    if (val > 0) {
      elm[ex] += val; 
    }
    else {
      elm[ex] -= val; 
    }
  }
}
#endif 

/*-------------------------------------------------------------*/
/* WARNING: This could be very slow.  */
void AzDvect::add(const AzReadOnlyVector *vect1, double coefficient) 
{
  const char *eyec = "AzDvect::add(vect,coeff)"; 

  if (num != vect1->rowNum()) {
    throw new AzException(eyec, "shape mismatch"); 
  }

  if (coefficient == 0) {
    return; 
  }

  /* vect1->rewind(); */
  AzCursor cursor; 
  for ( ; ; ) {
    double val1; 
    int ex1 = vect1->next(cursor, val1); 
    if (ex1 < 0) break;   

    if (coefficient != 1) {
      val1 *= coefficient; 
    }
    this->elm[ex1] += val1; 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::add(const AzSvect *vect1, double coefficient) 
{
  const char *eyec = "AzDvect::add(svect,coeff)"; 

  if (num != vect1->row_num) {
    throw new AzException(eyec, "shape mismatch"); 
  }

  if (coefficient == 0) {
    return; 
  }
  if (vect1->elm == NULL) return; 

  int ix; 
  for (ix = 0; ix < vect1->elm_num; ++ix) {
    double val = vect1->elm[ix].val; 
    if (val != 0) {
      elm[vect1->elm[ix].no] += coefficient * vect1->elm[ix].val; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDvect::add(const double *inp, int inp_num, double coefficient) 
{
  const char *eyec = "AzDvect::add(array,coeff)"; 
  if (num != inp_num) {
    throw new AzException(eyec, "shape mismatch"); 
  }
  if (coefficient == 0) return; 

  const double *ip = inp, *ip_end = inp+inp_num; 
  double *op = elm; 
  if (coefficient == 1) {
    for ( ; ip < ip_end; ++ip, ++op) {
      if (*ip != 0) *op += *ip; 
    }
  }
  else {
    for ( ; ip < ip_end; ++ip, ++op) {
      if (*ip != 0) *op += (*ip * coefficient); 
    }  
  }
}

/*-------------------------------------------------------------*/
void AzDvect::multiply(double val) 
{
  if (val == 1) {
    return; 
  }

  int ex; 
  for (ex = 0; ex < num; ++ex) {
    this->elm[ex] *= val; 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::scale(const AzDvect *dbles1, bool isInverse) 
/*  (NOTE)  x / 0 -> 0  */
{
  const char *eyec = "AzDvect::scale"; 
  if (num != dbles1->num) {
    throw new AzException(eyec, "shape mismatch"); 
  }
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != 0) {
      if (dbles1->elm[ex] == 0) elm[ex] = 0; 
      else if (dbles1->elm[ex] != 1) {
        if (isInverse) elm[ex] /= dbles1->elm[ex]; 
        else           elm[ex] *= dbles1->elm[ex]; 
      }
    }
  }
}

/*-------------------------------------------------------------*/
/* WARNING: not tested */
void AzDvect::scale(const AzSvect *vect1, bool isInverse) 
/*  (NOTE)  x / 0 -> 0  */
{
  const char *eyec = "AzDvect::scale (svect)"; 

  if (num != vect1->row_num) {
    throw new AzException(eyec, "shape mismatch"); 
  }

  int last_ex = -1; 
  int ix; 
  for (ix = 0; ix < vect1->elm_num; ++ix) {
    double val = vect1->elm[ix].val; 
    int ex = vect1->elm[ix].no; 

    int ex1; 
    for (ex1 = last_ex + 1; ex1 < ex; ++ex1) {
      elm[ex1] = 0; 
    }

    if (elm[ex] == 0) {
      elm[ex] = 0; 
    }
    else if (elm[ex] != 1) {
      if (isInverse) {
        elm[ex] /= val; 
      }
      else {
        elm[ex] *= val; 
      }
    }
    last_ex = ex; 
  }
  int ex1; 
  for (ex1 = last_ex + 1; ex1 < num; ++ex1) {
    elm[ex1] = 0; 
  }
}

/*-------------------------------------------------------------*/
double AzDvect::selfInnerProduct() const
{
  double n2 = 0; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != 0) {
      n2 += (elm[ex] * elm[ex]); 
    }
  }
  return n2; 
}

/*-------------------------------------------------------------*/
/* WARNING: This could be very slow.  */
double AzDvect::innerProduct(const AzReadOnlyVector *vect1) const
{
  const char *eyec = "AzDvect::getInenrProduct(vect)"; 

  if (vect1 == NULL) {
    return 0; 
  }

  if (num != vect1->rowNum()) {
    throw new AzException(eyec, "shape mismatch"); 
  }

  double iprod = 0; 
  /* vect1->rewind(); */
  AzCursor cursor; 
  for ( ; ; ) {
    double val; 
    int row = vect1->next(cursor, val); 
    if (row < 0) break; 

    iprod += val * elm[row]; 
  }

  return iprod; 
}

/*-------------------------------------------------------------*/
double AzDvect::innerProduct(const AzSvect *vect1) const
{
  const char *eyec = "AzDvect::getInenrProduct(svect)"; 

  if (vect1 == NULL || vect1->elm == NULL) {
    return 0; 
  }

  if (num != vect1->rowNum()) {
    throw new AzException(eyec, "shape mismatch"); 
  }

  double iprod = 0; 
  int ix; 
  for (ix = 0; ix < vect1->elm_num; ++ix) {
    double val = vect1->elm[ix].val; 
    if (val != 0) {
      iprod += val * elm[vect1->elm[ix].no]; 
    }
  }

  return iprod; 
}

/*-------------------------------------------------------------*/
double AzDvect::innerProduct(const AzDvect *dbles1) const
{
  const char *eyec = "AzDvect::getInenrProduct(doubles)"; 

  if (dbles1 == NULL) {
    return 0; 
  }

  if (num != dbles1->num) {
    throw new AzException(eyec, "shape mismatch"); 
  }

  double iprod = 0; 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    iprod += (elm[ex] * dbles1->elm[ex]); 
  }

  return iprod; 
}

/*-------------------------------------------------------------*/
double AzDvect::normalize()
{
  int ex; 
  double norm2 = 0; 
  for (ex = 0; ex < num; ++ex) {
    norm2 += (elm[ex] * elm[ex]); 
  }

  if (norm2 != 0) {
    norm2 = sqrt(norm2); 
    for (ex = 0; ex < num; ++ex) {
      elm[ex] /= norm2; 
    }
  }
  return norm2; 
}

/*-------------------------------------------------------------*/
double AzDvect::normalize1()
{
  int ex; 
  double sum = 0; 
  for (ex = 0; ex < num; ++ex) {
    sum += elm[ex]; 
  }

  if (sum != 0) {
    for (ex = 0; ex < num; ++ex) {
      elm[ex] /= sum; 
    }
  }
  return sum; 
}

/*-------------------------------------------------------------*/
int AzDvect::next(AzCursor &cursor, double &out_val) const
{
  int nonzero_ex = MAX(cursor.get(), 0); 
  for ( ; nonzero_ex < num; ++nonzero_ex) {
    if (elm[nonzero_ex] != 0) {
      break;
    }
  }

  cursor.set(nonzero_ex + 1);  /* prepare for next "next" */

  if (nonzero_ex < num) {
    out_val = elm[nonzero_ex]; 
    return nonzero_ex; 
  }

  /*---  end of the elements  ---*/
  out_val = 0; 
  return AzNone; 
}

/*-------------------------------------------------------------*/
void AzDvect::dump(const AzOut &out, const char *header, 
                   const AzStrArray *sp_row, 
                   int cut_num) const
{
  if (out.isNull()) return; 

  AzPrint o(out); 
  const char *my_header = ""; 
  if (header != NULL) my_header = header; 

  o.writeln(my_header); 

  int nz_num = nonZeroRowNum(); 
  int indent = 3; 
  o.printBegin(my_header, ",", "=", indent); 
  o.print("#row", num); 
  o.print("#non-zero", nz_num); 
  o.printEnd();

  if (cut_num > 0) {
    _dump(out, sp_row, cut_num); 
    return; 
  }

  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] == 0) {
      continue;  
    }

    /* [row] val (header) */
    o.printBegin("", " ", "=", indent); 
    o.inBrackets(ex, 4); 
    
//    o.print(elm[ex], 5, true); 
    o.print(elm[ex], 20, true); 
    
    if (sp_row != NULL) {
      const char *row_header = sp_row->c_str(ex); 
      o.inParen(row_header); 
    }
    o.printEnd(); 
  }
  o.flush();  
}

/*-------------------------------------------------------------*/
void AzDvect::_dump(const AzOut &out, 
                     const AzStrArray *sp_row, 
                     int cut_num) const
{
  AzPrint o(out); 

  AzIFarr ifa_nz; 
  nonZero(&ifa_nz); 

  ifa_nz.sort_Float(false); 
  ifa_nz.cut(cut_num); 
  int num = ifa_nz.size(); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int row; 
    double val = ifa_nz.get(ix, &row); 

    /* [row] val (header) */
    int indent = 3; 
    o.printBegin("", " ", "=", indent); 
    o.inBrackets(row, 4); 
    o.print(val, 5, true); 
    if (sp_row != NULL) {
      const char *row_header = sp_row->c_str(row); 
      o.inParen(row_header); 
    }
    o.printEnd();
  }
}

/*-------------------------------------------------------------*/
void AzDvect::cut(double min_val)
{
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    if (fabs(elm[ex]) < min_val) {
      elm[ex] = 0; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDvect::load(const AzIFarr *ifa_row_val)
{
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    elm[ix] = 0; 
  }

  int data_num = ifa_row_val->size(); 

  for (ix = 0; ix < data_num; ++ix) {
    int row; 
    double val = ifa_row_val->get(ix, &row);     
    if (row < 0 || row >= num) { /* out of range */
      throw new AzException("AzDvect::load", "Invalid input"); 
    }

    elm[row] = val; 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::square()
{
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    elm[ix] = elm[ix] * elm[ix]; 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::square()
{
  int col; 
  for (col = 0; col < col_num; ++col) {
    if (column[col] != NULL) {
      column[col]->square(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzDmat::average_sdev(AzDvect *v_avg, 
                          AzDvect *v_sdev) const 
{
  v_avg->reform(row_num); 
  if (v_sdev != NULL) v_sdev->reform(row_num); 
  if (col_num <= 0) return; 
  AzDvect v_avg2(row_num); 
  int cx;
  for (cx = 0; cx < col_num; ++cx) {
    v_avg->add(col(cx)); 
    if (v_sdev != NULL) {
      AzDvect v(col(cx)); v.square(); 
      v_avg2.add(&v); 
    }
  }
  v_avg->divide((double)col_num); 
  if (v_sdev != NULL) {
    v_avg2.divide((double)col_num); 
    AzDvect::sdev(v_avg, &v_avg2, v_sdev); 
  }
}

/*-------------------------------------------------------------*/
void AzDvect::rbind(const AzDvect *v) {
  int old_num = rowNum(); 
  int new_num = old_num + v->rowNum(); 
  resize(new_num);
  int ex;
  for (ex = 0; ex < v->rowNum(); ++ex) {
    elm[old_num+ex] = v->elm[ex]; 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::rbind(const AzDmat *m)
{
  if (m->colNum() != col_num) {
    throw new AzException("AzDmat::rbind", "#column must be the same"); 
  }
  int col; 
  for (col = 0; col < col_num; ++col) {
    column[col]->rbind(m->col(col)); 
  }
  row_num += m->rowNum(); 
  if (col_num > 0 && column[0]->rowNum() != row_num) {
    throw new AzException("AzDmat::rbind", "#row conflit"); 
  }
}

/*-------------------------------------------------------------*/
void AzDmat::undo_rbind(int added_len)
{
  if (added_len > row_num) {
    throw new AzException("AzDmat::rbind_rev", "#row is too small"); 
  }
  if (added_len <= 0) return; 
  int col; 
  for (col = 0; col < col_num; ++col) {
    AzDvect v(column[col]); 
    column[col]->set(column[col]->point(), row_num-added_len); 
  }
  row_num -= added_len; 
  if (col_num > 0 && column[0]->rowNum() != row_num) {
    throw new AzException("AzDmat::rbind", "#row conflit"); 
  }
}

/*------------------------------------------------------------*/ 
void AzDvect::polarize() {
  int org_num = num; 
  resize(org_num*2); 
  int ex;
  for (ex = 0; ex < org_num; ++ex) {
    if (elm[ex] < 0) {
      elm[org_num+ex] = -elm[ex]; 
      elm[ex] = 0; 
    }
  }
}

/*-------------------------------------------------------------*/
bool AzDvect::isSame(const AzReadOnlyVector *v) const {
  if (num != v->rowNum()) return false; 
  int ex;
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != v->get(ex)) return false; 
  }
  return true; 
}

/*-------------------------------------------------------------*/
bool AzDvect::isSame(const double *inp, int inp_num) const {
  if (num != inp_num) return false; 
  int ex;
  for (ex = 0; ex < num; ++ex) {
    if (elm[ex] != inp[ex]) return false; 
  }
  return true; 
}

/*-------------------------------------------------------------*/
bool AzDmat::isSame(const AzReadOnlyMatrix *m) const {
  if (m == NULL) throw new AzException("AzDmat::isSame", "null input"); 
  if (col_num != m->colNum() || row_num != m->rowNum()) return false;
  int cx;
  for (cx = 0; cx < col_num; ++cx) {
    if (!column[cx]->isSame(m->col(cx))) return false; 
  }
  return true; 
}

/*-------------------------------------------------------------*/
int AzDmat::set(const AzDmat *inp, const int *cols, int cnum,  /* new2old */
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
      column[my_col]->zeroOut();     
      ++negaindex; 
      continue; 
    }   
    if (col < 0 || col >= inp->col_num) {
      throw new AzException("AzDmat::set(inp,cols,cnum)", "invalid col#"); 
    }
    column[my_col]->set(inp->column[col]); 
  }
  return negaindex; 
}

/*-------------------------------------------------------------*/
void AzDmat::set(int col0, int col1, const AzDmat *inp, int i_col0)
{
  const char *eyec = "AzDmat::set(col0,col1,inp)"; 
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
    column[col]->set(inp->column[i_col]);       
  }
}  

/*-------------------------------------------------------------*/
void AzDmat::reduce(const int *cols, int cnum)  /* new2old */
{
  int new_col; 
  for (new_col = 0; new_col < cnum; ++new_col) {
    int old_col = cols[new_col];  
    if (old_col < 0 || old_col >= col_num) {
      throw new AzException("AzDmat::set(inp,cols,cnum)", "invalid col#"); 
    }
    if (new_col > 0 && old_col <= cols[new_col-1]) {
      throw new AzException("AzDmat::set(inp,cols,cnum)", "col#'s must be sorted"); 
    }
    if (new_col != old_col) {
      column[new_col]->set(column[old_col]); 
    }
  }
  resize(cnum); 
}
