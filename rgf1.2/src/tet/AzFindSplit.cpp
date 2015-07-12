/* * * * *
 *  AzFindSplit.cpp 
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

#include "AzFindSplit.hpp"

/*--------------------------------------------------------*/
void AzFindSplit::_begin(const AzTrTree_ReadOnly *inp_tree, 
                         const AzDataForTrTree *inp_data, 
                         const AzTrTtarget *inp_target, 
                         int inp_min_size)
{
  tree = inp_tree; 
  target = inp_target; 
  min_size = inp_min_size; 
  data = inp_data; 
}

/*--------------------------------------------------------*/
void AzFindSplit::_findBestSplit(int nx, 
                                 /*---  output  ---*/
                                 AzTrTsplit *best_split)
{
  const char *eyec = "AzFindSplit::_findBestSplit"; 

  if (tree == NULL || target == NULL || data == NULL) {
    throw new AzException(eyec, "information is not set"); 
  }

  const int *dxs = tree->node(nx)->data_indexes(); 
  const int dxs_num = tree->node(nx)->dxs_num; 

  const AzSortedFeatArr *sorted_arr = tree->sorted_array(nx, data); 
  if (sorted_arr == NULL) {
    throw new AzException(eyec, "No sorted array?!"); 
  }

  Az_forFindSplit total; 
  total.wy_sum = target->getTarDwSum(dxs, dxs_num);
  total.w_sum = target->getDwSum(dxs, dxs_num); 

  /*---  go through features to find the best split  ---*/
  int feat_num = data->featNum(); 
  const int *fxs = NULL; 
  if (ia_fx != NULL) {
    fxs = ia_fx->point(&feat_num); 
  }
  int ix; 
  for (ix = 0; ix < feat_num; ++ix) {
    int fx = ix; 
    if (fxs != NULL) fx = fxs[ix]; 

    AzSortedFeatWork tmp; 
    const AzSortedFeat *sorted = sorted_arr->sorted(fx); 
    if (sorted == NULL) { /* This happens only with Thrift or warm-start */
      const AzSortedFeat *my_sorted = sorted_arr->sorted(data->sorted_array(), fx, &tmp); 
      if (my_sorted->dataNum() != dxs_num) {
        throw new AzException(eyec, "conflict in #data"); 
      }
      loop(best_split, fx, my_sorted, dxs_num, &total); 
    }
    else {
      loop(best_split, fx, sorted, dxs_num, &total); 
    }
  }

  if (best_split->fx >= 0) {
    if (!dmp_out.isNull()) {
      data->featInfo()->desc(best_split->fx, &best_split->str_desc); 
    }
  }
}

/*--------------------------------------------------------*/
double AzFindSplit::evalSplit(const Az_forFindSplit i[2],
                              double bestP[2])
 const
{
  double gain = 0; 
  gain += getBestGain(i[0].w_sum, i[0].wy_sum, &bestP[0]); 
  gain += getBestGain(i[1].w_sum, i[1].wy_sum, &bestP[1]); 
  return gain; 
}

/*--------------------------------------------------------*/
void AzFindSplit::loop(AzTrTsplit *best_split, 
                       int fx, /* feature# */
                       const AzSortedFeat *sorted, 
                       int total_size, 
                       const Az_forFindSplit *total)
{
  /*---  first everyone is in GT(LE)  ---*/
  /*---  move the smallest(largest) ones from GT(LE) to LE(GT)  ---*/

  int dest_size = 0; 
  Az_forFindSplit i[2];
  Az_forFindSplit *src = &i[1], *dest = &i[0]; 
  double bestP[2] = {0,0}; 
  int le_idx, gt_idx; 
  if (sorted->isForward()) {
    le_idx = 0; 
    gt_idx = 1; 
  }
  else {
    le_idx = 1; 
    gt_idx = 0; 
  }

  AzCursor cursor; 
  sorted->rewind(cursor); 

  for ( ; ; ) {
    double value; 
    int index_num; 
    const int *index = NULL; 
    index = sorted->next(cursor, &value, &index_num); 
    if (index == NULL) break; 
    dest_size += index_num;  
    if (dest_size >= total_size) {
      break; /* don't allow all vs nothing */
    }

    const double *tarDw = target->tarDw_arr(); 
    const double *dw = target->dw_arr(); 
    double wy_sum_move = 0, w_sum_move = 0; 
    int ix; 
    for (ix = 0; ix < index_num; ++ix) {
      int dx = index[ix]; 
      wy_sum_move += tarDw[dx]; 
      w_sum_move += dw[dx]; 
    }
    dest->wy_sum += wy_sum_move; 
    dest->w_sum += w_sum_move; 

    if (min_size > 0) {
      if (dest_size < min_size) {
        continue; 
      }
      if (total_size - dest_size < min_size) {
        break; 
      }
    }

    src->wy_sum = total->wy_sum - dest->wy_sum; 
    src->w_sum  = total->w_sum  - dest->w_sum; 

    double gain = evalSplit(i, bestP); 
#if 0 
    best_split->keep_if_good(fx, value, gain, 
                        bestP[le_idx], bestP[gt_idx]); 
#else
    if (gain > best_split->gain) {
      best_split->reset_values(fx, value, gain, 
                        bestP[le_idx], bestP[gt_idx]); 
    }
#endif 
  }
}

/*--------------------------------------------------------*/
void AzFindSplit::_pickFeats(int pick_num, int f_num)
{
  if (pick_num < 1 || pick_num > f_num) {
    throw new AzException("AzFindSplit::pickFeats", "out of range"); 
  }
  ia_feats.reset(); 
  if (pick_num == f_num) {
    ia_fx = NULL; 
    return; 
  }

  AzIntArr ia_onOff; 
  ia_onOff.reset(f_num, 0); 
  int *onOff = ia_onOff.point_u(); 
  for ( ; ; ) {
    if (ia_feats.size() >= pick_num) break; 
    int fx = rand() % f_num; 
    if (onOff[fx] == 0) {
      onOff[fx] = 1; 
      ia_feats.put(fx); 
    }
  }
  ia_fx = &ia_feats; 
}
 