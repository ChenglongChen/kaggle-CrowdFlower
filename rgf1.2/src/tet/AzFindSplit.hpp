/* * * * *
 *  AzFindSplit.hpp 
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

#ifndef _AZ_FIND_SPLIT_HPP_
#define _AZ_FIND_SPLIT_HPP_

#include "AzUtil.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTrTtarget.hpp"
#include "AzTrTsplit.hpp"
#include "AzTrTree.hpp"

class Az_forFindSplit {
public:
  double wy_sum, w_sum; 
  Az_forFindSplit() : wy_sum(0), w_sum(0) {}
  void reset() {
    wy_sum = w_sum = 0; 
  }
};

//! Abstract class: provides building blocks for node split search. 
/*------------------------------------------*/
class AzFindSplit 
{
protected:
  const AzTrTtarget *target; 
  const AzDataForTrTree *data; 
  const AzTrTree_ReadOnly *tree; 
  int min_size; 

  AzIntArr ia_feats; 
  const AzIntArr *ia_fx; 

public:
  AzFindSplit() : target(NULL), data(NULL), tree(NULL), ia_fx(NULL), 
                  min_size(-1) {}
  ~AzFindSplit() {}
  void reset() {
    target = NULL;
    data = NULL; 
    tree = NULL;  
    min_size = -1; 
  }

  void _begin(const AzTrTree_ReadOnly *inp_tree, 
              const AzDataForTrTree *inp_data, 
              const AzTrTtarget *inp_target, 
              int inp_min_size); 
  void _end() {
    reset(); 
  }

  //----------------------------------------------------------------
  //  void findBestSplit(const AzTrTtarget *tar, 
  //                 const AzIntArr *ia_dx,              
  //                 ... parameters ...
  //                 AzTrTsplit *best_split); /* output */
  //----------------------------------------------------------------

  virtual void _pickFeats(int pick_num, int f_num); 

protected: 
  /*----------------------------------------------------------------*/
  virtual double getBestGain(double w_sum, 
                             double wy_sum, 
                             double *out_best_p) /* must not be null */
                             const = 0; 
  virtual double evalSplit(const Az_forFindSplit i[2], 
                           double bestP[2]) /* output */
                           const; 
  /*----------------------------------------------------------------*/

  void _findBestSplit(int nx, 
                      /*---  output  ---*/
                      AzTrTsplit *best_split); 
  void loop(AzTrTsplit *best_split, 
            int fx, /* feature# */
            const AzSortedFeat *sorted, 
            int dxs_num, 
            const Az_forFindSplit *total); 
}; 

#endif 
