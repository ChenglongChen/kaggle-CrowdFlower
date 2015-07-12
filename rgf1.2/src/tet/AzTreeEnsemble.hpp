/* * * * *
 *  AzTreeEnsemble.hpp 
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

#ifndef _AZ_TREE_ENSEMBLE_HPP_
#define _AZ_TREE_ENSEMBLE_HPP_

#include "AzUtil.hpp"
#include "AzTree.hpp"
#include "AzDmat.hpp"
#include "AzTE_ModelInfo.hpp"

//! Untrainable tree ensemble.  Like applier.  Generated as a result of training.  
class AzTreeEnsemble 
{
protected:
  AzObjPtrArray<AzTree> a_tree; 
  AzTree **t; 
  int t_num;   
  AzTree empty_tree; 
  double const_val; 

  AzBytArr s_config, s_sign; 
  int org_dim; /* dimension of original features */

public:
  AzTreeEnsemble() : t(NULL), t_num(0), const_val(0), org_dim(-1) {}
  ~AzTreeEnsemble() {}

  AzTreeEnsemble(const char *fn)
                   : t(NULL), t_num(0), const_val(0), org_dim(-1) {
    read(fn); 
  }
  AzTreeEnsemble(AzFile *file) 
                   : t(NULL), t_num(0), const_val(0), org_dim(-1) {
    _read(file); 
  }

  void transfer_from(AzTree *inp_tree[], /* destroys input */
                     int inp_tree_num, 
                     double const_val, 
                     int orgdim, 
                     const char *config, 
                     const char *sign); 

  void read(const char *fn); 
  void read(AzFile *file) {
    _release(); 
    _read(file); 
  }
  void write(const char *fn); 
  void write(AzFile *file); 

  inline void destroy() {
    _release(); 
  }

  inline const AzTree *tree(int tx) const {
    checkIndex(tx, "tree"); 
    if (t[tx] == NULL) {
      return &empty_tree; 
    }
    return t[tx]; 
  }

  int leafNum() const {
    return leafNum(0, t_num); 
  }
  int leafNum(int tx0, int tx1) const; 
  inline int size() const { return t_num; } 

  void apply(const AzSmat *m_data, 
             AzDvect *v_pred) /* output */
             const; 
  double apply(const AzSvect *v_data) const; 

  inline double constant() const { return const_val; }
  inline int orgdim() const { return org_dim; }
  const char *signature() const { return s_sign.c_str(); }
  const char *configuration() const { return s_config.c_str(); }

  /*---*/
  void info(AzTE_ModelInfo *out_info) const; 

  void show(const AzSvFeatInfo *feat, //!< may be NULL 
            const AzOut &out, const char *header="") const; 
  void finfo(AzIFarr *ifa_fx_count, 
             AzIFarr *ifa_fx_sum) const {
    finfo(0, t_num, ifa_fx_count, ifa_fx_sum); 
  }
  void finfo(int tx0, int tx1, 
             AzIFarr *ifa_fx_count, 
             AzIFarr *ifa_fx_sum) const; 
  void finfo(AzIntArr *ia_fx2tx) const; 
  void cooccurrences(AzIIFarr *iifa_fx1_fx2_count) const; 

  void show_weights(const AzOut &out, AzSvFeatInfo *fi) const; 

protected:
  void _read(AzFile *file); 
  inline void _release() {
    a_tree.free(&t); t_num = 0; 
    s_config.reset(); 
    s_sign.reset(); 
    const_val = 0; 
    org_dim = -1; 
  }
  inline void checkIndex(int tx, const char *msg) const {
    if (tx < 0 || tx >= t_num) {
      throw new AzException("AzTreeEnsemble::checkIndex", msg); 
    }
  }
  void clean_up(); 
}; 
#endif 

