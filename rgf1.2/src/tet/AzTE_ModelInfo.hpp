/* * * * *
 *  AzTE_ModelInfo.hpp 
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

#ifndef _AZ_TE_MODEL_INFO_HPP_
#define _AZ_TE_MODEL_INFO_HPP_

#include "AzUtil.hpp"

/*-------------------------------------------------------*/
/*!  Tree Ensemble model info */
class AzTE_ModelInfo {
public:
  int tree_num; //!< number of trees 
  int leaf_num; //!< number of features/leaf nodes 
  int f_num; //!< number of features including removed ones. 
  int nz_f_num; //!< number of non-zero-weight features after consolidation 
  AzBytArr s_sign; 
  AzBytArr s_config; //!< configuration 
  const char *sign;  //!< signature of trainer 
  AzTE_ModelInfo() : f_num(-1),leaf_num(-1),nz_f_num(-1),tree_num(-1) {}

  void reset() {
    tree_num = leaf_num = f_num = nz_f_num = -1; 
    s_sign.reset(); 
    s_config.reset(); 
  }
}; 
#endif 
