/* * * * *
 *  AzSmat.hpp 
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

#ifndef _AZ_SMAT_HPP_
#define _AZ_SMAT_HPP_

#include "AzUtil.hpp"
#include "AzStrArray.hpp"
#include "AzReadOnlyMatrix.hpp"

/* Changed AZ_MTX_FLOAT from single-precision to double-precision  */
typedef double AZ_MTX_FLOAT; 
#define _checkVal(x) 
/* static double _too_large_ = 16777216; */
/* static double _too_small_ = -16777216; */

typedef struct {
  int no; 
  AZ_MTX_FLOAT val; 
} AZI_VECT_ELM; 

class AzSmat; 

//! sparse vector 
class AzSvect : /* implements */ public virtual AzReadOnlyVector {
protected:
  int row_num; 
  AZI_VECT_ELM *elm; 
  AzBaseArray<AZI_VECT_ELM> a; 
  int elm_num; 

  void _release() {
    a.free(&elm); elm_num = 0; 
  }

public:
  friend class AzDmat; 
  friend class AzDvect; 
  friend class AzPmatSpa; 
  
  AzSvect() : row_num(0), elm(NULL), elm_num(0) {}
  AzSvect(int inp_row_num, bool asDense=false) : row_num(0), elm(NULL), elm_num(0) {
    initialize(inp_row_num, asDense); 
  }
  AzSvect(const AzSvect *inp) : row_num(0), elm(NULL), elm_num(0) {
    row_num = inp->row_num;  
    set(inp); 
  }
  AzSvect(const AzSvect &inp) : row_num(0), elm(NULL), elm_num(0) {
    row_num = inp.row_num; 
    set(&inp); 
  }
  AzSvect & operator =(const AzSvect &inp) {
    if (this == &inp) return *this; 
    _release(); 
    row_num = inp.row_num; 
    set(&inp); 
    return *this; 
  }
  AzSvect(const AzReadOnlyVector *inp) : row_num(0), elm(NULL), elm_num(0) {
    row_num = inp->rowNum();  
    set(inp); 
  }
  AzSvect(AzFile *file) : row_num(0), elm(NULL), elm_num(0) {
    _read(file); 
  }
  ~AzSvect() {}

  void read(AzFile *file) {
    _release(); 
    _read(file); 
  }

  void resize(int new_row_num); /* new #row must be greater than #row */
  void reform(int new_row_num, bool asDense=false); 
  void change_rowno(int new_row_num, const AzIntArr *ia_old2new, bool do_zero_negaindex=false); 
  
  void write(AzFile *file); 

  inline int rowNum() const { return row_num; }  
  void load(const AzIntArr *ia_row, double val, bool do_ignore_negative_rowno=false); 
  void load(const AzIFarr *ifa_row_val); 
  void load(AzIFarr *ifa_row_val) {
    ifa_row_val->sort_Int(true); 
    load((const AzIFarr *)ifa_row_val); 
  }   
  bool isZero() const;
  bool isOneOrZero() const; 

  void cut(double min_val); 
  void only_keep(int num); /* keep this number of components with largest fabs */
  void zerooutNegative(); /* not tested */
  void nonZero(AzIFarr *ifq, const AzIntArr *ia_sorted_filter) const; 

  void filter(const AzIntArr *ia_sorted, /* must be sorted; can have duplications */
              /*---  output  ---*/
              AzIFarr *ifa_nonzero, 
              AzIntArr *ia_zero) const; 

  int nonZeroRowNo() const; /* returns the first one */
  void nonZero(AzIFarr *ifa) const; 
  void all(AzIFarr *ifa) const;  /* not tested */
  void zeroRowNo(AzIntArr *ia) const;  /* not tested */
  void nonZeroRowNo(AzIntArr *intq) const; 
  int nonZeroRowNum() const; 

  void set_inOrder(int row_no, double val); 
  void set(int row_no, double val); 
  void set(double val); 
  void set(const AzSvect *vect1, double coefficient=1); 
  void set(const AzReadOnlyVector *vect1, double coefficient=1);  

  double get(int row_no) const; 

  double sum() const; 
  double absSum() const; 

  void add(int row_no, double val); 

  void multiply(int row_no, double val); 
  void multiply(double val); 
  inline void divide(double val) {
    if (val == 0) throw new AzException("AzSvect::divide", "division by zero"); 
    multiply((double)1/val); 
  }
 
  void plus_one_log();  /* x <- log(x+1) */

  void scale(const double *vect1); 

  double selfInnerProduct() const; 
  inline double squareSum() const { return selfInnerProduct(); }
  
  void log_of_plusone(); /* log(x+1) */
  double normalize(); 
  double normalize1(); 
  void binarize();  /* posi -> 1, nega -> -1 */
  void binarize1(); /* nonzero -> 1 */
  
  void clear(); 
  void zeroOut(); 

  int next(AzCursor &cursor, double &out_val) const; 

  double minPositive(int *out_row_no = NULL) const; 
  double min(int *out_row_no = NULL, 
                     bool ignoreZero=false) const; 
  double max(int *out_row_no = NULL, 
                    bool ignoreZero=false) const; 
  double maxAbs(int *out_row_no = NULL, double *out_real_val = NULL) const; 

  void polarize(); 
  
  void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, 
            int cut_num = -1) const; 

  void clear_prepare(int num); 
  bool isSame(const AzSvect *inp) const; 
  void cap(double cap_val); 

protected:
  void _read(AzFile *file); 
  void _swap(); 

  void initialize(int inp_row_num, bool asDense); 
  int to_insert(int row_no); 
  int find(int row_no, int from_this = -1) const; 
  int find_forRoom(int row_no, 
                   int from_this = -1, 
                    bool *out_isFound = NULL) const; 

  void _dump(const AzOut &out, const AzStrArray *sp_row, 
             int cut_num = -1) const; 

  inline int inc() const {
    return MIN(4096, MAX(32, elm_num)); 
  }
}; 

//! sparse matrix 
class AzSmat : /* implements */ public virtual AzReadOnlyMatrix {
protected:
  int col_num, row_num; 
  AzSvect **column; /* NULL if and only if col_num=0 */
  AzObjPtrArray<AzSvect> a; 
  AzSvect dummy_zero; 
  void _release() {
    a.free(&column); col_num = 0; 
    row_num = 0; 
  }
public: 
  AzSmat() : col_num(0), row_num(0), column(NULL) {}
  AzSmat(int inp_row_num, int inp_col_num, bool asDense=false)
    : col_num(0), row_num(0), column(NULL) {
    initialize(inp_row_num, inp_col_num, asDense); 
  }
  AzSmat(const AzSmat *inp) : col_num(0), row_num(0), column(NULL) {
    initialize(inp); 
  }
  AzSmat(const AzSmat &inp) : col_num(0), row_num(0), column(NULL) {
    initialize(&inp); 
  }
  AzSmat & operator =(const AzSmat &inp) {
    if (this == &inp) return *this; 
    _release(); 
    initialize(&inp); 
    return *this; 
  }

  AzSmat(AzFile *file) : col_num(0), row_num(0), column(NULL) {
    _read(file); 
  }
  AzSmat(const char *fn) : col_num(0), row_num(0), column(NULL) {
    read(fn); 
  }
  ~AzSmat() {}
  void read(AzFile *file) {
    _release(); 
    _read(file); 
  }
  void read(const char *fn) {
    AzFile file(fn); 
    file.open("rb"); 
    read(&file); 
    file.close(); 
  }
  inline void reset() {
    _release(); 
  }
  void resize(int new_col_num); 
  void resize(int new_row_num, int new_col_num); /* new #row must be greater than #row */
  void reform(int row_num, int col_num, bool asDense=false); 

  void change_rowno(int new_row_num, const AzIntArr *ia_old2new, bool do_zero_negaindex=false); 
  int nonZeroRowNo(AzIntArr *ia_nzrows) const; 
  
  void write(AzFile *file); 
  void write (const char *fn) {
    AzFile::write(fn, this); 
  }

  bool isZero() const; 
  bool isZero(int col_no) const; 
  bool isOneOrZero() const; 
  bool isOneOrZero(int col_no) const; 
  
  int nonZeroColNum() const; 
  double nonZeroNum(double *ratio=NULL) const; 

  void transpose(AzSmat *m_out, int col_begin = -1, int col_end = -1) const; 
  inline void transpose_to(AzSmat *m_out, int col_begin = -1, int col_end = -1) const {
    transpose(m_out, col_begin, col_end); 
  }
  void cut(double min_val); 
  void only_keep(int num); /* keep this number of components per column with largest fabs */
  void zerooutNegative(); /* not tested */

  void set(const AzSmat *inp); 
  void set(const AzSmat *inp, int col0, int col1); /* this <- inp[,col0:col1-1] */
  int set(const AzSmat *inp, const int *cols, int cnum, bool do_zero_negaindex=false); /* return #negatives in cols */
  void set(int col0, int col1, const AzSmat *inp, int icol0=0); /* this[col0:col1-1] <- inp[icol0::(col1-col0)] */
  inline void reduce(const AzIntArr *ia_cols) { 
    if (ia_cols == NULL) throw new AzException("AzSmat::reduce", "null input"); 
    reduce(ia_cols->point(), ia_cols->size());
  }
  void reduce(const int *cols, int cnum);  /* new2old; must be sorted; this <- selected columns of this */
  void set(const AzReadOnlyMatrix *inp);  
  void set(int row_no, int col_no, double val); 
  void set(double val); 

  void add(int row_no, int col_no, double val); 

  void multiply(int row_no, int col_no, double val); 
  void multiply(double val); 
  inline void divide(double val) {
    if (val == 0) throw new AzException("AzSmat::divide", "division by zero"); 
    multiply((double)1/val); 
  }

  void plus_one_log();  /* x <- log(x+1) */  
  double get(int row_no, int col_no) const; 

  double sum() const {
    if (column == NULL) return 0; 
    double mysum = 0; 
    int col; 
    for (col = 0; col < col_num; ++col) {
      if (column[col] != NULL) mysum += column[col]->sum(); 
    }
    return mysum; 
  }
  double sum(int col) const {
    if (col < 0 || col >= col_num) {
      throw new AzException("AzSmat::sum(col)", "invalid col#"); 
    }
    if (column == NULL) return 0; 
    if (column[col] == NULL) return 0; 
    return column[col]->sum(); 
  }
  
  /*---  this never returns NULL  ---*/
  inline const AzSvect *col(int col_no) const {
    if (col_no < 0 || col_no >= col_num) {
      throw new AzException("AzSmat::col", "col# is out of range"); 
    }
    if (column[col_no] == NULL) {
      if (dummy_zero.rowNum() != row_num) {
        throw new AzException("AzSmat::col", "#col of dummy_zero is wrong"); 
      }
      return &dummy_zero; 
    }
    return column[col_no]; 
  }

  /*---  this never returns NULL  ---*/
  inline AzSvect *col_u(int col_no) {
    if (col_no < 0 || col_no >= col_num) {
      throw new AzException("AzSmat::col_u", "col# is out of range"); 
    }
    if (column[col_no] == NULL) {
      column[col_no] = new AzSvect(row_num); 
    }
    return column[col_no]; 
  }
  inline int rowNum() const { return row_num; }
  inline int colNum() const { return col_num; }
  inline int size() const { return row_num*col_num; }
  
  void log_of_plusone(); /* log(x+1) */
  void normalize(); 
  void normalize1(); 
  void binarize();  /* posi -> 1, nega -> -1 */
  void binarize1(); /* nonzero -> 1 */
  
  inline void destroy() {
    reform(0,0); 
  }
  inline void destroy(int col) {
    if (col >= 0 && col < col_num && column != NULL) {
      delete column[col]; 
      column[col] = NULL; 
    } 
  }

  inline void load(int col, AzIFarr *ifa_row_val) {
    col_u(col)->load(ifa_row_val); 
  }

  void clear(); 
  void zeroOut(); 

  int next(AzCursor &cursor, int col, double &out_val) const; 

  double max(int *out_row=NULL, int *out_col=NULL, 
                    bool ignoreZero=false) const;
  double min(int *out_row=NULL, int *out_col=NULL, 
                     bool ignoreZero=false) const; 

  void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, const AzStrArray *sp_col = NULL, 
            int cut_num = -1) const; 

  bool isSame(const AzSmat *inp) const; 

  static void concat(AzSmat *m_array[], 
                     int m_num, 
                     int col_num, 
                     /*---  output  ---*/
                     AzSmat *m_out,  /* not initialized */
                     bool destroyInput=false); 

  inline static bool isNull(const AzSmat *inp) {
    if (inp == NULL) return true; 
    if (inp->col_num == 0) return true; 
    if (inp->row_num == 0) return true; 
    return false; 
  }
  void cap(double cap_val); 

  void rbind(const AzSmat *m1); 
  void cbind(const AzSmat *m1); 

protected:
  void _read(AzFile *file); 
  void initialize(int row_num, int col_num, bool asDense); 
  void initialize(const AzSmat *inp); 
  void _transpose(AzSmat *m_out, int col_begin, int col_end) const; 
}; 
 
/*********************************************************************/
class AzSmatVar {
protected: 
  int data_num; 
  AzIntArr ia_dcolind; 
  AzSmat m; 
public: 
  AzSmatVar() : data_num(0) {}
  AzSmatVar(const char *fn) : data_num(0) {
    read(fn); 
  }
  int dataNum() const { return data_num; }
  int rowNum() const { return m.rowNum(); }
  int colNum() const { return m.colNum(); }
  int col_begin(int idx) const {
    check_idx(idx, "col_begin"); 
    return ia_dcolind.get(idx*2); 
  }
  int col_end(int idx) const {
    check_idx(idx, "col_end"); 
    return ia_dcolind.get(idx*2+1); 
  }
  void reset(const AzSmat *_m, const AzIntArr *_ia_dcolind) {
    m.set(_m); 
    ia_dcolind.reset(_ia_dcolind); 
    data_num = ia_dcolind.size()/2; 
    check_consistency(); 
  }
  void reset(const AzSmat *_m) { /* regard one column as one data point */
    AzIntArr ia; 
    int col; 
    for (col = 0; col < _m->colNum(); ++col) {
      ia.put(col); ia.put(col+1); 
    }
    reset(_m, &ia); 
  }    
  
  void destroy() {
    ia_dcolind.reset(); 
    m.reset(); 
    data_num = 0; 
  }
  void reset() { destroy(); }

  const AzSmat *data() const { return &m; }
  const AzIntArr *index() const { return &ia_dcolind; }
  
  void reset(const AzDataArr<AzSmat> *am, int num=-1) {
    data_num = (num > 0) ? num : am->size(); 
    ia_dcolind.reset(data_num*2, 0); 
    int *dcolind = ia_dcolind.point_u(); 
    int col = 0, row_num = 0; 
    int dx; 
    for (dx = 0; dx < data_num; ++dx) {
      const AzSmat *m_inp = am->point(dx); 
      dcolind[dx*2] = col; 
      col += m_inp->colNum(); 
      dcolind[dx*2+1] = col;
      if (dx == 0) row_num = m_inp->rowNum(); 
      else if (row_num != m_inp->rowNum()) {
        throw new AzException("AzSmatVar::reset(am,num)", "#row must be fixed"); 
      }
    }
    m.reform(row_num, col); 
    for (dx = 0; dx < data_num; ++dx) {
      const AzSmat *m_inp = am->point(dx); 
      int col0 = dcolind[dx*2], col1 = dcolind[dx*2+1]; 
      for (col = col0; col < col1; ++col) m.col_u(col)->set(m_inp->col(col-col0)); 
    }      
  }

  void set(const AzSmatVar *mv0, 
           const int *dxs0, /* mv0's data points */
           int dxs0_num);   /* size of dxs0 */  
  void set(const AzSmatVar *mv0, int dx0, int dx1) {
    AzIntArr ia; ia.range(dx0, dx1); 
    set(mv0, ia.point(), ia.size()); 
  }
  void set(const AzSmatVar *mv) {
    reset(mv->data(), mv->index()); 
  }
  
  void write(AzFile *file) {
    file->writeInt(data_num); 
    ia_dcolind.write(file); 
    m.write(file); 
  }
  void read(AzFile *file) {
    data_num = file->readInt(); 
    ia_dcolind.read(file); 
    m.read(file); 
  }
  void write(const char *fn) {
    AzFile::write(fn, this); 
  }
  void read(const char *fn) {
    AzFile::read(fn, this); 
  } 
  static void rbind_cbind(const AzSmatVar *mv0,
                          const AzSmatVar *mv1, 
                          AzSmatVar *m_out); 
  
protected: 
  inline void check_idx(int idx, const char *msg="") const {
    if (idx < 0 || idx >= data_num) {
      throw new AzException("AzSmatVar::check_idx", "invalid data index", msg); 
    }
  }
  void check_consistency() {
    const char *eyec = "AzSmatVar::check_consistency"; 
    if (ia_dcolind.size() != data_num*2) throw new AzException(eyec, "#data conflict"); 
    const int *dcolind = ia_dcolind.point(); 
    int dx; 
    for (dx = 0; dx < data_num; ++dx) {
      int col0 = dcolind[dx*2], col1 = dcolind[dx*2+1]; 
      if (col1-col0 < 0 || col0 < 0 || col1 > m.colNum()) {
        cout << "col0=" << col0 << " col1=" << col1 << " m.colNum()=" << m.colNum() << endl; 
        throw new AzException(eyec, "invalid column range"); 
      }
    }
    if (data_num > 0 && dcolind[data_num*2-1] != m.colNum()) throw new AzException(eyec, "size conflict"); 
  }
};
#endif 

