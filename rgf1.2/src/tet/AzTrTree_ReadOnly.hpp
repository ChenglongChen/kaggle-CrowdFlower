/* * * * *
 *  AzTrTree_ReadOnly.hpp 
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

#ifndef _AZ_TR_TREE_READONLY_HPP_
#define _AZ_TR_TREE_READONLY_HPP_

#include "AzUtil.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTreeRule.hpp"
#include "AzSvFeatInfo.hpp"
#include "AzTreeNodes.hpp"
#include "AzTrTreeNode.hpp"
#include "AzSortedFeat.hpp"

//! Abstract class: interface for read-only (information-seeking) access to trainable tree.  
/*------------------------------------------*/
/* Trainable tree; read only */
class AzTrTree_ReadOnly : /* implements */ public virtual AzTreeNodes 
{
public:
  /*---  information seeking ... ---*/
  virtual int nodeNum() const = 0; 
  virtual int leafNum() const = 0; 
  virtual int maxDepth() const = 0; 
  virtual void show(const AzSvFeatInfo *feat, const AzOut &out) const = 0; 
  virtual void concat_stat(AzBytArr *o) const = 0; 
  virtual double getRule(int inp_nx, AzTreeRule *rule) const = 0; 
  virtual void concatDesc(const AzSvFeatInfo *feat, int nx, 
                  AzBytArr *str_desc, /* output */
                  int max_len=-1) const = 0; 
  virtual void isActiveNode(bool doAllowZeroWeightLeaf, 
                            AzIntArr *ia_isDecisionNode) const = 0; /* output */
  virtual bool usingInternalNodes() const = 0; 

  virtual const AzSortedFeatArr *sorted_array(int nx, 
                             const AzDataForTrTree *data) const = 0; 
                             /*--- (NOTE) this is const but changes sorted_arr[nx] ---*/

  virtual const AzIntArr *root_dx() const = 0; 

  /*---  apply ... ---*/
  virtual double apply(const AzDataForTrTree *data, int dx, 
                       AzIntArr *ia_nx=NULL) const /* node path */
                       = 0; 

  virtual const AzTrTreeNode *node(int nx) const = 0; 
}; 
#endif 
