/* * * * *
 *  AzSvDataR.hpp 
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

#ifndef _AZ_SV_DATA_S_HPP_
#define _AZ_SV_DATA_S_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzDmat.hpp"
#include "AzStrPool.hpp"
#include "AzSvFeatInfo.hpp"

/* S for separation of features and targets */
class AzSvDataS : public virtual AzSvFeatInfo /* feature template */
{
protected: 
  AzSmat m_feat; 
  AzStrPool sp_f_dic; 
  AzDvect v_y; 
  int max_data_num; 
  double const_to_add; 
  
public: 
  AzSvDataS() : max_data_num(-1), const_to_add(0) {}
  inline const AzSmat *feat() const {
    checkIfReady("feat"); 
    return &m_feat; 
  }

  inline const AzDvect *targets() const {
    checkIfReady("targets"); 
    return &v_y;  
  }
  inline const AzSvFeatInfo *featInfo() const { return this; }
  inline int featNum() const { return m_feat.rowNum(); }

  inline int size() const { return m_feat.colNum(); }

  void concatDesc(int ex, AzBytArr *str_desc) const; 

  inline 
  virtual void read(const char *feat_fn, 
                    const char *y_fn=NULL, 
                    const char *fdic_fn=NULL,
                    int max_data_num=-1) {
    if (y_fn == NULL || strlen(y_fn) == 0) read_features_only(feat_fn, fdic_fn, max_data_num); 
    else                                  _read(feat_fn, y_fn, fdic_fn, max_data_num); 
  }
  virtual void read_features_only(const char *feat_fn, 
                                  const char *fdic_fn=NULL,
                                  int max_data_num=-1); 
  virtual void read_targets_only(const char *y_fn, int max_data_num); 
  void destroy(); 
  
  void append_const(double const_to_add) {
    int r_num = m_feat.rowNum()+1, c_num = m_feat.colNum(); 
    m_feat.resize(r_num, c_num); 
    int col; 
    for (col = 0; col < c_num; ++col) {
      AzIFarr ifa; 
      m_feat.col(col)->nonZero(&ifa); 
      ifa.put(r_num-1, const_to_add); 
      m_feat.col_u(col)->load(&ifa); 
    }
  }
  
  /*-----------------------------------------------*/
  void normalize_feat() {
    m_feat.normalize();  /* to unit vectors */
  }
  void apply_log() {
    apply_log(&m_feat); 
  }
  void find_max(AzDvect *v_max) {
    find_max(&m_feat, v_max);     
  }
  void find_max(double ratio, int topk, AzDvect *v_max) {
    find_max(&m_feat, ratio, topk, v_max); 
  }
  void divide_by(const AzDvect *v_max) {
    divide_by(v_max, &m_feat); 
  }
  void cap(double capval) {
    cap(capval, &m_feat); 
  }
  void get_abs_sdev(AzDvect *v_sdev) const {
    get_abs_sdev(&m_feat, v_sdev); 
  }
  /*-----------------------------------------------*/
  
  /*---  static tools  ---*/
  inline static void readMatrix(const char *fn, AzDmat *m_data, int max_data_num=-1) {
    AzSmat m; 
    readMatrix(fn, &m, max_data_num); 
    m_data->set(&m); 
  }
  static void readMatrix(const char *fn, 
                         AzSmat *m_data, 
                         int max_data_num=-1) {
    readData_Large(fn, -1, m_data, max_data_num); 
  }

  static void readVector(const char *fn, 
                         /*---  output  ---*/
                         AzDvect *v_data) {
    read_target(fn, v_data); 
  }
  static void mergeData(int argc, 
                         const char *argv[]); 
  static void mergeData(const char *x_fn, 
                         const char *x_names_fn, 
                         const char *fn_template, 
                         const char *str, 
                         bool doSparse, 
                         int digits, 
                         const char *out_x_fn, 
                         const char *out_n_fn, 
                         int num,
                         const char *names[]); 

  static void mergeData(const AzSmat *m_x, 
                         const AzSvFeatInfo *feat, 
                         const char *fn_template, 
                         const char *str, 
                         bool doSparse, 
                         int digits, 
                         const char *out_x_fn, 
                         const char *out_n_fn,
                         int num,
                         const char *names[]); 

  /*------------------------------------------*/ 
  static void find_max(const AzSmat *m_x, AzDvect *v_max) {
    v_max->reform(m_x->rowNum()); 
    double *max = v_max->point_u(); 
    int col, row; 
    for (col = 0; col < m_x->colNum(); ++col) {
      AzDvect v_x(m_x->col(col)); 
      const double *xval = v_x.point(); 
      for (row = 0; row < m_x->rowNum(); ++row) {
        max[row] = MAX(max[row], fabs(xval[row])); 
      }
    }
  }

  /*------------------------------------------*/ 
  static void find_max(const AzSmat *m_x, double ratio, int topk, AzDvect *v_max) {
    AzSmat m_tran; 
    m_x->transpose(&m_tran); 
    int kk = MAX(topk, (int)((double)m_x->colNum()*ratio)); 
    v_max->reform(m_x->rowNum()); 
    double *max = v_max->point_u(); 
    int row; 
    for (row = 0; row < m_x->rowNum(); ++row) {
      AzDvect v(m_tran.col(row)); 
      v.abs(); 
      AzIFarr ifa; 
      v.nonZero(&ifa); 
      ifa.sort_Float(false); /* descending order */
      ifa.cut(kk); 
      max[row] = ifa.sum() / (double)kk; 
    }
  }

  /*------------------------------------------*/ 
  static void divide_by(const AzDvect *v_max, AzSmat *m_x) {
    int col; 
    for (col = 0; col < m_x->colNum(); ++col) {
      AzDvect v_x(m_x->col(col)); 
      bool doInverse = true; 
      v_x.scale(v_max, doInverse); 
      m_x->col_u(col)->set(&v_x); 
    } 
  }

  /*------------------------------------------*/ 
  static void apply_log(AzSmat *m_x) {
    int col; 
    for (col = 0; col < m_x->colNum(); ++col) {
      AzIFarr ifa; 
      const AzSvect *v_x = m_x->col(col); 
      AzCursor cur; 
      for ( ; ; ) {
        double val; 
        int row = v_x->next(cur, val); 
        if (row < 0) break; 
        if (val > 0) val = log(val+1); 
        else         val = -log(-val+1); 
        ifa.put(row, val); 
      }
      m_x->col_u(col)->load(&ifa); 
    }
  }

  /*------------------------------------------*/ 
  static void cap(double capval, AzSmat *m_x) {
    if (capval < 0) throw new AzException("AzSvDataS::cap", "cap must be non-negative"); 
    int col; 
    for (col = 0; col < m_x->colNum(); ++col) {
      AzIFarr ifa; 
      const AzSvect *v_x = m_x->col(col); 
      AzCursor cur; 
      for ( ; ; ) {
        double val; 
        int row = v_x->next(cur, val); 
        if (row < 0) break; 
        if (val > 0) val = MIN(val, capval); 
        else         val = MAX(val, -capval); 
        ifa.put(row, val); 
      }
      m_x->col_u(col)->load(&ifa); 
    }  
  }
  static void sdev(const AzSmat *m, AzDvect *v_sdev) {
    AzDvect v_avg(m->rowNum()), v_avg2(m->rowNum()); 
    int cx;
    for (cx = 0; cx < m->colNum(); ++cx) {
      v_avg.add(m->col(cx)); 
      AzDvect v(m->col(cx)); v.square(); 
      v_avg2.add(&v); 
    }
    v_avg.divide(m->colNum()); 
    v_avg2.divide(m->colNum()); 
    v_sdev->reform(m->rowNum()); 
    AzDvect::sdev(&v_avg, &v_avg2, v_sdev); 
  }
  static void get_abs_sdev(const AzSmat *m, AzDvect *v_sdev) {
    AzDvect v_avg(m->rowNum()), v_avg2(m->rowNum()); 
    int cx;
    for (cx = 0; cx < m->colNum(); ++cx) {
      AzDvect v(m->col(cx)); v.abs(); 
      v_avg.add(&v); 
      v.square(); 
      v_avg2.add(&v); 
    }
    v_avg.divide(m->colNum()); 
    v_avg2.divide(m->colNum()); 
    v_sdev->reform(m->rowNum()); 
    AzDvect::sdev(&v_avg, &v_avg2, v_sdev); 
  }  
  
protected:
  virtual void _read(const char *feat_fn, 
                    const char *y_fn, 
                    const char *fdic_fn=NULL,
                    int max_data_num=-1); 
  void checkIfReady(const char *msg) const; 
  void reset(); 

  static void read_feat(const char *feat_fn, 
                          const char *fdic_fn, 
                          /*---  output  ---*/
                          AzSmat *m_feat, 
                          AzStrPool *sp_f_dic, 
                          int max_data_num=-1); 
  static void read_target(const char *y_fn, 
                          AzDvect *v_y,
                          int max_data_num=-1); 

  static void readData(const char *data_fn, 
                         int expected_f_num, 
                         /*---  output  ---*/
                         AzSmat *m_data,
                         int max_data_num=-1) {                      
    readData_Large(data_fn, expected_f_num, m_data, max_data_num);   /* 12/16/2012 */
  }
#if 0   
  static void readData_Small(const char *data_fn, 
                         int expected_f_num, 
                         /*---  output  ---*/
                         AzSmat *m_data); 
#endif                          
  static void readData_Large(const char *data_fn, 
                         int expected_f_num, 
                         /*---  output  ---*/
                         AzSmat *m_feat, 
                         /*---  ---*/
                         int max_data_num=-1); 

  inline static void parseDataLine(const AzByte *inp, 
                              int inp_len, 
                              int f_num, 
                              const char *data_fn, 
                              int line_no, 
                              /*---  output  ---*/
                              AzSmat *m_feat, 
                              int col) {
    AzIFarr ifa_ex_val; 
    _parseDataLine(inp, inp_len, f_num, data_fn, line_no, ifa_ex_val);   
    m_feat->load(col, &ifa_ex_val);  
  }                           
  static void _parseDataLine(const AzByte *inp, 
                            int inp_len, 
                            int f_num, 
                            const char *data_fn, /* for printing error */
                            int line_no, 
                            /*---  output  ---*/
                            AzIFarr &ifa_ex_val); 

  static int countFeatures(const AzByte *line, 
                           const AzByte *line_end);

  static void scanData(const char *data_fn, 
                         /*---  output  ---*/
                         int &out_data_num, 
                         int &out_max_len); 

  /*---  For the sparse data format  ---*/
  inline static void parseDataLine_Sparse(const AzByte *inp, 
                              int inp_len, 
                              int f_num, 
                              const char *data_fn, 
                              int line_no, 
                              /*---  output  ---*/
                              AzSmat *m_feat, 
                              int col) {
    AzIFarr ifa_ex_val; 
    _parseDataLine_Sparse(inp, inp_len, f_num, data_fn, line_no, ifa_ex_val); 
    m_feat->load(col, &ifa_ex_val);
  }   
  static void _parseDataLine_Sparse(const AzByte *inp, 
                              int inp_len, 
                              int f_num, 
                              const char *data_fn, 
                              int line_no, 
                              /*---  output  ---*/
                              AzIFarr &ifa_ex_val); 

  static void decomposeFeat(const char *token, 
                            int line_no, 
                            /*---  output  ---*/
                            int *ex, 
                            double *val); 
  static int if_sparse(AzBytArr &s_line, int expected_f_num, const char *str="sparse"); 
  inline static double my_atof(const char *str, 
                           const char *eyec, 
                           int line_no) {
    if (*str == '\0' || *str >= '0' && *str <= '9' || 
        *str == '+' || *str == '-') {
      return atof(str); 
    }
    AzBytArr s("Invalid number expression in line# ");
    s.cn(line_no); s.c(" of the input data file: "); s.c(str); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }

  inline static int my_fno(const char *str, 
                           const char *eyec, 
                           int line_no) {
    if (*str >= '0' && *str <= '9' || 
        *str == '+') {
      return atol(str); 
    }
    AzBytArr s("Invalid field# expression in line# ");
    s.cn(line_no); s.c(" of the input data file: "); s.c(str); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }
}; 

#endif 
