/* * * * *
 *  AzTree.hpp 
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

#ifndef _AZ_TREE_HPP_
#define _AZ_TREE_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzSvFeatInfo.hpp"
#include "AzTreeNodes.hpp"

//!  Untrainalbe regression tree.  
/*------------------------------------------*/
class AzTree : /* implements */ public virtual AzTreeNodes {
protected:
  int root_nx; 
  int nodes_used; 
  AzTreeNode *nodes; 
  AzBaseArray<AzTreeNode> a_nodes; 

  inline void _checkNode(int nx, const char *eyec) const {
    if (nodes == NULL || nx < 0 || nx >= nodes_used) {
      throw new AzException(eyec, "nx is out of range"); 
    }
  }

public:
  AzTree() : root_nx(-1), nodes_used(0), nodes(NULL) {}
  AzTree(AzFile *file) 
           : root_nx(-1), nodes_used(0), nodes(NULL) {
    _read(file); 
  }
  AzTree(const AzTreeNodes *inp) : root_nx(-1), nodes_used(0), nodes(NULL) {
    if (inp != NULL) {
      copy_from(inp); 
    }
  }
  ~AzTree() {}

  void copy_from(const AzTreeNodes *tree_nodes); 

  inline void transfer_from(AzTree *) {
    throw new AzException("AzTree::transfer_from", "no support"); 
  }
  inline AzTree & operator =(const AzTree &inp) {
    if (this == &inp) return *this; 
    throw new AzException("AzTree:=", "Don't use ="); 
  }
  void reset() {
    _release(); 
  }

  void write(AzFile *file); 
  void read(AzFile *file); 

  inline int nodeNum() const {
    return nodes_used; 
  }

  static double apply(const AzReadOnlyVector *v_data, 
                      const AzTreeNodes *nodes, 
                      AzIntArr *ia_node=NULL); 

  double apply(const AzReadOnlyVector *v_data, 
               AzIntArr *ia_node=NULL) const {
    checkNodes("apply"); 
    return apply(v_data, this, ia_node); 
  }

  void show(const AzSvFeatInfo *feat, const AzOut &out, 
            const char *header="") const; 
  int leafNum() const; 
  void clean_up(); 

  inline const AzTreeNode *node(int nx) const {
    checkNode(nx, "point"); 
    return &nodes[nx]; 
  }
  inline int root() const {
    return root_nx; 
  }
  void finfo(AzIFarr *ifa_fx_count, 
             AzIFarr *ifa_fx_sum) const; /* appended */
  void finfo(AzIntArr *ia_fxs) const; /* appended */

  void cooccurrences(AzIIFarr *iifa_fx1_fx2_count) const; 

  virtual void genDesc(const AzSvFeatInfo *feat, 
               int nx, 
               AzBytArr *s) /* output */
               const; 

protected:
  /*---  functions  ---*/
  void _read(AzFile *file); 

  inline void checkNode(int nx, const char *eyec) const {
    if (nodes == NULL || nx < 0 || nx >= nodes_used) {
      throw new AzException(eyec, "AzTree, nx is out of range"); 
    }
  }
  inline void checkNodes(const char *msg) const {
    if (nodes == NULL && nodes_used > 0) {
      throw new AzException("AzTree, no nodes", msg); 
    }
  }
  void _show(const AzSvFeatInfo *feat, 
                    int nx, 
                    int depth, 
                    const AzOut &out) const; 

  void _release(); 
  virtual void _genDesc(const AzSvFeatInfo *feat, 
                      int nx, 
                      AzBytArr *s) /* output */
                      const; 
}; 

#endif 
