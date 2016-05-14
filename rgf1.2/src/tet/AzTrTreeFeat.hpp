/* * * * *
 *  AzTrTreeFeat.hpp 
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

#ifndef _AZ_TR_TREE_FEAT_HPP_
#define _AZ_TR_TREE_FEAT_HPP_

#include "AzUtil.hpp"
#include "AzStrPool.hpp"
#include "AzIntPool.hpp"
#include "AzDmat.hpp"
#include "AzBmat.hpp"
#include "AzSvFeatInfo.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTrTreeEnsemble_ReadOnly.hpp"
#include "AzTreeRule.hpp"
#include "AzParam.hpp"
#include "AzHelp.hpp"

class AzTrTreeFeatInfo {
public:
  bool isRemoved; 
  int tx, nx; 
  AzBytArr rule; 

  AzTrTreeFeatInfo() {
    isRemoved = false; 
    tx = nx = -1; 
  }

  AzTrTreeFeatInfo(const AzTrTreeFeatInfo &inp) {
    reset(&inp);     
  }
  AzTrTreeFeatInfo(const AzTrTreeFeatInfo *inp) {
    reset(inp);     
  }
  AzTrTreeFeatInfo & operator =(const AzTrTreeFeatInfo &inp) {
    if (this == &inp) return *this; 
    reset(&inp); 
    return *this; 
  }
  void reset(const AzTrTreeFeatInfo *inp) {
    if (inp == NULL) return; 
    isRemoved = inp->isRemoved; 
    tx = inp->tx; 
    nx = inp->nx; 
    rule.reset(&inp->rule); 
  }
}; 

//! Assign feature id's to tree nodes; count duplicated rules. 
class AzTrTreeFeat : public virtual AzSvFeatInfo 
{
protected:
  bool doAllowZeroWeightLeaf; 

  /*! for maintenance purpose only */
  bool doCountRules; 
  bool doCheckConsistency; 

  AzSvFeatInfoClone org_featInfo; //!< original feature info 
  AzIntPool ip_featDef;  //!< [tree#] 
  AzStrPool sp_desc;     //!< [feat#]: may be empty 
  AzDataArray<AzTrTreeFeatInfo> f_inf;  //!< [feat#] 
  AzStrPool pool_rules, pool_rules_rmved; 
  AzOut out; 

public: 
  AzTrTreeFeat() : doAllowZeroWeightLeaf(true), 
                   doCountRules(false), doCheckConsistency(false) {}
  AzTrTreeFeat(const AzTrTreeFeat *inp) : 
                   doAllowZeroWeightLeaf(true), 
                   doCountRules(false), doCheckConsistency(false)
  {
    reset(inp); 
  }
  ~AzTrTreeFeat() {}
  void reset(const AzTrTreeFeat *inp); 

  void reset(const AzDataForTrTree *data, 
             AzParam &param, 
             const AzOut &out_req, 
             bool inp_doAllowZeroWeightLeaf=true); 
  inline int featNum() const {
    return f_inf.cursor(); 
  }

  int update_with_new_tree(const AzTrTree_ReadOnly *dtree, 
                           int tx); 
  void concatDesc(int ex, AzBytArr *str_desc) const; 

  inline int treeNum() const {
    return ip_featDef.size(); 
  }
  void getWeight(const AzTrTreeEnsemble_ReadOnly *ens, 
                 AzDvect *v_w) const; /* output */

  int countNonzeroNodup(const AzDvect *v_w, 
                        const AzTrTreeEnsemble_ReadOnly *ens) const; 

  int countNonzeroNodup(const AzTrTreeEnsemble_ReadOnly *ens) const; 

  int update_with_ens(const AzTrTreeEnsemble_ReadOnly *ens, 
                      AzIntArr *ia_rmved_fx); /* removed featuers */

  void checkConsistency(const AzTrTreeEnsemble_ReadOnly *ens) const; 
  void consolidateInternalWeights(const AzDvect *inp_v_weight, 
                                  const AzTrTreeEnsemble_ReadOnly *ens, 
                                  AzDvect *v_weight) const; /* output */

  void updateMatrix(const AzDataForTrTree *data, 
                  const AzTrTreeEnsemble_ReadOnly *ens, 
                  AzBmat *m_tran) /* inout */
                  const; 

  const AzTrTreeFeatInfo *featInfo(int fx) const {
    return f_inf.point(fx); 
  }

  void printHelp(AzHelp &h) const; 
  void dump(const AzOut &out, const char *header) const; 

  void featIds(int tx,  
              AzIIarr *iia_nx_fx) /* output: (node#, feat#) */
  const; 

protected:
  void updateRulePools(); 
  void genFeats(const AzTrTree_ReadOnly *dtree, 
                int tx, /* tree# of dtree */
                const AzDataForTrTree *data, 
                int dx, 
                int fx_offs, 
                /*---  output  ---*/
                AzDataArray<AzIntArr> *aia_fx_dx) const; 

  int _update(const AzTrTree_ReadOnly *dtree, 
                      int tx, 
                      AzIntArr *ia_isActiveNode, /* output */
                     /*---  for the tree already in the system  ---*/ 
                     const int *featDef=NULL, 
                     int featDef_len=0, 
                     AzIntArr *ia_rmv_fx=NULL); 

  void _getWeight(const AzTrTree_ReadOnly *dtree, 
                 int tx, 
                 AzDvect *v_w) const; /* output */

  void _updateMatrix(const AzDataForTrTree *data, 
                     const AzTrTreeEnsemble_ReadOnly *ens, 
                     int old_f_num, 
                     /*---  output  ---*/
                     AzBmat *m_tran) const; 

  void removeFeat(int removed_fx); 
  int countNonzeroNodup(const AzDvect *v_w) const; 

  int _update_with_new_trees(int old_t_num, 
                  const AzTrTreeEnsemble_ReadOnly *ens); 
  int _update_with_existing_trees(int old_t_num, 
                   const AzTrTreeEnsemble_ReadOnly *ens, 
                   AzIntArr *ia_rmv_fx); /* output */

  bool resetParam(AzParam &param); 
  void printParam(const AzOut &out) const; 
}; 
#endif 
