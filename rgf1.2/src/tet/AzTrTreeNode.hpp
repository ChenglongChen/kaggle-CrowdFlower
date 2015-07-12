/* * * * *
 *  AzTrTreeNode.hpp 
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

#ifndef _AZ_TR_TREE_NODE_HPP_
#define _AZ_TR_TREE_NODE_HPP_

#include "AzTreeNodes.hpp"

class AzTrTree; 

/*---------------------------------------------*/
/*! used only for training */
class AzTrTreeNode : /* extends */ public virtual AzTreeNode {
protected:
  const int *dxs; /* data indexes belonging to this node */

public:
  int dxs_offset;  /* position in the data indexes at the root */
  int dxs_num; 
  int depth; //!< node depth 

  AzTrTreeNode() : depth(-1), dxs(NULL), dxs_offset(-1), dxs_num(-1) {}
  void reset() {
    AzTreeNode::reset(); 
    depth = dxs_offset = dxs_num = -1; 
    dxs = NULL; 
  }
  void transfer_from(AzTrTreeNode *inp) {
    AzTreeNode::transfer_from(inp); 
    dxs = inp->dxs; 
    dxs_offset = inp->dxs_offset; 
    dxs_num = inp->dxs_num; 
    depth = inp->depth; 
  }

  inline const int *data_indexes() const {
    if (dxs_num > 0 && dxs == NULL) {
      throw new AzException("AzTrTreeNode::data_indexes", 
                            "data indexes are unavailable"); 
    }
    return dxs; 
  }
  inline void reset_data_indexes(const int *ptr) {
    dxs = ptr; 
  }

  friend class AzTrTree; 
}; 

#endif 
