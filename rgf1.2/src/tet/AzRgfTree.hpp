/* * * * *
 *  AzRgfTree.hpp 
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

#ifndef _AZ_RGF_TREE_HPP_
#define _AZ_RGF_TREE_HPP_

#include "AzUtil.hpp"
#include "AzTrTree.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTrTtarget.hpp"
#include "AzRgf_FindSplit.hpp"
#include "AzParam.hpp"

class AzRgfTreeTemp {
public:
  AzFile *file;  
  int offset; 
  int node_num; 
  AzRgfTreeTemp() : offset(-1), node_num(0), file(NULL) {}
  inline void reset(AzFile *inp_file) {
    file = inp_file; 
    offset = -1; 
    node_num = 0; 
  }
  inline bool canStore() {
    if (file == NULL) return false; 
    return true; 
  }
  inline bool isStored() {
    if (offset >= 0) {
      return true; 
    }
    return false; 
  }
  void set(int inp_offset, int inp_node_num) {
    offset = inp_offset; 
    node_num = inp_node_num; 
  }
};

//! Tree for RGF.  
/*------------------------------------------*/
class AzRgfTree : /* extends */ public virtual AzTrTree
{
protected:
  int max_depth, max_leaf_num, min_size; 
  bool doUseInternalNodes; 
  AzOut out, my_dmp_out; 
  bool beVerbose; 

  AzRgfTreeTemp wk; 

  const static int min_size_dflt = 10; 

public:
  AzRgfTree() 
    : max_depth(-1), max_leaf_num(-1), min_size(min_size_dflt), 
	doUseInternalNodes(false), beVerbose(false), 
      my_dmp_out(dmp_out) {}
  AzRgfTree(AzParam &param) 
    : max_depth(-1), max_leaf_num(-1), min_size(min_size_dflt), 
	doUseInternalNodes(false), beVerbose(false), 
      my_dmp_out(dmp_out) {
    resetParam(param); 
  }

  void reset(const AzOut &out_req) {
    out = out_req; 
  }
  virtual void reset(AzParam &param) {
    AzTrTree::_release(); 
    resetParam(param); 
  }

  /*---  to store data indexes to disk  ---*/
  virtual void forStoringDataIndexes(AzFile *file) {
    wk.reset(file); 
  }
  virtual void storeDataIndexes(); 
  virtual void releaseDataIndexes(); 
  virtual void restoreDataIndexes(); 
  virtual int estimateSizeofDataIndexes(int data_num) const; 

  /*---  ---*/
  virtual void resetParam(AzParam &param); 
  virtual void printParam(const AzOut &out) const; 
  virtual void printHelp(AzHelp &h) const; 

  virtual 
  void findSplit(AzRgf_FindSplit *fs, 
                 const AzRgf_FindSplit_input &inp, 
                 bool doRefreshAll, 
                 /*---  output  ---*/
                 AzTrTsplit *best_split) const; 

  inline virtual int makeRoot(const AzDataForTrTree *dfd, 
                      const AzIntArr *ia_tr_dx=NULL) {
    AzTrTree::_genRoot(max_leaf_num, dfd, ia_tr_dx); 
    return root_nx; 
  }

  inline virtual void splitNode(const AzDataForTrTree *data, 
                                const AzTrTsplit *split) {
    AzTrTree::_splitNode(data, max_leaf_num, 
                         doUseInternalNodes, 
                         split->nx, split, my_dmp_out); 
  }

  virtual void removeSplitAssessment(); 

  /*---  weight updating ...  ---*/
  virtual void resetWeights(); 
  virtual void setWeight(int nx, double weight) {
    AzTrTree::_checkNode(nx, "setWeight"); 
    nodes[nx].weight = weight; 
  }

  inline bool usingInternalNodes() const { return doUseInternalNodes; }
  inline void isActiveNode(bool doAllowZeroWeightLeaf, 
                    AzIntArr *ia_isDecisionNode) const { /* output */
    AzTrTree::_isActiveNode(doUseInternalNodes, doAllowZeroWeightLeaf, 
                            ia_isDecisionNode); 
  }

  /*---  prohibit =  ---*/
  inline AzRgfTree & operator =(const AzRgfTree &inp) {
    if (this == &inp) return *this; 
    throw new AzException("AzRgfTree:=", "Don't use ="); 
  }

  //! copy only nodes.  no split.  
  void copy_nodes_from(const AzTrTree_ReadOnly *inp) {
    AzTrTree::copy_nodes_from(inp); 
    doUseInternalNodes = inp->usingInternalNodes(); 
  }
  virtual inline void releaseWork() {
    AzTrTree::_releaseWork(); 
    storeDataIndexes(); 
  }

  virtual void quick_warmup(const AzTreeNodes *inp, 
                      const AzDataForTrTree *data, 
                      AzDvect *v_p, /* inout */
                      const AzIntArr *inp_ia_tr_dx) /* may be NULL */
  {
    AzTrTree::quick_warmup(inp, data, v_p, inp_ia_tr_dx); 
    storeDataIndexes(); 
  }

protected:
  virtual inline void _findSplit_begin(AzRgf_FindSplit *fs, 
                              const AzRgf_FindSplit_input &inp) const {
    fs->begin(this, inp, min_size); 
  }

  virtual inline void _findSplit(AzRgf_FindSplit *fs, 
                                 int nx, bool doRefreshAll) const {
    if (doRefreshAll || 
        split[nx] == NULL || 
        nx == root_nx) {
      if (split[nx] == NULL) split[nx] = new AzTrTsplit(); 
      else                   split[nx]->reset();
      fs->findSplit(nx, split[nx]); 
    }
  }
  virtual inline void _findSplit_end(AzRgf_FindSplit *fs) const {
    fs->end(); 
  }

  virtual void adjustParam(); 
}; 

#endif 
