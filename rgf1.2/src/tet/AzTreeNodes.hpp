/* * * * *
 *  AzTreeNodes.hpp 
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

#ifndef _AZ_TREE_NODES_HPP_
#define _AZ_TREE_NODES_HPP_

#include "AzUtil.hpp"

/*! Tree node */
class AzTreeNode {
public:
  int fx;       //!< feature id 
  double border_val;
  int le_nx; //!< x[fx] <= border_val 
  int gt_nx; //!< x[fx] >  border_val 
  int parent_nx; //!< pointing parent node 
  double weight; //!< weight 

  /*---  ---*/
  AzTreeNode() {
    reset(); 
  }
  void reset() {
    border_val = weight = 0; 
    fx = le_nx = gt_nx = parent_nx = -1; 
  }
  AzTreeNode(AzFile *file) {
    read(file); 
  }
  inline bool isLeaf() const {
    if (le_nx < 0) return true; 
    return false; 
  }
  void write(AzFile *file); 
  void read(AzFile *file); 

  void transfer_from(AzTreeNode *inp) {
    *this = *inp; 
  }
}; 

class AzTreeNodes {
public:
  virtual const AzTreeNode *node(int nx) const = 0; 
  virtual int nodeNum() const = 0; 
  virtual int root() const = 0; 
}; 
#endif 
