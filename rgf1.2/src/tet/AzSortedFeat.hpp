/* * * * *
 *  AzSortedFeat.hpp 
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

#ifndef _AZ_SORTED_FEAT_HPP_
#define _AZ_SORTED_FEAT_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzDmat.hpp"


class AzSortedFeat
{
public:
  virtual int dataNum() const = 0; 
  virtual void rewind(AzCursor &cur) const = 0; 
  virtual const int *next(AzCursor &cur, double *out_val, int *out_num) const = 0; 
  virtual bool isForward() const = 0; 
  virtual void getIndexes(const int *inp_dxs, int inp_dxs_num, 
                              double border_val, 
                              /*---  output  ---*/
                              AzIntArr *ia_le_dx, 
                              AzIntArr *ia_gt_dx) const = 0; 
}; 

class AzSortedFeat_Dense : public virtual AzSortedFeat
{
protected:
  AzIntArr ia_index; 
  const int *index; 
  int index_num; 
  int offset; 
  const AzDvect *v_dx2v; 
  bool isOriginal; 

public:
  AzSortedFeat_Dense() : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {}
  AzSortedFeat_Dense(const AzDvect *v_data_transpose, 
                     const AzIntArr *ia_dx) 
                       : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {
    reset(v_data_transpose, ia_dx); 
  }
  AzSortedFeat_Dense(const AzSortedFeat_Dense *inp,  /* must not be NULL */
               const AzIntArr *ia_isYes,    
               int yes_num)
                       : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {
    filter(inp, ia_isYes, yes_num); 
  }
  AzSortedFeat_Dense(const AzSortedFeat_Dense *inp)
                       : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {
    copy_base(inp); 
  }

  void reset(const AzDvect *v_data_transpose, const AzIntArr *ia_dx); 
  void filter(const AzSortedFeat_Dense *inp,
              const AzIntArr *ia_isYes,
              int yes_num); 

  inline int dataNum() const {
    return index_num; 
  }

  inline int *base_index_for_update(int *len) {
    if (isOriginal) {
      throw new AzException("AzSortedFeat_Dense::base_index_for_update", 
                            "Not allowed"); 
    }
    return ia_index.point_u(len); 
  }

  inline void rewind(AzCursor &cur) const {
    cur.set(0); 
  }
  const int *next(AzCursor &cur, double *out_val, int *out_num) const; 

  inline bool isForward() const {
    return true; 
  }

  AzSortedFeat_Dense & operator =(const AzSortedFeat_Dense &inp) { /* never tested */
    if (this == &inp) return *this; 
    ia_index.reset(&inp.ia_index); 
    v_dx2v = inp.v_dx2v; 
    return *this; 
  }

  void getIndexes(const int *inp_dxs, int inp_dxs_num,  
                              double border_val, 
                              /*---  output  ---*/
                              AzIntArr *ia_le_dx, 
                              AzIntArr *ia_gt_dx) const; 

  static void separate(AzSortedFeat_Dense *base, /* indexes will be swaped */
                       const AzSortedFeat_Dense *inp, 
                       const AzIntArr *isYes, 
                       int yes_num, 
                       AzSortedFeat_Dense *yes, 
                       AzSortedFeat_Dense *no); 

protected:
  void copy_base(const AzSortedFeat_Dense *inp); 

  static void separate_indexes(int *index, 
                           int index_num, 
                           const int *isYes, 
                           int yes_num, 
                           int max_dx); 
}; 


//! Data points sorted by feature values.  
class AzSortedFeat_Sparse : public virtual AzSortedFeat
{
protected:
  AzIntArr ia_zero; /* may not be set if unnecessary */
  AzIntArr ia_index; 
  AzDvect v_value; 
  bool _shouldDoBackward; 
  int data_num; 

public:
  AzSortedFeat_Sparse() : _shouldDoBackward(false), data_num(0) {}
  AzSortedFeat_Sparse(const AzSvect *v_data_transpose, 
               const AzIntArr *ia_dx) /* must be sorted */ 
                        : _shouldDoBackward(false), data_num(0) {
    reset(v_data_transpose, ia_dx); 
  }
  AzSortedFeat_Sparse(const AzSortedFeat_Sparse *inp,  /* must not be NULL */
               const AzIntArr *ia_isYes,    
               int yes_num) 
                        : _shouldDoBackward(false), data_num(0) {
    filter(inp, ia_isYes, yes_num); 
  }
  AzSortedFeat_Sparse(const AzSortedFeat_Sparse *inp) 
                        : _shouldDoBackward(false), data_num(0) {
    copy(inp); 
  }


  inline int dataNum() const {
    return data_num; 
  }

  void reset(const AzSvect *v_data_transpose, const AzIntArr *ia_dx); 
  void filter(const AzSortedFeat_Sparse *inp,  /* may be NULL */
              const AzIntArr *ia_isYes, 
              int yes_num); 

  inline void rewind(AzCursor &cur) const {
    if (_shouldDoBackward) {
      cur.set(ia_index.size()); 
    }
    else {
      cur.set(0); 
    }
  }
  inline const int *next(AzCursor &cur, double *out_val, int *out_num) const {
    if (_shouldDoBackward) {
      return backward(cur, out_val, out_num); 
    }
    else {
      return forward(cur, out_val, out_num); 
    }
  }

  inline bool isForward() const {
    return !_shouldDoBackward; 
  }

  AzSortedFeat_Sparse & operator =(const AzSortedFeat_Sparse &inp) { /* never tested */
    if (this == &inp) return *this; 
    ia_zero.reset(&inp.ia_zero); 
    ia_index.reset(&inp.ia_index); 
    v_value.set(&inp.v_value); 
    _shouldDoBackward = inp._shouldDoBackward; 
    data_num = inp.data_num; 
    return *this; 
  }

  void getIndexes(const int *inp_dxs, int inp_dxs_num, 
                              double border_val, 
                              /*---  output  ---*/
                              AzIntArr *ia_le_dx, 
                              AzIntArr *ia_gt_dx) const; 

  static void separate(const AzSortedFeat_Sparse *inp, 
                          const AzIntArr *isYes, 
                          int yes_num, 
                          AzSortedFeat_Sparse *yes, 
                          AzSortedFeat_Sparse *no); 

protected:
  void copy(const AzSortedFeat_Sparse *inp); 
  static void sub_initialize(const AzSortedFeat_Sparse *inp, 
                                 int num, 
                                 AzSortedFeat_Sparse *ptr); 
  static void sub_terminate(AzSortedFeat_Sparse *ptr, 
                            int num, 
                            int where_is_zero); 
  const AzIntArr *getIndexes_Zero(const int *inp_dxs, int inp_dxs_num,  
                                  AzIntArr *out_ia_zero) const; 

  const int *forward(AzCursor &cur, double *out_val, int *out_num) const; 
  const int *backward(AzCursor &cur, double *out_val, int *out_num) const; 
}; 


class AzSortedFeatWork {
public:
  AzSortedFeat_Sparse tmps; 
  AzSortedFeat_Dense tmpd; 
}; 

class AzSortedFeatArr { 
protected:
  AzSortedFeat_Sparse **arrs; 
  AzSortedFeat_Dense **arrd; 
  AzObjPtrArray<AzSortedFeat_Sparse> a_sparse; 
  AzObjPtrArray<AzSortedFeat_Dense> a_dense; 
  int f_num; 
  bool beTight; 

  /*---  used only when beTight = true  ---*/
  AzIntArr ia_isActive; 
  int active_num; 

public: 
  AzSortedFeatArr() : arrs(NULL), arrd(NULL), f_num(0), beTight(false) {}
  AzSortedFeatArr(const AzSortedFeatArr *inp)
                    : arrs(NULL), arrd(NULL), f_num(0), beTight(false) {
    copy_base(inp); 
  }
  AzSortedFeatArr(const AzSortedFeatArr *inp, const int *dxs, int dxs_num) 
                    : arrs(NULL), arrd(NULL), f_num(0), beTight(false) {
    filter_base(inp, dxs, dxs_num); 
  }
  void reset_sparse(const AzSmat *m_tran, 
                    bool beTight=false); 
  void reset_dense(const AzDmat *m_tran_dense, 
                   bool inp_beTight=false); 

  inline bool doingSparse() const {
    if (arrs != NULL) return true; 
    else              return false; 
  }
  inline int featNum() const {
    return f_num; 
  }
  inline const AzSortedFeat *sorted(int fx) const {
    if (fx < 0 || fx >= f_num) {
      throw new AzException("AzSortedFeatArr::sorted", "out of range"); 
    }
    if (arrd != NULL) {
      return arrd[fx]; 
    }
    if (arrs != NULL) {
      return arrs[fx]; 
    }
    return NULL; 
  }
  const AzSortedFeat *sorted(const AzSortedFeatArr *inp, 
              int fx, 
              AzSortedFeatWork *out) const; 

  void reset() {
    a_dense.free(&arrd); 
    a_sparse.free(&arrs); 
    f_num = 0; 
    ia_isActive.reset(); 
    active_num = 0;   
  }

  static void separate(AzSortedFeatArr *base, 
             const AzSortedFeatArr *inp, 
             const int *yes_dxs, int yes_dxs_num, 
             const int *no_dxs, int no_dxs_num, 
             AzSortedFeatArr *yes, AzSortedFeatArr *no); 

  void copy_base(const AzSortedFeatArr *inp); 
  void filter_base(const AzSortedFeatArr *inp, const int *dxs, int dxs_num); 

protected:
  static void sub_initialize(const AzSortedFeatArr *inp, 
                      AzSortedFeatArr *ptr); 
}; 

#endif 

