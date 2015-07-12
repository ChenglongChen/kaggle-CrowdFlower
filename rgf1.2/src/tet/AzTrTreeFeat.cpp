/* * * * *
 *  AzTrTreeFeat.cpp 
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

#include "AzTrTreeFeat.hpp"
#include "AzHelp.hpp"
#include "AzRgf_kw.hpp"

/*------------------------------------------------------------------*/
void AzTrTreeFeat::reset(const AzDataForTrTree *data, 
                         AzParam &param, 
                         const AzOut &out_req, 
                         bool inp_doAllowZeroWeightLeaf)
{
  out = out_req; 
  org_featInfo.reset(data->featInfo()); 
  ip_featDef.reset(); 
  int init_size=10000, avg_len=32; 
  pool_rules.reset(init_size, avg_len); 
  pool_rules_rmved.reset(init_size, avg_len); 
  sp_desc.reset(init_size, avg_len); 
  f_inf.reset(); 
  doAllowZeroWeightLeaf = inp_doAllowZeroWeightLeaf; 

  bool beVerbose = resetParam(param); 
  printParam(out); 

  if (!beVerbose) {
    out.deactivate(); 
  }
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::reset(const AzTrTreeFeat *inp)
{
  doAllowZeroWeightLeaf = inp->doAllowZeroWeightLeaf; 

  org_featInfo.reset((AzSvFeatInfo *)&inp->org_featInfo); 
  ip_featDef.reset(&inp->ip_featDef); 
  sp_desc.reset(&inp->sp_desc); 
  f_inf.reset(&inp->f_inf); 
  pool_rules.reset(&inp->pool_rules); 
  pool_rules_rmved.reset(&inp->pool_rules_rmved); 

  doCountRules = inp->doCountRules; 
  doCheckConsistency = inp->doCheckConsistency; 
  out = inp->out; 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::featIds(int tx, 
                          AzIIarr *iia_nx_fx) const 
{
  const char *eyec = "AzTrTreeFeat::featIds"; 
  if (tx < 0 || tx >= ip_featDef.size()) {
    throw new AzException(eyec, "tx is out of range"); 
  }
  int len; 
  const int *def = ip_featDef.point(tx, &len); 
  int nx; 
  for (nx = 0; nx < len; ++nx) {
    if (def[nx] >= 0) {
      iia_nx_fx->put(nx, def[nx]); 
    }
  } 
}

/*! features are only added  -- never removed here. */
/*------------------------------------------------------------------*/
int AzTrTreeFeat::update_with_new_tree(const AzTrTree_ReadOnly *dtree, 
                                       int tx)
{
  const char *eyec = "AzTrTreeFeat::update"; 
  int old_t_num = ip_featDef.size(); 
  if (tx != old_t_num) {
    throw new AzException(eyec, "tree# conflict"); 
  }

  AzIntArr ia_isActiveNode; 
  int added_feat_num = _update(dtree, tx, &ia_isActiveNode); 
  ip_featDef.put(&ia_isActiveNode); 
  if (ip_featDef.size()-1 != tx) {
    throw new AzException(eyec, "ip_featDef conflict"); 
  }

  updateRulePools(); 

  return added_feat_num; 
}

/*! features may be removed and added */
/*------------------------------------------------------------------*/
int AzTrTreeFeat::update_with_ens(const AzTrTreeEnsemble_ReadOnly *ens, 
                                  AzIntArr *ia_rmv_fx) /* output */
{
  int old_t_num = ip_featDef.size(); 
  int added_from_new = 0, added_from_old = 0; 
  added_from_old = _update_with_existing_trees(old_t_num, ens, ia_rmv_fx); 
  added_from_new = _update_with_new_trees(old_t_num, ens);  

  updateRulePools(); 

  if (!out.isNull()) {
    AzPrint o(out); 
    o.printBegin("", ",", ","); 
    o.print("#tree", treeNum()); 
    o.print("#added(from existing trees)", added_from_old); 
    o.print("#added(from new trees)", added_from_new); 
    o.print("#removed", ia_rmv_fx->size()); 
    o.printEnd(); 
  }

  if (doCheckConsistency) {
    checkConsistency(ens); 
  }

  return added_from_new+added_from_old; 
}

/*------------------------------------------------------------------*/
int AzTrTreeFeat::_update_with_new_trees(int old_t_num, 
                  const AzTrTreeEnsemble_ReadOnly *ens)
{
  const char *eyec = "AzTrTreeFeat::_update_with_new_trees"; 
  if (old_t_num != treeNum()) {
    throw new AzException(eyec, "tree# conflict1"); 
  }

  /*---  add those from new trees  ---*/
  int added_f_num1 = 0; 
  int tree_num = ens->size(); 
  int tx; 
  for (tx = old_t_num; tx < tree_num; ++tx) {
    AzIntArr ia_isActiveNode; 
    added_f_num1 += _update(ens->tree(tx), tx, &ia_isActiveNode); 
    ip_featDef.put(&ia_isActiveNode); 
  }

  /*---  just checking ... ---*/
  if (treeNum() != tree_num) {
    throw new AzException(eyec, "tree# conflict2"); 
  }
  return added_f_num1; 
}

/*------------------------------------------------------------------*/
int AzTrTreeFeat::_update_with_existing_trees(int old_t_num, 
                   const AzTrTreeEnsemble_ReadOnly *ens, 
                   AzIntArr *ia_rmv_fx) /* output */
{
  /*---  add features from existing trees  ---*/
  int added_f_num2 = 0; 
  int tx; 
  for (tx = 0; tx < old_t_num; ++tx) {
    int featDef_len; 
    const int *featDef = ip_featDef.point(tx, &featDef_len);     
    if (ens->tree(tx)->nodeNum() == featDef_len) {
      continue; /* no change */
    }

    AzIntArr ia_new_featDef;    
    added_f_num2 += _update(ens->tree(tx), tx, &ia_new_featDef, 
                            featDef, featDef_len, 
                            ia_rmv_fx); 
    ip_featDef.update(tx, &ia_new_featDef); 
  }
  return added_f_num2; 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::_updateMatrix(const AzDataForTrTree *data, 
                          const AzTrTreeEnsemble_ReadOnly *ens, 
                          int old_f_num, 
                          /*---  output  ---*/
                          AzBmat *b_tran) const
{
  int data_num = data->dataNum(); 
  int f_num = featNum(); 
  if (old_f_num == 0) {
    b_tran->reform(data_num, f_num); 
  }
  else {
    if (b_tran->rowNum() != data_num || 
        b_tran->colNum() != old_f_num) {
      throw new AzException("AzTrTreeFeat::_updateMatrix", 
                            "b_tran has a wrong shape"); 
    }
    b_tran->resize(f_num); 
  }

  /*---  which trees are referred in the new features?  ---*/
  AzIntArr ia_tx; 
  int fx; 
  for (fx = old_f_num; fx < f_num; ++fx) {
    ia_tx.put(f_inf.point(fx)->tx); 
  }
  ia_tx.unique(); /* remove duplication */
  int tx_num; 
  const int *txs = ia_tx.point(&tx_num); 

  /*---  generate features  ---*/
  AzDataArray<AzIntArr> aia_fx_dx(f_num-old_f_num); 
  int xx; 
  for (xx = 0; xx < tx_num; ++xx) {
    int tx = txs[xx]; 
    int dx; 
    for (dx = 0; dx < data_num; ++dx) {
      genFeats(ens->tree(tx), tx, data, dx, 
               old_f_num, &aia_fx_dx); 
    }
  }

  /*---  load into the matrix  ---*/
  for (fx = old_f_num; fx < f_num; ++fx) {
    b_tran->load(fx, aia_fx_dx.point(fx-old_f_num)); 
  }  
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::genFeats(const AzTrTree_ReadOnly *dtree, 
                        int tx, 
                        const AzDataForTrTree *data, 
                        int dx, 
                        int fx_offs, 
                        /*---  output  ---*/
                        AzDataArray<AzIntArr> *aia_fx_dx)
const
{
  AzIntArr ia_nx; 
  dtree->apply(data, dx, &ia_nx); 

  int num; 
  const int *nx = ia_nx.point(&num); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int feat_no = (ip_featDef.point(tx))[nx[ix]]; 
    if (feat_no >= fx_offs) {     
      aia_fx_dx->point_u(feat_no-fx_offs)->put(dx); 
    }
  }
}

/* can be used for both frontier and non-frontier */
/*------------------------------------------------------------------*/
void AzTrTreeFeat::updateMatrix(const AzDataForTrTree *data, 
                                const AzTrTreeEnsemble_ReadOnly *ens, 
                                AzBmat *b_tran) /* inout */
                                const
{
  if (data == NULL) return; 

  const char *eyec = "AzTrTreeFeat::updateMatrix"; 
  int f_num = featNum(); 
  if (ens->size() != treeNum()) {
    throw new AzException(eyec, "size of tree ensemble and #feat should be the same"); 
  }
  int old_f_num = b_tran->colNum(); 
  if (old_f_num > f_num) {
    throw new AzException(eyec, "#col is bigger than #feat"); 
  }
  int fx; 
  for (fx = 0; fx < old_f_num; ++fx) {
    if (f_inf.point(fx)->isRemoved) {
      b_tran->clear(fx); 
    }
  }

  if (old_f_num == f_num) {
    if (old_f_num == 0) {
      /* for the rare case that no feature was generated */
      b_tran->reform(data->dataNum(), f_num);  /* added 9/13/2011 */
    }
    return; 
  }

  _updateMatrix(data, ens, old_f_num, b_tran); 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::removeFeat(int removed_fx)
{
  AzTrTreeFeatInfo *fp = f_inf.point_u(removed_fx); 
  if (fp->isRemoved) {
    throw new AzException("AzTrTreeFeat::removeFeat", "want to remove a removed feature??"); 
  }
  fp->isRemoved = true; 
  if (doCountRules) {
    pool_rules_rmved.put(&fp->rule); 
  }
}
/*------------------------------------------------------------------*/
int AzTrTreeFeat::_update(const AzTrTree_ReadOnly *dtree, 
                      int tx, 
                      AzIntArr *ia_isActiveNode, /* output */
                      /*---  for updating old feature definition  ---*/
                      const int *prevDef, int prevDef_len, 
                      AzIntArr *ia_rmv_fx) 
{
  int added_feat_num = 0, rmv_num = 0; 
  dtree->isActiveNode(doAllowZeroWeightLeaf, ia_isActiveNode); 
  int node_num; 
  int *isActive = ia_isActiveNode->point_u(&node_num); 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!isActive[nx]) {
      isActive[nx] = AzNone; 
      if (prevDef != NULL && 
          nx < prevDef_len && 
          prevDef[nx] >= 0) {
        /*---  this was a feature before, but not any more  ---*/
        /* dtree->setFeatNo(nx, AzNone); */
        removeFeat(prevDef[nx]);  
        if (ia_rmv_fx != NULL) {
          ia_rmv_fx->put(prevDef[nx]); 
        }
        ++rmv_num;  
      }
      continue; 
    }

    /*---  active node  ---*/
    if (prevDef != NULL && 
        nx < prevDef_len && 
        prevDef[nx] >= 0) {
      /*---  this is already defined  ---*/
      isActive[nx] = prevDef[nx];  /* set feat# as set before */
      continue; 
    }

    /*---  we need to assign new feat#  ---*/
    AzTreeRule rule; 
    dtree->getRule(nx, &rule); 

    int feat_no; 
    AzTrTreeFeatInfo *fp = f_inf.new_slot(&feat_no);  
    fp->tx = tx; 
    fp->nx = nx; 
    fp->rule.reset(rule.bytarr()); 

    if (doCountRules) {
      pool_rules.put(rule.bytarr()); 
    }
    isActive[nx] = feat_no; 

    ++added_feat_num; 

    /*---  only for print purpose  ---*/
    if (!dmp_out.isNull()) {
      AzBytArr str_desc; 
      int max_len = 256; 
      dtree->concatDesc(&org_featInfo, nx, &str_desc, max_len); 
      sp_desc.put(&str_desc); 
    } 
  }

  return added_feat_num; 
}

/* -----------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
void AzTrTreeFeat::concatDesc(int ex, AzBytArr *str_desc) const
{
  str_desc->inBrackets(ex); 
  if (sp_desc.size() > 0) {
    if (ex >= 0 && ex < sp_desc.size()) {
      sp_desc.get(ex, str_desc); 
    }
    else {
      str_desc->concat("?!?"); 
    }
  }
  else {
    str_desc->concat("tf"); 
    str_desc->concatInt(ex); 
  }
}

//! for maintenance purpose only 
/*------------------------------------------------------------------*/
void AzTrTreeFeat::checkConsistency(const AzTrTreeEnsemble_ReadOnly *ens) const
{
  const char *eyec = "AzTrTreeFeat::checkConsistency"; 

  int dtree_num = ens->size(); 
  if (ip_featDef.size() != dtree_num) {
    throw new AzException(eyec, "#tree conflict"); 
  }
  int f_num = featNum(); 
  if (sp_desc.size() > 0 && sp_desc.size() != f_num) {
    throw new AzException(eyec, "#feat conflict"); 
  }

  int tx; 
  for (tx = 0; tx < dtree_num; ++tx) {
    int num; 
    const int *feat_no = ip_featDef.point(tx, &num); 
    if (num != ens->tree(tx)->nodeNum()) {
      throw new AzException(eyec, "#node conflict"); 
    }
    int nx; 
    for (nx = 0; nx < num; ++nx) {
      /*---  ip_featDef -> dtree[]  ---*/
      double tree_w = ens->tree(tx)->node(nx)->weight; 
      if (tree_w != 0 && feat_no[nx] < 0) {
        throw new AzException(eyec, "non-zero weight in the tree for non-feature?"); 
      }

      /*---  ip_featDef -> f_inf  ---*/
      if (feat_no[nx] >= 0) {
        const AzTrTreeFeatInfo *fp = f_inf.point(feat_no[nx]); 
        if (fp->isRemoved) {
          throw new AzException(eyec, "a removed feature is active in ip_featDef"); 
        }
        if (fp->tx != tx || fp->nx != nx) {
          throw new AzException(eyec, "conflict between f_inf and ip_featDef"); 
        }
      }
    }
  }

  /*---  f_inf -> ip_featDef  ---*/
  int fx; 
  for (fx = 0; fx < f_num; ++fx) {
    const AzTrTreeFeatInfo *fp = f_inf.point(fx); 
    if (fp->isRemoved) continue; 
    
    if (fp->tx < 0 || fp->tx >= dtree_num) {
      throw new AzException(eyec, "f_inf is pointing non-existing tree"); 
    }
    int num; 
    const int *feat_no = ip_featDef.point(fp->tx, &num);     
    if (fp->nx < 0 || fp->nx >= num) {
      throw new AzException(eyec, "f_inf is pointing non-existing node");       
    }
    if (feat_no[fp->nx] != fx) {
      throw new AzException(eyec, "conflict in feat# between f_inf and ip_featDef"); 
    }
  }
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::consolidateInternalWeights(const AzDvect *v_inp_w, 
                                          const AzTrTreeEnsemble_ReadOnly *ens, 
                                          AzDvect *v_w) /* output */
                                          const
{
  int f_num = featNum();  /* #features */
  v_w->reform(f_num); 

  const double *inp_w = v_inp_w->point(); 
  double *w = v_w->point_u(); 

  int dtree_num = ens->size(); 

  int fx; 
  for (fx = 0; fx < f_num; ++fx) {
    const AzTrTreeFeatInfo *fp = f_inf.point(fx); 
    if (fp->isRemoved) continue; 
    if (fp->tx < 0 || fp->tx >= dtree_num) {
      throw new AzException("AzTrTreeFeat::consolidateInternalWeights", "tree# conflict"); 
    }
    const AzTrTree_ReadOnly *my_tree = ens->tree(fp->tx); 
    int nx = fp->nx; 
    if (my_tree->node(nx)->isLeaf()) {
      int num; 
      const int *feat_no = ip_featDef.point(fp->tx, &num); 
      for ( ; ; ) {
        if (nx < 0) break; 
        if (feat_no[nx] >= 0) {
          w[fx] += inp_w[feat_no[nx]]; 
        }
        nx = my_tree->node(nx)->parent_nx; 
      }
    }
  }
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::updateRulePools()
{
  const char *eyec = "AzTrTreeFeat::updateRulePools"; 

  if (!doCountRules) return; 
  if (pool_rules.isThisCommitted() && 
      pool_rules_rmved.isThisCommitted()) {
    return; 
  }

  pool_rules.commit(); 
  pool_rules_rmved.commit(); 
  int rmved = pool_rules_rmved.size(); 
  int rx; 
  for (rx = 0; rx < rmved; ++rx) { /* for every removed rule */
    int len; 
    const AzByte *bytes = pool_rules_rmved.point(rx, &len); 
    int idx = pool_rules.find(bytes, len); 
    if (idx < 0) {
      throw new AzException(eyec, "rule doesn't exist??"); 
    }

    AZint8 nega = pool_rules_rmved.getCount(rx); /* how many times removed */
    AZint8 posi = pool_rules.getCount(idx);  /* how many times added */
    AZint8 new_count = posi - nega; 
    if (new_count == 0) {
      pool_rules.removeEntry(idx); 
    }
    else if (new_count < 0) {
      throw new AzException(eyec, "#added < #removed?"); 
    }
    else {
      pool_rules.setCount(idx, new_count); 
    }
  }

  pool_rules_rmved.reset(); 
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
int AzTrTreeFeat::countNonzeroNodup(const AzDvect *v_w, 
                                    const AzTrTreeEnsemble_ReadOnly *ens) const 
{
  if (ens == NULL || ens->size() <= 0) {
    return 0; 
  }
  if (ens->tree(0)->usingInternalNodes()) {
    AzDvect my_v_w; 
    consolidateInternalWeights(v_w, ens, &my_v_w); 
    return countNonzeroNodup(&my_v_w); 
  }
  else {
    return countNonzeroNodup(v_w);    
  }
}

//! count non-zero featuers without duplication. 
/*------------------------------------------------------------------*/
int AzTrTreeFeat::countNonzeroNodup(const AzDvect *v_w) const
{
  const char *eyec = "AzTrTreeFeat::countNonzeroNodup"; 

  if (!doCountRules) {
    return -1; 
  }

  if (v_w->rowNum() != featNum()) {
    throw new AzException(eyec, "#feat conflict"); 
  }

  AzIFarr ifa_rx_zerocount; 

  int f_num = v_w->rowNum(); 
  const double *w = v_w->point(); 
  int fx; 
  for (fx = 0; fx < f_num; ++fx) {
    const AzTrTreeFeatInfo *fp = f_inf.point(fx); 
    if (fp->isRemoved) {
      continue; 
    }
    if (w[fx] == 0) {
      int rule_idx = pool_rules.find(&fp->rule); 
      if (rule_idx < 0) {
        throw new AzException(eyec, "rule not found in the pool?!"); 
      }
      ifa_rx_zerocount.put(rule_idx, 1); 
    }
  }

  int nz_f_num = pool_rules.size(); 
  ifa_rx_zerocount.squeeze_Sum(); 
  int num = ifa_rx_zerocount.size(); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int rule_idx; 
    int zero_count = (int)ifa_rx_zerocount.get(ix, &rule_idx); 
    if (zero_count >= pool_rules.getCount(rule_idx)) {
      --nz_f_num; 
    }
  }
  return nz_f_num; 
}

/*------------------------------------------------------------------*/
int AzTrTreeFeat::countNonzeroNodup(const AzTrTreeEnsemble_ReadOnly *ens) const
{
  AzDvect v_w; 
  getWeight(ens, &v_w); 
  return countNonzeroNodup(&v_w); 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::getWeight(const AzTrTreeEnsemble_ReadOnly *ens, 
                         AzDvect *v_w) const /* output */
{
  const char *eyec = "AzTrTreeFeat::getWeight"; 
  int dtree_num = ens->size();
  if (dtree_num != treeNum()) {
    throw new AzException(eyec, "#trees conflict"); 
  }
  int feat_num = featNum(); 
  v_w->reform(feat_num); 

  int tx; 
  for (tx = 0; tx < dtree_num; ++tx) {
    _getWeight(ens->tree(tx), tx, v_w);    
  }
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::_getWeight(const AzTrTree_ReadOnly *dtree, 
                         int tx, 
                         AzDvect *v_w) /* output: must be initialized by caller */
                         const 
{
  int num; 
  const int *featNo = ip_featDef.point(tx, &num); 
  if (num != dtree->nodeNum()) {
    throw new AzException("AzTrTreeFeat::_getWeight", "#nodes conflict"); 
  }
  int nx; 
  for (nx = 0; nx < num; ++nx) {
    if (featNo[nx] >= 0) {
      double w = dtree->node(nx)->weight; 
      v_w->set(featNo[nx], w); 
    }
  }   
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::dump(const AzOut &out, const char *header) const
{
  if (out.isNull()) return; 
 
  AzPrint o(out); 
  o.printBegin(header); 
  o.printSw("doAllowZeroWeightLeaf", doAllowZeroWeightLeaf); 
  o.printSw("doCountRules", doCountRules); 
  o.printSw("doCheckConsistency", doCheckConsistency); 
  o.printEnd(); 
  ip_featDef.dump(dmp_out, "ip_featDef"); 
  
  int f_num = featNum(); 
  int fx; 
  for (fx = 0; fx < f_num; ++fx) {
    const AzTrTreeFeatInfo *fp = f_inf.point(fx); 
    
    o.printBegin("", ", ", "="); 
    o.inBrackets(fx); 
    o.print("isRemoved", fp->isRemoved); 
    o.print("tx", fp->tx); 
    o.print("nx", fp->nx); 
    o.print("rule length", fp->rule.length()); 
    o.printEnd(); 
  }
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
bool AzTrTreeFeat::resetParam(AzParam &p)
{
  p.swOn(&doCountRules, kw_doCountRules); 
  p.swOn(&doCheckConsistency, kw_doCheckConsistency); 
  bool beVerbose = false; 
  p.swOn(&beVerbose, kw_opt_beVerbose); 
  return beVerbose; 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::printParam(const AzOut &out) const
{
  if (out.isNull()) return; 

  if (!doCountRules && !doCheckConsistency) {
    return; 
  }

  AzPrint o(out); 
  o.ppBegin("AzTrTreeFeat", "Feature management", ", "); 
  o.printSw(kw_doCountRules, doCountRules); 
  o.printSw(kw_doCheckConsistency, doCheckConsistency); 
  o.ppEnd(); 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::printHelp(AzHelp &h) const
{
  h.begin(Azforest_config, "AzTrTreeFeat"); 
  h.item_experimental(kw_doCountRules, help_doCountRules); 
  h.item_experimental(kw_doCheckConsistency, help_doCheckConsistency); 
  h.end(); 
}



