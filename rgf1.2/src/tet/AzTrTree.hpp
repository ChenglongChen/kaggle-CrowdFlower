/* * * * *
 *  AzTrTree.hpp 
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

#ifndef _AZ_TR_TREE_HPP_
#define _AZ_TR_TREE_HPP_

#include "AzUtil.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTreeRule.hpp"
#include "AzLoss.hpp"
#include "AzSvFeatInfo.hpp"
#include "AzTreeNodes.hpp"
#include "AzTrTree_ReadOnly.hpp"
#include "AzTrTsplit.hpp"
#include "AzTrTtarget.hpp"
#include "AzTrTreeNode.hpp"
#include "AzTree.hpp"

/*---------------------------------------------*/
/* Abstract class: Trainable Tree              */
/* Derived classes: AzStdTree, AzRgfTree       */
/*---------------------------------------------*/
//! Abstract class: Trainable trees.  Building blocks for AzStdTree and AzRgfTree.  
class AzTrTree
 : /* implements */ public virtual AzTrTree_ReadOnly
{
protected:
  int root_nx; 
  int nodes_used; 
  AzTrTreeNode *nodes; 
  AzObjArray<AzTrTreeNode> a_node; 

  AzIntArr ia_root_dx; /*!!! Do NOT add components after generating the root.  */
                       /*!!! All the nodes refer to the components by pointer. */

  AzTrTsplit **split;  
  AzObjPtrArray<AzTrTsplit> a_split; 

  AzSortedFeatArr **sorted_arr; 
  AzObjPtrArray<AzSortedFeatArr> a_sorted_arr; 

  int curr_min_pop, curr_max_depth; 
  bool isBagging; 

public:
  AzTrTree() : 
    nodes_used(0), nodes(NULL), split(NULL), sorted_arr(NULL), root_nx(AzNone), 
    curr_min_pop(-1), curr_max_depth(-1), isBagging(false) {}

  /*---  derived classes must implement these             ---*/
  /*---------------------------------------------------------*/
  virtual bool usingInternalNodes() const = 0; 
  /*!  Return which nodes should make features.  */
  virtual void isActiveNode(bool doAllowZeroWeightLeaf, 
                    AzIntArr *ia_isActiveNode) /*!< output: array of size #node */
                    const = 0;
  /*---------------------------------------------------------*/

  /*---  for faster node search  ---*/
  virtual const AzSortedFeatArr *sorted_array(int nx, 
                             const AzDataForTrTree *data) const; 

  /*---  information seeking ... ---*/
  inline int maxDepth() const {
    return curr_max_depth; 
  }
  inline int nodeNum() const {
    return nodes_used; 
  }
  inline int root() const {
    return root_nx; 
  }
  inline const AzTrTreeNode *node(int nx) const {
    _checkNode(nx, "look"); 
    return &nodes[nx];  
  }
  bool isEmptyTree() const; 
  int leafNum() const; 
  void show(const AzSvFeatInfo *feat, const AzOut &out) const; 
  void concat_stat(AzBytArr *o) const; 
  double getRule(int inp_nx, AzTreeRule *rule) const; 
  void concatDesc(const AzSvFeatInfo *feat, int nx, 
                  AzBytArr *str_desc, /*!< output */
                  int max_len=-1) const; 

  /*---  apply ... ---*/
  void apply(const AzDataForTrTree *data, 
             const AzIntArr *ia_dx, 
             AzDvect *v_p) const; /* output */
  double apply(const AzDataForTrTree *data, int dx, 
               AzIntArr *ia_nx=NULL) const; /* node path */
  double apply(const AzSvect *v) const {
    return AzTree::apply(v, this); 
  }

  void updatePred(const AzDataForTrTree *dfd, 
                  AzDvect *v_pval) const; /* inout */

  /*---  prohibit =  ---*/
  inline AzTrTree & operator =(const AzTrTree &inp) {
    if (this == &inp) return *this; 
    throw new AzException("AzTrTree:=", "Don't use ="); 
  }

  /*-----*/
  void copy_to(AzTree *tree) const; 

  //! copy only nodes; no split.  
  virtual void copy_nodes_from(const AzTrTree_ReadOnly *inp); 

  virtual void warmup(const AzTreeNodes *inp, 
                     const AzDataForTrTree *data, 
                     AzDvect *v_p, /* inout */
                     const AzIntArr *ia_tr_dx=NULL); 
  virtual void quick_warmup(const AzTreeNodes *inp, 
                      const AzDataForTrTree *data, 
                      AzDvect *v_p, /* inout */
                      const AzIntArr *inp_ia_tr_dx); /* may be NULL */

  static double init_const(AzLossType loss_type, 
                           const AzDvect *v_y, 
                           const AzIntArr *ia_tr_dx=NULL); /* may be NULL*/
  static double init_constw(AzLossType loss_type, 
                            const AzDvect *v_y, 
                            const AzDvect *v_fixed_dw, /* may be NULL */
                            const AzIntArr *ia_tr_dx=NULL); /* may be NULL*/

  inline const AzIntArr *root_dx() const {
    return &ia_root_dx; 
  }

  /*---  to store data indexes to disk  ---*/
  virtual void forStoringDataIndexes(AzFile *file) {}
  virtual int estimateSizeofDataIndexes(int data_num) {return -1;}
protected:
  /*---  tools for derived classes; for building a tree  ---*/
  void _release(); 
  void _releaseWork(); 
  int _newNode(int max_size); 
  void _genRoot(int max_size, const AzDataForTrTree *data, 
                const AzIntArr *ia_dx=NULL); 
  void _splitNode(const AzDataForTrTree *data, 
                 int max_size, bool doUseInternalNodes, 
                 int nx,
                 const AzTrTsplit *split, 
                 const AzOut &out); 

  /*---  sub-routines for information seeking ... ---*/
  void _show(const AzSvFeatInfo *feat, 
             int nx, 
             int depth, 
             const AzOut &out) const; 
  void _genDesc(const AzBytArr *str_parent, 
                      AzBytArr str_feat, 
                      double border_val, 
                      bool isLe, 
                      AzBytArr *str_desc) const; /* output */
  void _genDesc(const AzSvFeatInfo *feat, 
                int nx, 
                AzBytArr *str_desc) const; /* output */

  /*---  check consistency  ---*/
  inline void _checkNode(int nx, const char *eyec) const {
    if (nodes == NULL || nx < 0 || nx >= nodes_used) {
      throw new AzException(eyec, "nx is out of range"); 
    }
  }
  inline void _checkNodes(const char *msg) const {
    if (nodes == NULL && nodes_used > 0) {
      throw new AzException("no nodes", msg); 
    }
  }

  /*---  ---*/
  void _isActiveNode(bool doWantInternalNodes, 
                     bool doAllowZeroWeightLeaf, 
                     AzIntArr *ia_isDecisionNode) const; /* output */


  void dump_split(const AzTrTsplit *inp, 
                     int nx, 
                     double org_weight, 
                     const AzOut &out); 

  /*---  change the contents of ia_root_dx  ---*/
  const int *set_data_indexes(int offset, 
                     const int *dxs, 
                     int dxs_num); 

  void orderLeaves(AzIntArr *ia_leaf_in_order); 
  void _orderLeaves(AzIntArr *ia_leaf_in_order, 
                    int nx); 
}; 

#endif 
