/* * * * *
 *  AzBmat.hpp 
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

#ifndef _AZ_BMAT_HPP_
#define _AZ_BMAT_HPP_
#include "AzUtil.hpp"

//! binary matrix 
class AzBmat {
protected:
  int row_num; 
  AzDataArray<AzIntArr> a; 

public: 
  AzBmat() : row_num(0) {}
  AzBmat(int inp_row_num, int inp_col_num) : row_num(0) {
    resize(inp_col_num); 
  }
  AzBmat(const AzBmat *inp) : row_num(0) {
    set(inp);   
  }
  inline void set(const AzBmat *inp) {
    row_num = inp->row_num; 
    a.reset(&inp->a); 
  }
  AzBmat(const AzBmat &inp) : row_num(0) {
    set(&inp); 
  }
  AzBmat & operator =(const AzBmat &inp) {
    if (this == &inp) return *this; 
    set(&inp);  
    return *this; 
  }
  inline void reform(int inp_row_num, int inp_col_num) {
    reset(); 
    row_num = inp_row_num;  
    resize(inp_col_num); 
  }
  inline void resize(int new_col_num) {
    a.resz(new_col_num); 
  }

  inline void reset() {
    row_num = 0; 
    a.reset(); 
  }
  inline int rowNum() const {
    return row_num; 
  }
  inline int colNum() const {
    return a.cursor(); 
  }

  inline const AzIntArr *on_rows(int col) const {
    return a.point(col); 
  }
  inline void clear(int fx) {
    a.point_u(fx)->reset(); 
  }

  inline void load(int col, const AzIntArr *ia_on_rows) {
    if (ia_on_rows == NULL || ia_on_rows->size() <= 0) return; 

    if (ia_on_rows->min() < 0 || 
        ia_on_rows->max() >= row_num) {
      throw new AzException("AzBmat::load", "wrong row#"); 
    }
    a.point_u(col)->reset(ia_on_rows); 
  }
}; 
#endif 


