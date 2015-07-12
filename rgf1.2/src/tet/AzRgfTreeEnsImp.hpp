/* * * * *
 *  AzRgfTreeEnsImp.hpp 
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

#ifndef _AZ_RGF_TREE_ENS_IMP_HPP_
#define _AZ_RGF_TREE_ENS_IMP_HPP_

#include "AzRgfTreeEnsemble.hpp"
#include "AzTrTreeEnsemble.hpp"

//! implement AzRgfTreeEnsemble.  T must be AzRgfTree or its extension.  
template<class T>
class AzRgfTreeEnsImp : /*implements */public virtual AzRgfTreeEnsemble
{
protected:
  AzTrTreeEnsemble<T> ens; 

public:
  AzRgfTreeEnsImp() {}
  ~AzRgfTreeEnsImp() {}
  inline bool usingTempFile() const {
    return ens.usingTempFile(); 
  }
  inline void reset() {
    ens.reset(); 
  }
  inline const char *param_c_str() const {
    return ens.param_c_str(); 
  }

  inline double constant() const {
    return ens.constant(); 
  }
  inline int orgdim() const {
    return ens.orgdim(); 
  }
  inline void set_constant(double inp) {
    ens.set_constant(inp); 
  }
  inline AzRgfTree *new_tree(int *out_tx=NULL) {
    return ens.new_tree(out_tx);     
  }

  inline const AzRgfTree *tree(int tx) const {
    return ens.tree(tx);  
  }
  inline AzRgfTree *tree_u(int tx) const {
    return ens.tree_u(tx); 
  }

  inline T *rawtree_u(int tx) const {
    return ens.tree_u(tx); 
  }

  inline int leafNum() const {
    return ens.leafNum(); 
  }
  inline int leafNum(int tx0, int tx1) const {
    return ens.leafNum(tx0, tx1); 
  }

  inline int lastIndex() const {
    return ens.lastIndex(); 
  }
  inline int nextIndex() const { /* next slot */
    return ens.nextIndex(); 
  }

  inline int size() const { 
    return ens.size(); 
  } 
  inline int max_size() const {
    return ens.max_size(); 
  }
  inline bool isFull() const {
    return ens.isFull(); 
  }
  inline void printHelp(AzHelp &h) const {
    ens.printHelp(h); 
  }
  inline void copy_to(AzTreeEnsemble *out_ens, 
                      const char *config, const char *sign) const {
    ens.copy_to(out_ens, config, sign); 
  }
  inline void copy_nodes_from(const AzTrTreeEnsemble_ReadOnly *inp) {
    ens.copy_nodes_from(inp); 
  }
  inline void show(const AzSvFeatInfo *feat, 
                   const AzOut &out, const char *header="") const {
    ens.show(feat, out, header); 
  }

  inline virtual void cold_start(AzParam &param, 
                          const AzBytArr *s_temp_prefix, 
                          int data_num, 
                          const AzOut &out, 
                          int tree_num_max, 
                          int inp_org_dim) {
    ens.cold_start(param, s_temp_prefix, data_num, 
                   out, tree_num_max, inp_org_dim); 
  }
  inline virtual void warm_start(const AzTreeEnsemble *inp_ens, 
              const AzDataForTrTree *data, 
              AzParam &param,  
              const AzBytArr *s_temp_prefix, 
              const AzOut &out,                       
              int max_t_num, 
              int search_t_num, /* to release work areas for the fixed trees */
              AzDvect *v_p, /* inout */
              const AzIntArr *inp_ia_tr_dx=NULL) {
    ens.warm_start(inp_ens, data, param, s_temp_prefix, out, max_t_num, search_t_num, 
                   v_p, inp_ia_tr_dx); 
  }
}; 
#endif 
