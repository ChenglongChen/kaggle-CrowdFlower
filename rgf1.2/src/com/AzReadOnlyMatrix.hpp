/* * * * *
 *  AzReadOnlyMatrix.hpp 
 *  Copyright (C) 2011, 2012 Rie Johnson
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

#ifndef _AZ_READONLY_MATRIX_HPP_
#define _AZ_READONLY_MATRIX_HPP_

#include "AzUtil.hpp"
#include "AzStrArray.hpp"

//! Abstract class: interface for read-only vectors.  
class AzReadOnlyVector {
public:
  virtual int rowNum() const = 0; 
  virtual double get(int row_no) const = 0; 
  virtual int next(AzCursor &cursor, double &out_val) const = 0; 
  virtual bool isZero() const = 0; 
  virtual void dump(const AzOut &out, const char *header, 
                    const AzStrArray *sp_row = NULL, 
                    int cut_num = -1) const = 0; 
  virtual double selfInnerProduct() const = 0; 
  virtual int nonZeroRowNum() const = 0; 
  virtual void nonZero(AzIFarr *ifa) const = 0; 
  virtual double sum() const = 0; 

  /*--------------------------------------*/
  virtual void writeText(const char *fn, int digits) const {
    AzIntArr ia; 
    ia.range(0, rowNum()); 
    writeText(fn, &ia, digits);  
  }

  /*--------------------------------------*/
  virtual void writeText(const char *fn, const AzIntArr *ia, int digits) const {
    AzFile file(fn); 
    file.open("wb"); 
    AzBytArr s;
    int ix; 
    for (ix = 0; ix < ia->size(); ++ix) {
      int row = ia->get(ix);  
      double val = get(row); 
      s.cn(val, digits); 
      s.nl(); 
    }
    s.writeText(&file); 
    file.close(true); 
  }

  /*--------------------------------------*/
  virtual void to_sparse(AzBytArr *s, int digits) const {
    AzCursor cur; 
    for ( ; ; ) {
      double val; 
      int row = next(cur, val); 
      if (row < 0) break; 
      if (s->length() > 0) {
        s->c(' '); 
      }
      s->cn(row); 
      if (val != 1) {
        s->c(':'); s->cn(val, digits); 
      }
    }    
  }

  /*--------------------------------------*/
  virtual void to_dense(AzBytArr *s, int digits) const {
    int row; 
    for (row = 0; row < rowNum(); ++row) {
      double val = get(row); 
      if (row > 0) {
        s->c(" "); 
      }
      s->cn(val, digits); 
    }
  }
}; 

//! Abstract class: interface for read-only matrices.  
class AzReadOnlyMatrix {
public: 
  virtual int rowNum() const = 0; 
  virtual int colNum() const = 0; 

  virtual void destroy() = 0; 
#if 0 
  virtual void destroy(int col) = 0; 
#endif 

  virtual double get(int row_no, int col_no) const = 0; 

  virtual const AzReadOnlyVector *col(int col_no) const = 0; 

  virtual bool isZero() const = 0; 
  virtual bool isZero(int col) const = 0; 

  virtual void dump(const AzOut &out, const char *header, 
                    const AzStrArray *sp_row = NULL, const AzStrArray *sp_col = NULL, 
                    int cut_num = -1) const = 0; 

  /*--------------------------------------*/
  virtual void writeText(const char *fn, int digits, 
                         bool doSparse=false,
                         bool doAppend=false) const {
    AzIntArr ia; 
    ia.range(0, colNum()); 
    writeText(fn, &ia, digits, doSparse, doAppend); 
  }

  /*--------------------------------------*/
  virtual void writeText(const char *fn, const AzIntArr *ia, 
                         int digits, 
                         bool doSparse=false, 
                         bool doAppend=false) const {
    if (doSparse) {
      if (doAppend) throw new AzException("AzReadOnlyMatrix::writeText", "sparse matrices cannot be appended"); 
      writeText_sparse(fn, ia, digits); 
      return; 
    }
    AzFile file(fn); 
    if (doAppend) file.open("ab"); 
    else          file.open("wb");
    int num; 
    const int *cxs = ia->point(&num);  
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int cx = cxs[ix];  
      AzBytArr s; 
      col(cx)->to_dense(&s, digits); 
      s.nl(); 
      s.writeText(&file); 
    }
    file.close(true); 
  }

  /*--------------------------------------*/
  virtual void writeText_sparse(const char *fn, const AzIntArr *ia, int digits) const {
    AzFile file(fn); 
    file.open("wb"); 
    AzBytArr s_header("sparse "); s_header.cn(rowNum()); s_header.nl(); 
    s_header.writeText(&file);

    int num; 
    const int *cxs = ia->point(&num); 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int cx = cxs[ix];  
      AzBytArr s; 
      col(cx)->to_sparse(&s, digits); 
      s.nl(); 
      s.writeText(&file); 
    }
    file.close(true); 
  }
}; 
#endif 
