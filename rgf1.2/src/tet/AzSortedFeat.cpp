/* * * * *
 *  AzSortedFeat.cpp 
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

#include "AzSortedFeat.hpp"
#include "AzTools.hpp"
#include "AzPrint.hpp"

/*------------------------------------------------------*/
/*------------------------------------------------------*/
void AzSortedFeat_Dense::reset(const AzDvect *v_data_transpose, 
                                   const AzIntArr *ia_dx) 
{
  v_dx2v = v_data_transpose; 
  const double *dx2value = v_dx2v->point(); 

  const int *dxs = ia_dx->point(); 
  AzIFarr ifa_dx_val; 
  ifa_dx_val.prepare(ia_dx->size()); 
  int ix; 
  for (ix = 0; ix < ia_dx->size(); ++ix) {
    int dx = dxs[ix]; 
    ifa_dx_val.put(dx, dx2value[dx]); 
  }
  ifa_dx_val.sort_FloatInt(true); /* ascending order */

  ia_index.reset(); 
  ia_index.prepare(ifa_dx_val.size()); 
  for (ix = 0; ix < ifa_dx_val.size(); ++ix) {
    int dx; 
    ifa_dx_val.get(ix, &dx); 
    ia_index.put(dx); 
  }

  index = ia_index.point(&index_num); 
  offset = 0; 
  isOriginal = true; /* This is the original one.  Don't change. */
}

/*------------------------------------------------------*/
void AzSortedFeat_Dense::filter(const AzSortedFeat_Dense *inp, 
                          const AzIntArr *ia_isYes, 
                          int yes_num)
{
  ia_index.prepare(yes_num); 
  v_dx2v = inp->v_dx2v; 

  int max_dx = ia_isYes->size() - 1; 
  const int *isYes = ia_isYes->point(); 

  int inp_index_num; 
  const int *inp_index = inp->ia_index.point(&inp_index_num); 

  int ix; 
  for (ix = 0; ix < inp_index_num; ++ix) {
    int dx = inp_index[ix]; 
    if (dx <= max_dx && isYes[dx]) {
      ia_index.put(dx); 
    }
  }

#if 0 
  offset = -1; /* not pointing to the original indexes */
#else
  offset = 0; /* 04/06/2012 */
#endif 
  index = ia_index.point(&index_num); 
}

/*------------------------------------------------------*/
/* place indexes so that yes's first and no's last and  */
/* the order with yes's and no's does not change.       */
/*------------------------------------------------------*/
/* static */
void AzSortedFeat_Dense::separate_indexes(int *index, 
                           int index_num, 
                           const int *isYes, 
                           int yes_num, 
                           int max_dx)
{
  AzIntArr ia_no;  
  ia_no.prepare(index_num-yes_num); 
  int yes_ix = 0; 
  int ix; 
  for (ix = 0; ix < index_num; ++ix) {
    int dx = index[ix]; 
    if (dx <= max_dx && isYes[dx]) {
      if (yes_ix != ix) {
        index[yes_ix] = dx; 
      }
      ++yes_ix; 
    }
    else {
      ia_no.put(dx); 
    }
  }

  if (ia_no.size() > 0) {
    memcpy(index+yes_ix, ia_no.point(), sizeof(int)*ia_no.size()); 
  }
  if (yes_ix != yes_num) {
    throw new AzException("AzSortedFeat_Dense::separate_indexes", 
                          "conflict in # of yes's"); 
  }
}

/*------------------------------------------------------*/
void AzSortedFeat_Dense::separate(
                          AzSortedFeat_Dense *base,  
                          const AzSortedFeat_Dense *inp, 
                          const AzIntArr *ia_isYes, 
                          int yes_num, 
                          AzSortedFeat_Dense *yes, 
                          AzSortedFeat_Dense *no)
{
  const char *eyec = "AzSortedFeat_Dense::separate"; 

  yes->v_dx2v = inp->v_dx2v; 
  no->v_dx2v = inp->v_dx2v; 

  int max_dx = ia_isYes->size() - 1; 
  const int *isYes = ia_isYes->point(); 

  int base_index_num; 
  int *base_index = base->base_index_for_update(&base_index_num); 

  int *sub_index = base_index + inp->offset; 

  if (inp->index != sub_index || 
      inp->offset+inp->index_num > base_index_num) {
    throw new AzException(eyec, "index conflict"); 
  }

  separate_indexes(sub_index, inp->index_num, 
                   isYes, yes_num, max_dx); 

  yes->index = sub_index; 
  yes->index_num = yes_num; 
  yes->offset = inp->offset; 
  no->index = sub_index + yes_num; 
  no->index_num = inp->index_num - yes_num; 
  no->offset = inp->offset + yes_num; 

  if (base_index+yes->offset != yes->index || 
      base_index+no->offset != no->index) {
    throw new AzException(eyec, "conflict in pointers"); 
  }
}

/*------------------------------------------------------*/
void AzSortedFeat_Dense::copy_base(const AzSortedFeat_Dense *inp)
{
  if (inp->ia_index.size() <= 0 || 
      inp->index != inp->ia_index.point() || 
      inp->index_num != inp->ia_index.size() || 
      inp->offset != 0) {
    throw new AzException("AzSortedFeat_Dense::copy_base", 
                          "Expected the base as input"); 
  }

  ia_index.reset(&inp->ia_index); 
  v_dx2v = inp->v_dx2v; 
  index = ia_index.point(&index_num); 
  offset = 0; 

  isOriginal = false; /* This is a copy. */
}

/* called when and only when data points are sampled */
/*--------------------------------------------------------*/
void AzSortedFeatArr::filter_base(const AzSortedFeatArr *inp, 
                                  const int *dxs, 
                                  int dxs_num) 
{
  const char *eyec = "AzSortedFeatArr::filter_base"; 
  beTight = inp->beTight; 
  f_num = inp->featNum(); 
  a_sparse.free(&arrs); 
  a_dense.free(&arrd); 

  ia_isActive.reset(); 
  ia_isActive.toOnOff(dxs, dxs_num); 
  active_num = dxs_num; 

  if (beTight) {
    return; 
  }

  if (inp->doingSparse()) {
    a_sparse.alloc(&arrs, f_num, eyec, "arrs"); 
    int fx; 
    for (fx = 0; fx < f_num; ++fx) {
      if (inp->arrs == NULL || inp->arrs[fx] == NULL) {
        throw new AzException(eyec, "No sorted sparse features?!");
      }
      arrs[fx] = new AzSortedFeat_Sparse(inp->arrs[fx], &ia_isActive, active_num); 
    }
  }
  else {
    a_dense.alloc(&arrd, f_num, eyec, "arrd"); 
    int fx; 
    for (fx = 0; fx < f_num; ++fx) {
      if (inp->arrd == NULL || inp->arrd[fx] == NULL) {
        throw new AzException(eyec, "No sorted dense features?!");
      }
      arrd[fx] = new AzSortedFeat_Dense(inp->arrd[fx], &ia_isActive, active_num); 
    }
  }
}

/*--------------------------------------------------------*/
const int *AzSortedFeat_Dense::next(AzCursor &cur, 
                              double *out_val, int *out_num) /* output */
const 
{
  const char *eyec = "AzSortedFeat_Dense::next"; 

  int cursor = cur.get(); 
  if (cursor >= index_num) {
    return NULL;  /* end of data */
  }

  const double *dx2value = v_dx2v->point(); 

  int dx = index[cursor]; 
  double curr_val = dx2value[dx]; 

  int begin = cursor; 
  cursor = cur.inc(); 
  if (cursor >= index_num) {
    return  NULL; /* this will produce all vs none anyway */
  }

  double avg_val = curr_val + 0.00000001; 
  for ( ; cursor < index_num; cursor=cur.inc()) {
    int dx = index[cursor]; 
    double next_val = dx2value[dx]; 
    if (next_val != curr_val) {
      avg_val = (curr_val + next_val) / 2; 
      break; 
    }
  }
  if (cursor >= index_num) {
    return NULL; /* this will produce all vs none anyway */
  }

  *out_val = avg_val; 
  *out_num = cursor - begin; 
  return index + begin;   
}

/*------------------------------------------------------*/
void AzSortedFeat_Dense::getIndexes(const int *inp_dxs, /* not used */
                              int inp_dxs_num, 
                              double border_val, 
                              /*---  output  ---*/
                              AzIntArr *ia_le_dx, 
                              AzIntArr *ia_gt_dx)
const
{
  if (inp_dxs_num != index_num) {
    throw new AzException("AzSortedFeat_Dense::getIndexes", 
                          "Conflict in # of data points"); 
  }

  const double *dx2value = v_dx2v->point(); 
  int ix; 
  for (ix = 0; ix < index_num; ++ix) {
    int dx = index[ix]; 
    if (dx2value[dx] > border_val) break; 
  }
  int le_size = ix; 
  ia_le_dx->reset(index, le_size); 
  ia_gt_dx->reset(index+le_size, index_num-le_size); 
}


/*-------------------------------------------------------*/
/*-------------------------------------------------------*/
/* 
 * For sparse data with many zeroes                      
 *   Avoid going through /carrying around data indexes for  
 *   zero entries as much as possible.                     
 * (NOTE) The order of addition of floating-point values becomes 
 *        different from AzSortedFeat_Dense, which may cause slight 
 *        differences in the results.  
 */
void AzSortedFeat_Sparse::reset(const AzSvect *v_data_transpose, 
                         const AzIntArr *ia_dx) /* must be sorted */
{
  data_num = ia_dx->size(); 

  AzIFarr ifa_dx_val; 
  v_data_transpose->filter(ia_dx, /* must be sorted */
                           &ifa_dx_val, NULL); 

  int zero_num = ia_dx->size() - ifa_dx_val.size(); 
  ia_zero.reset(); 

  double min_val = ifa_dx_val.findMin(); 
  double max_val = ifa_dx_val.findMax(); 

  if (min_val == max_val) { /* one value, no need to sort */
    if (zero_num > 0) { /* some values are zero */
      if (min_val > 0) {
        ifa_dx_val.insert(0, AzNone, (double)0); /* zero is smaller */
      }
      else {
        ifa_dx_val.put(AzNone, (double)0); /* zero is bigger */
      }     
    }
  }
  else {
    if (zero_num > 0) {
      ifa_dx_val.put(AzNone, (double)0); /* place holder for zero */
    }
    ifa_dx_val.sort_FloatInt(true); /* ascending order */
  }

  /*---  make it flat for faster access later on  ---*/
  AzTools::flatten(&ifa_dx_val, &ia_index, &v_value); 

  /*---  ---*/
  _shouldDoBackward = false;
  if (zero_num > 0) {
    if (max_val < 0) { /* zero is at the end */
      _shouldDoBackward = false; 
    }
    else if (min_val > 0) { /* zero is at the beginning */
      _shouldDoBackward = true; 
    }
    else {
      /*---  we can't avoid getting indexes for zero values  ---*/
      v_data_transpose->filter(ia_dx, NULL, &ia_zero); 
    }
  }
}

/*------------------------------------------------------*/
void AzSortedFeat_Sparse::filter(const AzSortedFeat_Sparse *inp, 
                          const AzIntArr *ia_isYes, 
                          int yes_num)
{
  sub_initialize(inp, yes_num, this); 

  int max_dx = ia_isYes->size() - 1; 
  const int *isYes = ia_isYes->point(); 
  int inp_zero_num; 
  const int *inp_zero = inp->ia_zero.point(&inp_zero_num); 
  int ix; 
  for (ix = 0; ix < inp_zero_num; ++ix) {
    int dx = inp_zero[ix]; 
    if (dx <= max_dx && isYes[dx]) {
      ia_zero.put(dx); 
    }
  }

  int inp_index_num; 
  const int *inp_index = inp->ia_index.point(&inp_index_num); 
  const double *inp_value = inp->v_value.point(); 
  int where_is_zero = -1; 
  for (ix = 0; ix < inp_index_num; ++ix) {
    int dx = inp_index[ix]; 
    if (dx == AzNone) {  /* place holder for zero */
      ia_index.put(AzNone); 
      where_is_zero = ia_index.size()-1; 
    }
    else if (dx <= max_dx && isYes[dx]) {
      ia_index.put(dx); 
      v_value.set(ia_index.size()-1, inp_value[ix]); 
    }
  }

  sub_terminate(this, yes_num, where_is_zero); 
}

/*------------------------------------------------------*/
void AzSortedFeat_Sparse::separate(const AzSortedFeat_Sparse *inp, 
                          const AzIntArr *ia_isYes, 
                          int yes_num, 
                          AzSortedFeat_Sparse *yes, 
                          AzSortedFeat_Sparse *no)
{
  sub_initialize(inp, yes_num, yes); 
  int no_num = inp->data_num - yes_num; 
  sub_initialize(inp, no_num, no); 

  int max_dx = ia_isYes->size() - 1; 
  const int *isYes = ia_isYes->point(); 
  int inp_zero_num; 
  const int *inp_zero = inp->ia_zero.point(&inp_zero_num); 
  int ix; 
  for (ix = 0; ix < inp_zero_num; ++ix) {
    int dx = inp_zero[ix]; 
    if (dx <= max_dx && isYes[dx]) {
      yes->ia_zero.put(dx); 
    }
    else {
      no->ia_zero.put(dx); 
    }
  }

  int inp_index_num; 
  const int *inp_index = inp->ia_index.point(&inp_index_num); 
  const double *inp_value = inp->v_value.point(); 
  int yes_where_is_zero = -1, no_where_is_zero = -1; 
  for (ix = 0; ix < inp_index_num; ++ix) {
    int dx = inp_index[ix]; 
    if (dx == AzNone) {  /* place holder for zero */
      yes->ia_index.put(AzNone); 
      no->ia_index.put(AzNone); 
      yes_where_is_zero = yes->ia_index.size()-1; 
      no_where_is_zero = no->ia_index.size()-1; 
    }
    else if (dx <= max_dx && isYes[dx]) {
      yes->ia_index.put(dx); 
      yes->v_value.set(yes->ia_index.size()-1, inp_value[ix]); 
    }
    else {
      no->ia_index.put(dx); 
      no->v_value.set(no->ia_index.size()-1, inp_value[ix]); 
    }
  }

  sub_terminate(yes, yes_num, yes_where_is_zero); 
  sub_terminate(no, no_num, no_where_is_zero); 
}

/*------------------------------------------------------*/
void AzSortedFeat_Sparse::sub_initialize(const AzSortedFeat_Sparse *inp, 
                                 int num, 
                                 AzSortedFeat_Sparse *ptr)
{
  ptr->ia_zero.reset(); 
  ptr->ia_zero.prepare(MIN(num, inp->ia_zero.size())); 
  ptr->ia_index.reset(); 
  int i_max = MIN(inp->ia_index.size(), num+1); /* plus one for zero */
  ptr->ia_index.prepare(i_max); 
  ptr->v_value.reform(i_max); 
  ptr->_shouldDoBackward = inp->_shouldDoBackward; 
}

/*------------------------------------------------------*/
void AzSortedFeat_Sparse::sub_terminate(AzSortedFeat_Sparse *ptr, 
                                      int num, 
                                      int where_is_zero)
{
  const char *eyec = "AzSortedFeat_Sparse::sub_terminate"; 
  if (ptr->v_value.rowNum() > ptr->ia_index.size()) {
    ptr->v_value.resize(ptr->ia_index.size()); 
  }
  ptr->data_num = num; 
  int zero_num = 0; 
  if (where_is_zero >= 0) {
    zero_num = num - (ptr->ia_index.size() - 1); /* -1 for dummy entry for zero */
    if (zero_num < 0) {
      throw new AzException(eyec, "numbers don't match around zero"); 
    }
    if (zero_num == 0) {
      /*---  remove dummy entry  ---*/
      ptr->ia_index.remove(where_is_zero); 
      double *value = ptr->v_value.point_u(); 
      int ix; 
      for (ix = where_is_zero+1; ix < ptr->v_value.rowNum(); ++ix) {
        value[ix-1] = value[ix]; 
      }
    }
  }
  else {
    if (ptr->ia_index.size() != num) {
      throw new AzException(eyec, "numbers don't match around zero"); 
    }
  }

  ptr->_shouldDoBackward = false; 
  if (zero_num == 0) {
    if (ptr->ia_zero.size() > 0) {
      throw new AzException(eyec, "conflict in #zero"); 
    }
  }
  else {
    if (where_is_zero == 0) {
      ptr->_shouldDoBackward = true; 
    }
  }
}

/*------------------------------------------------------*/
void AzSortedFeat_Sparse::copy(const AzSortedFeat_Sparse *inp) 
{
  /*---  copy  ---*/    
  ia_zero.reset(&inp->ia_zero); 
  ia_index.reset(&inp->ia_index); 
  v_value.set(&inp->v_value); 
  data_num = inp->data_num; 
  _shouldDoBackward = inp->_shouldDoBackward; 
}

/*--------------------------------------------------------*/
const int *AzSortedFeat_Sparse::forward(AzCursor &cur, 
                              double *out_val, int *out_num) /* output */
const 
{
  const char *eyec = "AzSortedFeat_Sparse::forward"; 
  if (_shouldDoBackward) {
    throw new AzException(eyec, "_shouldDoBackward is on"); 
  }

  int num; 
  const int *index = ia_index.point(&num); 
  int cursor = cur.get(); 
  if (cursor >= num) {
    return NULL;  /* end of data */
  }

  const double *value = v_value.point(); 

  int dx = index[cursor]; 
  double curr_val = value[cursor]; 
  if (dx == AzNone) { /* value=0 */
    cursor = cur.inc(); 
    if (cursor >= num) {
      return NULL; /* since this will cause all vs empty anyway */
    }
    if (ia_zero.size() <= 0) {
      throw new AzException(eyec, "empty zero in the middle.  something is wrong"); 
    }
    double next_val = value[cursor]; 
    *out_val = (curr_val + next_val) / 2; 
    *out_num = ia_zero.size(); 
    return ia_zero.point(); 
  }

  int begin = cursor; 
  cursor = cur.inc(); 
  double avg_val = curr_val + 0.00000001; 
  for ( ; cursor < num; cursor=cur.inc()) {
    double next_val = value[cursor]; 
    if (next_val != curr_val) {
      avg_val = (curr_val + next_val) / 2; 
      break; 
    }
  }

  if (cursor >= num) {
    return NULL;  /* end of data */
  }

  *out_val = avg_val; 
  *out_num = cursor - begin; 
  return index + begin;   
}

/*--------------------------------------------------------*/
const int *AzSortedFeat_Sparse::backward(AzCursor &cur, 
                              double *out_val, int *out_num) /* output */
const
{
  const char *eyec = "AzSortedFeat_Sparse::backward"; 
  if (!_shouldDoBackward) {
    throw new AzException(eyec, "_shouldDoBackward is off"); 
  }

  int num; 
  const int *index = ia_index.point(&num); 
  int cursor = cur.get(); 
  if (cursor < 1) {
    return NULL;  /* end of data */
  }

  const double *value = v_value.point(); 

  int dx = index[cursor-1]; 
  double curr_val = value[cursor-1]; 
  if (dx == AzNone) { /* value=0 */
    cursor = cur.dec(); 
    if (cursor < 1) {
      return NULL; /* b/c this will cause empty vs all anyway */
    }
    if (ia_zero.size() <= 0) {
      throw new AzException(eyec, "empty zero in the middle.  something is wrong"); 
    }
    double prev_val = value[cursor-1]; 
    *out_val = (curr_val + prev_val)/2; 
    *out_num = ia_zero.size(); 
    return ia_zero.point(); 
  }

  int end = cursor; 
  cursor = cur.dec(); 
  double avg_val = curr_val - 0.00000001; 
  for ( ; cursor >= 1; cursor=cur.dec()) {
    double prev_val = value[cursor-1]; 
    if (prev_val != curr_val) {
      avg_val = (curr_val + prev_val) / 2; 
      break; 
    }
  }

  if (cursor < 1) {
    return NULL;  /* end of data */
  }

  *out_val = avg_val; 
  *out_num = end - cursor; 
  return index + cursor;   
}

/*------------------------------------------------------*/
void AzSortedFeat_Sparse::getIndexes(const int *inp_dxs, 
                              int inp_dxs_num, 
                              double border_val, 
                              /*---  output  ---*/
                              AzIntArr *ia_le_dx, 
                              AzIntArr *ia_gt_dx)
const
{
  ia_le_dx->reset(); 
  ia_gt_dx->reset(); 
  ia_le_dx->prepare(inp_dxs_num); 
  ia_gt_dx->prepare(inp_dxs_num); 

  AzIntArr ia_temp; 
  const AzIntArr *ia_zero_index = getIndexes_Zero(inp_dxs, inp_dxs_num, &ia_temp); 

  int num; 
  const int *index = ia_index.point(&num); 
  const double *value = v_value.point(); 

  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int dx = index[ix]; 
    if (value[ix] == 0) {
      if (ia_zero_index->size() > 0) {
        if (0 <= border_val) {
          ia_le_dx->concat(ia_zero_index);  
        }    
        else {
          ia_gt_dx->concat(ia_zero_index); 
        }
      }
    }
    else if (value[ix] <= border_val) {
      ia_le_dx->put(dx); 
    }
    else {
      ia_gt_dx->put(dx); 
    }
  }
  if (ia_le_dx->size() + ia_gt_dx->size() != inp_dxs_num) {
    throw new AzException("AzSortedFeat_Sparse::getIndexes", "num conflict"); 
  }
}

/*------------------------------------------------------*/
const AzIntArr *AzSortedFeat_Sparse::getIndexes_Zero(
                          const int *inp_dxs, 
                          int inp_dxs_num, 
                          AzIntArr *out_ia_zero)
const
{
  if (ia_zero.size() > 0) {
    return &ia_zero; 
  }

  AzIntArr ia_isNonZero; 
  bool isThereZero = ia_index.toOnOff(&ia_isNonZero); 
  if (!isThereZero) return &ia_zero; 

  out_ia_zero->reset(); 
  out_ia_zero->prepare(inp_dxs_num - ia_index.size() + 1); 
  int nz_dx_max = ia_isNonZero.size()-1; 
  const int *isNonZero = ia_isNonZero.point(); 
  int jx; 
  for (jx = 0; jx < inp_dxs_num; ++jx) {
    int dx = inp_dxs[jx]; 
    if (dx > nz_dx_max || !isNonZero[dx]) {
      out_ia_zero->put(dx); 
    }
  }
  return out_ia_zero; 
}


/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void AzSortedFeatArr::reset_sparse(const AzSmat *m_tran, 
                            bool inp_beTight)
{
  const char *eyec = "AzSortedFeatArr::reset_sparse"; 
  beTight = inp_beTight; 
  f_num = m_tran->colNum(); 
  int data_num = m_tran->rowNum(); 
  ia_isActive.reset(); 
  active_num = 0; 

  a_sparse.free(&arrs); 
  a_dense.free(&arrd); 
  a_sparse.alloc(&arrs, f_num, eyec, "arrs"); 

  int nz_num = 0; 
  AzIntArr ia_all_dx; 
  ia_all_dx.range(0, data_num); 
  int fx; 
  for (fx = 0; fx < f_num; ++fx) {
    arrs[fx] = new AzSortedFeat_Sparse(m_tran->col(fx), &ia_all_dx); 
  }
}

/*--------------------------------------------------------*/
void AzSortedFeatArr::reset_dense(const AzDmat *m_tran_dense,   /* set */
                                  bool inp_beTight)
{
  const char *eyec = "AzSortedFeatArr::reset (dense)"; 

  beTight = inp_beTight; 
  f_num = m_tran_dense->colNum(); 
  int data_num = m_tran_dense->rowNum(); 
  ia_isActive.reset(); 
  active_num = 0; 

  a_sparse.free(&arrs); 
  a_dense.free(&arrd); 
  a_dense.alloc(&arrd, f_num, eyec, "arrd"); 

  int nz_num = 0; 
  AzIntArr ia_all_dx; 
  ia_all_dx.range(0, data_num); 
  int fx; 
  for (fx = 0; fx < f_num; ++fx) {
    arrd[fx] = new AzSortedFeat_Dense(m_tran_dense->col(fx), &ia_all_dx); 
  }
}

/*--------------------------------------------------------*/
void AzSortedFeatArr::copy_base(const AzSortedFeatArr *inp)
{
  const char *eyec = "AzSortedFeatArr::copy_base"; 
  beTight = inp->beTight; 
  f_num = inp->featNum(); 
  a_sparse.free(&arrs); 
  a_dense.free(&arrd); 

  ia_isActive.reset(); 
  active_num = 0; 

  if (beTight) {
    return; 
  }

  if (inp->doingSparse()) {
    a_sparse.alloc(&arrs, f_num, eyec, "arrs"); 
    int fx; 
    for (fx = 0; fx < f_num; ++fx) {
      if (inp->arrs == NULL || inp->arrs[fx] == NULL) {
        throw new AzException(eyec, "No sorted sparse features?!");
      }
      arrs[fx] = new AzSortedFeat_Sparse(inp->arrs[fx]); 
    }
  }
  else {
    a_dense.alloc(&arrd, f_num, eyec, "arrd"); 
    int fx; 
    for (fx = 0; fx < f_num; ++fx) {
      if (inp->arrd == NULL || inp->arrd[fx] == NULL) {
        throw new AzException(eyec, "No sorted dense features?!");
      }
      arrd[fx] = new AzSortedFeat_Dense(inp->arrd[fx]); 
    }
  }
}

/*--------------------------------------------------------*/
/*
 *  called when beTight is on. 
 */
const AzSortedFeat *AzSortedFeatArr::sorted(const AzSortedFeatArr *inp, 
                             int fx, 
                             AzSortedFeatWork *out) const 
{
  const char *eyec = "AzSortedFeatArr::sorted(inp,fx,work0)"; 
  if (f_num != inp->featNum() || 
      ia_isActive.size() <= 0 || active_num <= 0) {
    throw new AzException(eyec, "not ready?!"); 
  }
  if (fx < 0 || fx >= f_num) {
    throw new AzException(eyec, "out of range"); 
  }
  if (inp->doingSparse()) {
    if (inp->arrs == NULL || inp->arrs[fx] == NULL) {
      throw new AzException(eyec, "Expected sparse sorted features as input"); 
    }
    out->tmps.filter(inp->arrs[fx], &ia_isActive, active_num); 
    return &out->tmps; 
  }
  else {
    if (inp->arrd == NULL || inp->arrd[fx] == NULL) {
      throw new AzException(eyec, "Expected dense sorted features as input"); 
    }
    out->tmpd.filter(inp->arrd[fx], &ia_isActive, active_num); 
    return &out->tmpd; 
  }
}

/*--------------------------------------------------------*/
void AzSortedFeatArr::sub_initialize(const AzSortedFeatArr *inp, 
                      AzSortedFeatArr *ptr)
{
  ptr->ia_isActive.reset(); 
  ptr->active_num = 0; 
  ptr->beTight = inp->beTight; 
  ptr->f_num = inp->featNum(); 
  ptr->a_sparse.free(&ptr->arrs);
  ptr->a_dense.free(&ptr->arrd); 

  if (!inp->beTight) {
    const char *eyec = "AzSortedFeatArr::sub_initialize"; 
    if (inp->doingSparse()) {
      ptr->a_sparse.alloc(&ptr->arrs, ptr->f_num, eyec, "arrs"); 
    }
    else {
      ptr->a_dense.alloc(&ptr->arrd, ptr->f_num, eyec, "arrd");
    }
  }
}

/*--------------------------------------------------------*/
/* static */
void AzSortedFeatArr::separate(AzSortedFeatArr *base, /* used only by Dense */
             const AzSortedFeatArr *inp, 
             const int *yes_dxs, int yes_dxs_num, 
             const int *no_dxs, int no_dxs_num, 
             AzSortedFeatArr *yes, AzSortedFeatArr *no) 
{
  const char *eyec = "AzSortedFeatArr::separate"; 
  sub_initialize(inp, yes); 
  sub_initialize(inp, no); 

  AzIntArr ia_isActive; 
  ia_isActive.toOnOff(yes_dxs, yes_dxs_num); 
  int active_num = yes_dxs_num; 

  if (inp->beTight) {
    yes->ia_isActive.reset(&ia_isActive); 
    yes->active_num = active_num; 
    no->ia_isActive.toOnOff(no_dxs, no_dxs_num); 
    no->active_num = no_dxs_num; 
    return; 
  }

  if (inp->doingSparse()) {
    int fx; 
    for (fx = 0; fx < inp->featNum(); ++fx) {
      if (inp->arrs == NULL || inp->arrs[fx] == NULL) {
        throw new AzException(eyec, "No sparse sorted featuers given as input"); 
      }
      yes->arrs[fx] = new AzSortedFeat_Sparse(); 
      no->arrs[fx] = new AzSortedFeat_Sparse(); 
      AzSortedFeat_Sparse::separate(inp->arrs[fx], &ia_isActive, active_num,  
                             yes->arrs[fx], no->arrs[fx]); 
      if (yes->arrs[fx]->dataNum() != yes_dxs_num || 
          no->arrs[fx]->dataNum() != no_dxs_num) {
        throw new AzException(eyec, "conflict in pop (sparse)"); 
      }
    }
  }
  else {
    if (base == NULL) {
      throw new AzException(eyec, "base is null.  something is wrong"); 
    }
    int fx; 
    for (fx = 0; fx < inp->featNum(); ++fx) {
      if (base->arrd == NULL || base->arrd[fx] == NULL) {
        throw new AzException(eyec, "No dense sorted featuers given as base"); 
      }
      if (inp->arrd == NULL || inp->arrd[fx] == NULL) {
        throw new AzException(eyec, "No dense sorted featuers given as input"); 
      }
      yes->arrd[fx] = new AzSortedFeat_Dense(); 
      no->arrd[fx] = new AzSortedFeat_Dense(); 
      AzSortedFeat_Dense::separate(base->arrd[fx], 
                               inp->arrd[fx], &ia_isActive, active_num,  
                               yes->arrd[fx], no->arrd[fx]); 
      if (yes->arrd[fx]->dataNum() != yes_dxs_num || 
          no->arrd[fx]->dataNum() != no_dxs_num) {
        throw new AzException(eyec, "conflict in pop (dense)"); 
      }
    }
  }
}
