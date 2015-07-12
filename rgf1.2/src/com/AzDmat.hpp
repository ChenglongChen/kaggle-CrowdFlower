/* * * * *
 *  AzDmat.hpp 
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

#ifndef _AZ_DMAT_HPP_
#define _AZ_DMAT_HPP_

#include "AzUtil.hpp"
#include "AzStrArray.hpp"
#include "AzSmat.hpp"
#include "AzReadOnlyMatrix.hpp"

//! dense vector 
class AzDvect : /* implements */ public virtual AzReadOnlyVector {
protected:
  int num; 
  double *elm; /* updated only through AzBaseArray functions */
               /* must not be NULL no matter what ... */
  AzBaseArray<double> a; 

  inline void _release() {
    a.free(&elm); num = 0; 
  }

public:
  AzDvect() : num(0), elm(NULL) {} 
  AzDvect(int inp_num) : num(0), elm(NULL) {
    reform(inp_num); 
  }
  AzDvect(const double inp[], int inp_num) : num(0), elm(NULL) {
    set(inp, inp_num); 
  }
  template <class T>
  AzDvect(const T inp[], int inp_num) : num(0), elm(NULL) {
    set(inp, inp_num); 
  }
  
  AzDvect(const AzDvect *inp) : num(0), elm(NULL) {
    set(inp); 
  }
  inline void set(const AzDvect *inp, double coeff=1) {
    _set(inp->elm, inp->num); 
    if (coeff != 1) multiply(coeff); 
  }
  inline void set_chk(const AzDvect *inp, double coeff=1) {
    if (num != inp->num) {
      throw new AzException("AzDvect::set_chk", "dimensionality mismatch"); 
    }
    set(inp, coeff); 
  }

  AzDvect(const AzDvect &inp) : num(0), elm(NULL) {
    set(inp.elm, inp.num); 
  }
  AzDvect & operator =(const AzDvect &inp) {
    if (this == &inp) return *this; 
    set(&inp); 
    return *this; 
  }
  AzDvect(const AzReadOnlyVector *inp) /* copy */
    : num(0), elm(NULL) {
    set(inp); 
  }
  void set(const AzReadOnlyVector *inp); 
  void set(const AzSvect *inp);  /* not tested */
  
  ~AzDvect() {}

  AzDvect(AzFile *file) : num(0), elm(NULL) {
    _read(file); 
  }

  void reset() {
    _release(); 
  }

  inline void reform(int new_row_num) {
    _reform_noset(new_row_num); 
    zeroOut(); 
  }
  inline void reform_chk(int rnum, bool do_chk, const char *msg) {
    if (do_chk && num != rnum) {
      throw new AzException("AzDvect::reform_chk", msg); 
    }
    reform(rnum); 
  }
  void resize(int new_row_num); 

  void write(AzFile *file); 

  void write(const char *fn) {
    AzFile file(fn); 
    file.open("wb"); 
    write(&file); 
    file.close(true); 
  }
  void read(const char *fn) {
    _release(); 
    AzFile file(fn); 
    file.open("rb"); 
    read(&file); 
    file.close(); 
  }
  void read(AzFile *file) {
    _release(); 
    _read(file); 
  }

  inline int size() const { return num; }
  inline int rowNum() const { return num; }  

  inline void destroy() {
    _release();  
  }

  bool isZero() const; 

  void load(const AzIFarr *ifa_row_val); 

  void cut(double min_val); 

  void binarize(); 
  void binarize1(); 

  void values(const int exs[], 
                 int ex_num, 
                 AzIFarr *ifa_ex_value); /* output */
  void nonZero(AzIFarr *ifq) const; 
  void nonZeroRowNo(AzIntArr *iq) const; 
  int nonZeroRowNum() const; 
  void all(AzIFarr *ifa) const {
    ifa->prepare(num); 
    int row; 
    for (row = 0; row < num; ++row) ifa->put(row, elm[row]); 
  }
  void zeroRowNo(AzIntArr *ia) const {
    ia->reset(); 
    int row; 
    for (row = 0; row < num; ++row) if (elm[row] == 0) ia->put(row); 
  }
  
  inline void set(int row, double val) {
    checkIndex(row, "AzDvect::set"); 
    elm[row] = val; 
  }
  inline void set(const double *inp, int inp_num) {
    if (inp == NULL || inp_num < 0) {
      throw new AzException("AzDvect::set(array)", "Invalid input"); 
    }
    _set(inp, inp_num); 
  }

  inline void set_chk(const double *inp, int inp_num) {
    if (num != inp_num) {
      throw new AzException("AzDvect::set_chk(array)", "shape mismatch"); 
    }
    set(inp, inp_num); 
  }
  template <class T> 
  inline void set_chk(const T *inp, int inp_num) {
    if (num != inp_num) {
      throw new AzException("AzDvect::set_chk(array)", "shape mismatch"); 
    }
    set(inp, inp_num); 
  }
  template <class T>
  void set(const T *inp, int inp_num) {
    if (inp == NULL || inp_num < 0) {
      throw new AzException("AzDvect::set(inp_tmpl, num)", "Invalid input"); 
    }
    if (inp_num != num) _reform_noset(inp_num); 
    int ex; 
    for (ex = 0; ex < num; ++ex) elm[ex] = inp[ex];  
  }
 
  void set(double val); 

  inline double get(int row) const {
    checkIndex(row, "AzDvect::get"); 
    return elm[row]; 
  }
  inline double get_nochk(int row) const {
    return elm[row]; 
  }

  inline void add(int row, double val) {
    checkIndex(row, "AzDvect::add"); 
    if (val != 0) {
      elm[row] += val; 
    }
  }
  inline void add(double val, const AzIntArr *ia_rows) {
    if (ia_rows == NULL) return; 
    add(val, ia_rows->point(), ia_rows->size()); 
  }
  void add_nochk(double val, const AzIntArr *ia_rows) {
    add_nochk(val, ia_rows->point(), ia_rows->size()); 
  }
  void add(double val, const int *rows, int rows_num); 
  void add_nochk(double val, const int *rows, int rows_num); 
  void add(double val); 
  void add(const double *inp, int inp_num, double coefficient=1); 
  void add(const AzReadOnlyVector *vect1, double coefficient=1); 
  void add(const AzSvect *vect1, double coefficient=1); 
  inline void add(const AzDvect *vect1, double coefficient=1) {
    if (vect1 == NULL) throw new AzException("AzDvect::add(vect)", "null input"); 
    add(vect1->elm, vect1->num, coefficient); 
  }

  inline void multiply(int row, double val) {
    checkIndex(row, "AzDvect::multiply"); 
    if (val != 1) {
      elm[row] *= val; 
    }
  }

  void abs() {
    int ex;
    for (ex = 0; ex < num; ++ex) {
      if (elm[ex] < 0) elm[ex] = -elm[ex]; 
    }
  }
  double sum() const; 
  double absSum() const; 
  double sum(const int *row, int row_num) const; 
  double absSum(const int *row, int row_num) const; 
  inline double sum(const AzIntArr *ia_rows) const {
    if (ia_rows == NULL) return sum(); 
    return sum(ia_rows->point(), ia_rows->size()); 
  }
  inline double absSum(const AzIntArr *ia_rows) const {
    if (ia_rows == NULL) return absSum(); 
    return absSum(ia_rows->point(), ia_rows->size()); 
  }
  inline double average(const AzIntArr *ia_rows=NULL) const {
    if (num == 0) return 0; 
    if (ia_rows == NULL) return sum() / (double)num; 
    int pop = ia_rows->size(); 
    return sum(ia_rows->point(), pop) / (double)pop; 
  }

  void multiply(double val); 
  inline void divide(double val) {
    if (val == 0) throw new AzException("AzDvect::divide", "division by zero"); 
    multiply((double)1/val); 
  }

  void scale(const AzSvect *vect1, bool isInverse=false); /* not tested */
  void scale(const AzDvect *dbles1, bool isInverse=false); 

  double innerProduct(const AzReadOnlyVector *vect1) const; 
  double innerProduct(const AzSvect *vect1) const; 
  double innerProduct(const AzDvect *dbles1) const; 
  double selfInnerProduct() const; 
  inline double squareSum() const { return selfInnerProduct(); }

  double normalize(); 
  double normalize1(); 

  inline void zeroOut() {
    int ex; 
    for (ex = 0; ex < num; ++ex) elm[ex] = 0; 
  }
  
  int next(AzCursor &cursor, double &out_val) const; 

  double max(int *out_row = NULL) const;  
  double maxAbs(int *out_row = NULL, double *out_real_val = NULL) const; 

  void max_abs(const AzDvect *v);  /* keep max */
  void add_abs(const AzDvect *v); 

  double min(int *out_row = NULL) const;  
  double max(const AzIntArr *ia_dx, int *out_row = NULL) const;  
  double min(const AzIntArr *ia_dx, int *out_row = NULL) const;  

  void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, 
            int cut_num = -1) const; 

  inline double *point_u() {
    return elm; 
  }
  inline const double *point() const {
    return elm; 
  }

  void square(); 

  static inline double sum(const double val[], int num) {
    double sum = 0; 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      sum += val[ix]; 
    }
    return sum; 
  }
  static inline bool isNull(const AzDvect *v) {
    if (v == NULL) return true; 
    if (v->rowNum() == 0) return true; 
    return false; 
  }

  void mysqrt() {
    if (elm == NULL) return; 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      if (elm[ix]<0) throw new AzException("AzDvect:;sqrt", "Negative component"); 
      elm[ix]=sqrt(elm[ix]); 
    }
  }
  static void sdev(const AzDvect *v_avg, const AzDvect *v_avg2, AzDvect *v_sdev) {
    v_sdev->add(v_avg2); 
    AzDvect v(v_avg); v.scale(v_avg); 
    v_sdev->add(&v, -1); /* avg(x^2)-avg(x)^2 */
    v_sdev->mysqrt(); 
  }
  void rbind(const AzDvect *v); 
  void polarize(); 
  bool isSame(const double *inp, int inp_num) const; 
  inline bool isSame(const AzDvect *v) const {
    if (v == NULL) throw new AzException("AzDvect::isSame", "null input"); 
    return isSame(v->elm, v->num); 
  }
  bool isSame(const AzReadOnlyVector *v) const; 
protected:
  /*-------------------------------------------------------------*/
  inline void _set(const double *inp, int inp_num) {
    _reform_noset(inp_num); 
    memcpy(elm, inp, sizeof(inp[0]) * num); 
  }
  /*-------------------------------------------------------------*/
  void _reform_noset(int inp_num) { /* must be immediately followed by setting values */
    if (num == inp_num) {
      return; 
    }
    const char *eyec = "AzDvect::_reform_noset"; 
    a.free(&elm); num = 0; 
    if (inp_num < 0) {
      throw new AzException(eyec, "dim must be non-negative"); 
    }
    num = inp_num; 
    if (num > 0) {
      a.alloc(&elm, num, eyec, "elm"); 
    }
  }
  /*-------------------------------------------------------------*/

  void _read(AzFile *file); 

  void _swap(); 

  void _dump(const AzOut &out, const AzStrArray *sp_row, 
             int cut_num = -1) const; 
  inline void checkIndex(int row, const char *eyec) const {
    if (row < 0 || row >= num) {
      throw new AzException(eyec, "index is out of range"); 
    }
  }
}; 

//! dense matrix 
class AzDmat : /* implements */ public virtual AzReadOnlyMatrix {
protected:
  bool isLocked; 
  int col_num, row_num; 
  AzDvect **column;  /* updated only through AzPtrArray functions */
                     /* must not be NULL no matter what ... */
  AzObjPtrArray<AzDvect> a; 
  AzDvect dummy_zero; 

  void _release() {
    checkLock("_release"); 
    a.free(&column); col_num = 0; 
    row_num = 0; 
    dummy_zero.reform(0); 
  }
public: 
  AzDmat() : row_num(0), col_num(0), column(NULL), isLocked(false) {}
  AzDmat(int row_num, int col_num) 
           : row_num(0), col_num(0), column(NULL), isLocked(false)  {
    reform(row_num, col_num); 
  }
  AzDmat(const AzDmat *inp) 
           : row_num(0), col_num(0), column(NULL), isLocked(false)  {
    set(inp);  
  }

  /*----------------------------------------------------*/
  inline void set(const AzDmat *m_inp, double coeff=1) {
    if (m_inp == NULL) {
      throw new AzException("AzDmat::set(Dmat)", "null input"); 
    }
    _set(m_inp); 
    if (coeff != 1) {
      multiply(coeff); 
    }
  }
  /*----------------------------------------------------*/
  inline void set_chk(const AzDmat *inp, double coeff=1) {
    if (row_num != inp->row_num || col_num != inp->col_num) {
      throw new AzException("AzDmat::set_chk", "dimensionality mismatch"); 
    }
    set(inp, coeff); 
  }
  
  AzDmat(const AzDmat &inp) 
           : row_num(0), col_num(0), column(NULL), isLocked(false)  {
    set(&inp); 
  }
  AzDmat & operator =(const AzDmat &inp) {
    if (this == &inp) return *this; 
    set(&inp); 
    return *this; 
  }

  AzDmat(const AzReadOnlyMatrix *inp) /* copy */
           : row_num(0), col_num(0), column(NULL), isLocked(false)  {
    initialize(inp); 
  }
  void set(const AzReadOnlyMatrix *inp) {
    _release(); 
    initialize(inp); 
  }

  AzDmat(AzFile *file) 
           : row_num(0), col_num(0), column(NULL), isLocked(false)  {
    _read(file); 
  }
  ~AzDmat() {}

  void reset() {
    _release(); 
  }

  inline void lock() {
    isLocked = true; 
  }
  inline void unlock() {
    isLocked = false; 
  }

  void resize(int new_col_num); 
  void resize(int new_row_num, int new_col_num); 
  inline void reform(int rnum, int cnum) {
    _reform(rnum, cnum, true); 
  }
  inline void reform_chk(int rnum, int cnum, const char *msg) {
    reform_chk(rnum, cnum, true, msg); 
  }
  inline void reform_chk(int rnum, int cnum, bool do_chk, const char *msg) {
    if (do_chk && (row_num != rnum || col_num != cnum)) {
      throw new AzException("AzDmat::reform_or_chk", msg); 
    }
    reform(rnum, cnum); 
  }

  void destroy() {
    _release(); 
  }

  /*--------------------------------------------------*/
  void write(AzFile *file); 
  void read(AzFile *file) {
    _release(); 
    _read(file); 
  }
  void write(const char *fn) {
    AzFile::write(fn, this); 
  }
  void read(const char *fn) {
    _release(); 
    AzFile::read(fn, this); 
  }  

  void convert(AzSmat *m_out); 
  inline void convert_to(AzSmat *m_out) { convert(m_out); }

  void transpose(AzDmat *m_out, int col_begin = -1, int col_end = -1) const; 
  void transpose_to(AzDmat *m_out, int col_begin = -1, int col_end = -1) const { 
    transpose(m_out, col_begin, col_end); 
  }
  void transpose_from(const AzSmat *m_inp); 

  void cut(double min_val); 

  inline void set(double val) {
    int col; 
    for (col = 0; col < colNum(); ++col) {
      col_u(col)->set(val); 
    }
  }
  int set(const AzDmat *inp, const int *cols, int cnum, bool do_zero_negaindex=false); 
  void set(int col0, int col1, const AzDmat *inp, int icol0=0); /* this[col0:col1-1] <- inp[icol0::(col1-col0)] */
  inline void reduce(const AzIntArr *ia) {
    if (ia == NULL) throw new AzException("AzDmat::reduce", "null input"); 
    reduce(ia->point(), ia->size()); 
  }
  void reduce(const int *cols, int cnum); 
  void set(const AzDmat *inp, int col0, int col1) { /* this <- inp[col0:col1-1] */
    if (col0 < 0 || col1 > inp->col_num || col1-col0 <= 0) {
      throw new AzException("AzDmat::set(m,c0,c1)", "invalid request"); 
    }
    AzIntArr ia; ia.range(col0, col1); 
    set(inp, ia.point(), ia.size()); 
  }
  void set(int row, int col, double val); 
  void add(int row, int col, double val); 
  void add(const AzDmat *inp, double coeff=1); 
  void add(const AzSmat *inp, double coeff=1); 
  void add(double val); 
  
  void multiply(int row, int col, double val); 
  void multiply(double val); 
  inline void divide(double val) {
    if (val == 0) throw new AzException("AzDmat::divide", "division by zero"); 
    multiply((double)1/val); 
  }
  
  double get(int row, int col) const; 
  inline double get_nochk(int row, int col) const { /* for speed */
    return column[col]->get_nochk(row); 
  }
  
  /*---  this never returns NULL  ---*/
  inline AzDvect *col_u(int col) {
    if (col < 0 || col >= col_num) {
      throw new AzException("AzDmat::col_u", "col# is out of range"); 
    }
    if (column[col] == NULL) {
      column[col] = new AzDvect(row_num); 
    }
    return column[col]; 
  }

  /*---  this never returns NULL ---*/
  inline const AzDvect *col(int col) const {
    if (col < 0 || col >= col_num) {
      throw new AzException("AzDmat::col", "col# is out of range"); 
    }
    if (column[col] == NULL) {
      if (dummy_zero.rowNum() != row_num) {
        throw new AzException("AzDmat::col", "wrong dummy_zero"); 
      }
      return &dummy_zero; 
    }
    return column[col]; 
  }

  inline int rowNum() const { return row_num; }
  inline int colNum() const { return col_num; }

  void normalize(); 
  void normalize1(); 

  void binarize(); 
  void binarize1(); 

  bool isZero() const; 
  bool isZero(int col) const; 

  void zeroOut(); 

  double max(int *out_row, int *out_col) const; 

  inline void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, const AzStrArray *sp_col = NULL, 
            int cut_num = -1) const {
    dump(out, header, -1, sp_row, sp_col, cut_num); 
  }
  void dump(const AzOut &out, const char *header, int max_col, 
            const AzStrArray *sp_row = NULL, const AzStrArray *sp_col = NULL, 
            int cut_num = -1) const; 
            
  void scale(const AzSvect *vect1, bool isInverse=false); 
  void scale(const AzDvect *vect1, bool isInverse=false);
  void scale(const AzDmat *m, bool isInverse=false); 
  void abs() {
    int col; 
    for (col = 0; col < col_num; ++col) if (column[col] != NULL) column[col]->abs(); 
  }
  
  void square(); 

  inline void load(int col, const AzIFarr *ifa_row_val) {
    col_u(col)->load(ifa_row_val); 
  }

  inline void average(AzDvect *v_avg) const {
    average_sdev(v_avg, NULL); 
  }
  void average_sdev(AzDvect *v_avg, 
                    AzDvect *v_stdev) const; 

  /*---  x times transpose(x)  ---*/
  void add_xxT(const AzDvect *v) {
    if (v->rowNum() != rowNum() || v->rowNum() != colNum()) {
      throw new AzException("AzDmat::add_xxT", "shape mismatch");
    }
    const double *val = v->point(); 
    int rx; 
    for (rx = 0; rx < v->rowNum(); ++rx) {
      col_u(rx)->add(v, val[rx]); 
    }
  }
  
  void xxT(const AzDmat *m) {
    reform(m->rowNum(), m->rowNum()); 
    int cx;
    for (cx = 0; cx < m->colNum(); ++cx) {
      add_xxT(m->col(cx)); 
    }
  }

  /*---  transpose(x) times x  ---*/
  void xTx(const AzDmat *m) {
    reform(m->colNum(), m->colNum()); 
    int cx, cx1;
    for (cx = 0; cx < m->colNum(); ++cx) {
      const AzDvect *v = m->col(cx); 
      for (cx1 = cx; cx1 < m->colNum(); ++cx1) {
        double val = v->innerProduct(m->col(cx1)); 
        set(cx, cx1, val); 
        set(cx1, cx, val); 
      }
    }
  }

  void rbind(const AzDmat *m); 
  void undo_rbind(int added_len); 
  void cbind(const AzDmat *m) {
    if (colNum() <= 0 || rowNum() <= 0) {
      set(m); 
      return; 
    }
    if (m->rowNum() != rowNum()) {
      throw new AzException("AzDmat::cbind", "shape mismatch"); 
    }
    int org_num = colNum(); 
    int new_num = org_num + m->colNum(); 
    resize(new_num); 
    int cx;
    for (cx = 0; cx < m->colNum(); ++cx) {
      col_u(org_num+cx)->set(m->col(cx)); 
    }
  }    
  void cbind(const AzDataArr<AzDmat> *am) {
    if (am->size() <= 0) {
      reform(0,0); 
      return; 
    }
    int rnum = am->point(0)->rowNum(); 
    int cnum = 0; 
    int ix; 
    for (ix = 0; ix < am->size(); ++ix) cnum += am->point(ix)->colNum(); 
    reform(rnum, cnum); 
    int col = 0; 
    for (ix = 0; ix < am->size(); ++ix) {
      const AzDmat *m = am->point(ix);   
      if (m->rowNum() != rnum) {
        throw new AzException("AzDmat::cbind(am)", "Invalid #row"); 
      }      
      set(col, col+m->colNum(), m);
      col += m->colNum(); 
    }  
  }
  
  bool isSame(const AzReadOnlyMatrix *m) const; 

  template <class T>
  void set(const T *inp, int inp_num) {
    if (inp_num != row_num * col_num) {
      throw new AzException("AzDmat::set(array)", "length conflict"); 
    }
    int cur = 0; 
    int col; 
    for (col = 0; col < col_num; ++col) {
      column[col]->set_chk(inp+cur, row_num); 
      cur += row_num; 
    }
  }
  double sum() const {
    double val = 0; 
    int cx; 
    for (cx = 0; cx < col_num; ++cx) {
      val += column[cx]->sum(); 
    }
    return val; 
  }
  
  double min() const {
    double mymin = 0; 
    int cx; 
    for (cx = 0; cx < colNum(); ++cx) {
      double colmin = col(cx)->min(); 
      if (cx == 0 || colmin < mymin) mymin = colmin; 
    }
    return mymin; 
  }
  double max() const {
    double mymax = 0; 
    int cx; 
    for (cx = 0; cx < colNum(); ++cx) {
      double colmax = col(cx)->max(); 
      if (cx == 0 || colmax > mymax) mymax = colmax; 
    }
    return mymax; 
  }
  double squareSum() const {
    double sum = 0; 
    int cx; 
    for (cx = 0; cx < col_num; ++cx) sum += col(cx)->selfInnerProduct(); 
    return sum; 
  }
  
protected:
  inline void checkLock(const char *who) {
    if (isLocked) {
      throw new AzException("AzDmat::checkLock", 
            "Illegal attempt to change the pointers of a locked matrix by", who); 
    }
  }
  void _reform(int rnum, int cnum, bool do_zeroOut); 
  void _set(const AzDmat *inp); 
    
  void _read(AzFile *file); 

  /*---  call _release() before calling initialize(...)  ---*/
  /*---  dimensionality may be changed  ---*/
  void initialize(const AzReadOnlyMatrix *inp); 

  void _transpose(AzDmat *m_out, int col_begin, int col_end) const; 
}; 

/*-----------------------------------------------------------------------*/
class AzDSmat {
protected: 
  const AzDmat *md; 
  const AzSmat *ms; 
public: 
  AzDSmat() : md(NULL), ms(NULL) {}
  AzDSmat(const AzDmat *md_inp) {
    reset(md_inp); 
  }
  AzDSmat(const AzSmat *ms_inp) {
    reset(ms_inp); 
  }
  inline void reset(const AzDmat *md_inp) {
    md = md_inp; 
    ms = NULL; 
  }
  inline void reset(const AzSmat *ms_inp) {
    ms = ms_inp; 
    md = NULL; 
  }
  
  inline int rowNum() const { 
    if (md != NULL) return md->rowNum(); 
    if (ms != NULL) return ms->rowNum(); 
    return 0; 
  }
  inline int colNum() const { 
    if (md != NULL) return md->colNum(); 
    if (ms != NULL) return ms->colNum(); 
    return 0; 
  }
  inline void add_to(AzDvect *v_dst, int col, double coeff) const {
    if      (md != NULL) v_dst->add(md->col(col), coeff); /* dst += md[,col]*coeff */
    else if (ms != NULL) v_dst->add(ms->col(col), coeff); 
    else                 throw new AzException("AzDSmat::add", "No data"); 
  }
  inline void add_to(AzDmat *m_dst, double coeff) const {
    if      (md != NULL) m_dst->add(md, coeff); /* dst += md*coeff */
    else if (ms != NULL) m_dst->add(ms, coeff); 
    else                 throw new AzException("AzDSmat::add", "No data"); 
  }
  inline bool is_ready() const { return (md != NULL || ms != NULL); }
  inline bool is_dense() const { return (md != NULL); }
  inline bool is_sparse() const { return (ms != NULL); }
  inline const AzSmat *sparse() const { 
    if (ms == NULL) throw new AzException("AzDSmat::sparse()", "No data"); 
    return ms; 
  }
  inline const AzDmat *dense() const {
    if (md == NULL) throw new AzException("AzDSmat::dense()", "No data"); 
    return md;
  }
};

/*********************************************************************/
class AzDmatVar {
protected: 
  AzDataArr<AzDvect> av; 
public: 
  AzDmatVar() {}
  void reset(int data_num) { av.reset(data_num); }  
  int dataNum() const { return av.size(); }
  int length(int idx) const { 
    check_idx(idx, "length(idx)"); 
    return av.point(idx)->rowNum(); 
  }
  AzDvect *vec_u(int idx) {
    check_idx(idx, "AzDmatVar::ptr_u"); 
    return av.point_u(idx); 
  }
  const AzDvect *vec(int idx) const {
    check_idx(idx, "AzDmatVar::ptr"); 
    return av.point(idx); 
  }
  void destroy() { reset(0); }
  void reset() { reset(0); }
protected: 
  inline void check_idx(int idx, const char *msg="") const {
    if (idx < 0 || idx >= av.size()) {
      throw new AzException("AzDmatVar::check_idx", "invalid data index", msg); 
    }
  }  
}; 
#endif 


