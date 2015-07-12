/* * * * *
 *  AzTrTreeEnsemble.hpp 
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

#ifndef _AZ_TR_TREE_ENSEMBLE_HPP_
#define _AZ_TR_TREE_ENSEMBLE_HPP_

#include "AzUtil.hpp"
#include "AzTrTreeEnsemble_ReadOnly.hpp"
#include "AzTreeEnsemble.hpp"
#include "AzParam.hpp"
#include "AzHelp.hpp"

/*---------------------------------------------------------------*/
template<class T>
class AzTemp_forTrTreeEns {
protected: 
  AzBytArr s_temp_prefix; 
  AzDataPool<AzFile> pool_file; 
  int unit_size; 
  static const int max_size = 2000000000; 

public:
  AzTemp_forTrTreeEns() : unit_size(-1) {}

  bool isActive() const {
    if (unit_size > 0) return true; 
    return false; 
  }
  void reset() {
    unit_size = -1; 
    s_temp_prefix.reset();
    pool_file.reset(); 
  }
  void reset(T *tree, 
             int data_num, 
             const AzBytArr *inp_s_temp_prefix) {
    reset(); 
    if (inp_s_temp_prefix == NULL || inp_s_temp_prefix->length() <= 0) {
      return; 
    }
    s_temp_prefix.reset(inp_s_temp_prefix); 
    unit_size = tree->estimateSizeofDataIndexes(data_num); 
    if (unit_size <= 0) {
      return; 
    }
    if (unit_size > max_size) {
      throw new AzException(AzInputError, "AzTemp_forTrTreeEns::reset", 
                            "Data size is too large."); 
    }
    open_new_file(); 
  }
  AzFile *point_file() {
    if (!isActive()) return NULL; 
    int fx = pool_file.size() - 1; 
    if (fx < 0) {
      throw new AzException("AzTemp_forTrTreeEns", "The temporary file is not ready"); 
    }
    AzFile *file = pool_file.point_u(fx); 
    int remain = max_size - file->size_under2G("AzTrTreeEnsemble::point_file"); 
    if (remain < unit_size) {
      file = open_new_file(); 
    }
    return file; 
  }
protected:
  AzFile *open_new_file() {
    int idx; 
    AzFile *file = pool_file.new_slot(&idx); 
    AzBytArr s_fn; 
    s_fn.reset(&s_temp_prefix); s_fn.c("--"); s_fn.cn(idx, 2, true); s_fn.c("--"); 
    file->reset(s_fn.c_str()); 
    file->open("w+b"); 
    return file; 
  }
}; 

/*---------------------------------------------------------------*/
//! Ensemble of trainable trees.  
template<class T>
class AzTrTreeEnsemble : /*implements */public virtual AzTrTreeEnsemble_ReadOnly 
{
protected:
  AzObjPtrArray<T> a_tree; 
  T **t; 
  int t_num;  
  double const_val; 
  int org_dim; 

  AzBytArr s_param; 
  const char *dt_param; 

  AzTemp_forTrTreeEns<T> temp_files; 

public:
  AzTrTreeEnsemble() : t(NULL), t_num(0), const_val(0), org_dim(-1), dt_param("") {}

  inline bool usingTempFile() const {
    return temp_files.isActive(); 
  }

  inline void reset() {
    a_tree.free(&t); t_num = 0; 
    const_val = 0; 
    org_dim = -1; 
    s_param.reset(); 
    dt_param = ""; 
    temp_files.reset(); 
  }
  inline void cold_start(
                    AzParam &param, 
                    const AzBytArr *s_temp_prefix, /* may be NULL */
                    int data_num, /* to estimate the data size for temp */
                    const AzOut &out, 
                    int tree_num_max, 
                    int inp_org_dim) {
    reset(); 
    T dummy_tree(param); 
    dummy_tree.printParam(out); 
    s_param.reset(param.c_str()); 
    dt_param = s_param.c_str(); 
    alloc(tree_num_max, "AzTrTreeEnsemble::reset"); 
    org_dim = inp_org_dim; 

    temp_files.reset(&dummy_tree, data_num, s_temp_prefix); 
  }

  inline const char *param_c_str() const {
    return s_param.c_str(); 
  }

  inline double constant() const {
    return const_val; 
  }
  inline int orgdim() const {
    return org_dim; 
  }
  inline void set_constant(double inp) {
    const_val = inp; 
  }
  inline void set_orgdim(int inp) {
    org_dim = inp; 
  }

  virtual 
  T *new_tree(int *out_tx=NULL) {
    if (t_num >= a_tree.size()) {
      throw new AzException("AzTrTreeEnsemble::new_tree", "no more empty slot"); 
    }
    int tx = t_num; 
    if (t[tx] != NULL) {
      throw new AzException("AzTrTreeEnsemble::new_tree", "something is wrong"); 
    }
    AzParam p(dt_param, false); 
    t[tx] = new T(p); 
    t[tx]->forStoringDataIndexes(temp_files.point_file()); 
    ++t_num; 
    if (out_tx != NULL) {
      *out_tx = tx; 
    }
    return t[tx]; 
  }

  virtual 
  inline const T *tree(int tx) const {
    checkIndex(tx, "tree"); 
    if (t[tx] == NULL) {
      throw new AzException("AzTreeEnesemble::tree", "there is no tree"); 
    }
    return t[tx]; 
  }

  virtual 
  inline T *tree_u(int tx) const {
    checkIndex(tx, "tree_u"); 
    if (t[tx] == NULL) {
      throw new AzException("AzTrTreeEnsemble::tree_u", "there is no tree"); 
    }
    return t[tx]; 
  }

  int leafNum() const {
    return leafNum(0, t_num); 
  }
  int leafNum(int tx0, int tx1) const {
    if (tx0 < 0 || tx1 > t_num) {
      throw new AzException("AzTrTreeEnsemble::leafNum", "out of range"); 
    }
    int l_num = 0; 
    int tx; 
    for (tx = tx0; tx < tx1; ++tx) {
      l_num += t[tx]->leafNum(); 
    }
    return l_num; 
  }

  inline int lastIndex() const {
    return t_num - 1; 
  }
  inline int nextIndex() const { /* next slot */
    return t_num; 
  }

  inline int size() const { return t_num; } 
  inline int max_size() const { return a_tree.size(); }

  inline bool isFull() const {
    if (t_num >= a_tree.size()) {
      return true; 
    }
    return false; 
  }

  void printHelp(AzHelp &h) const {
    T dummy_tree; 
    dummy_tree.printHelp(h); 
  }

  void copy_to(AzTreeEnsemble *out, 
               const char *config, const char *sign) const {
    AzTree **my_tree = NULL; 
    AzObjPtrArray<AzTree> my_a; 
    my_a.alloc(&my_tree, t_num, "AzTrTreeEnsemble::copy_to"); 
    int tx; 
    for (tx = 0; tx < t_num; ++tx) {
      if (t[tx] != NULL) {
        my_tree[tx] = new AzTree(t[tx]); 
      }
    }
    out->transfer_from(my_tree, t_num, const_val, org_dim, config, sign); 
  }

  //! copy nodes only; not split
  void copy_nodes_from(const AzTrTreeEnsemble_ReadOnly *inp) {
    reset(); 
    const_val = inp->constant(); 
    org_dim = inp->orgdim(); 
    t_num = inp->size(); 

    s_param.reset(inp->param_c_str()); 
    dt_param = s_param.c_str(); 
    AzParam p(dt_param, false); 
    a_tree.alloc(&t, t_num, "AzTrTreeEnsemble::copy_nodes_from"); 
    int tx; 
    for (tx = 0; tx < t_num; ++tx) {
      t[tx] = new T(p);
      t[tx]->copy_nodes_from(inp->tree(tx)); 
    }
  }

  void warm_start(const AzTreeEnsemble *inp_ens, 
              const AzDataForTrTree *data, 
              AzParam &param,              
              const AzBytArr *s_temp_prefix, 
              const AzOut &out,           
              int max_t_num, 
              int search_t_num, 
              AzDvect *v_p, /* inout */
              const AzIntArr *inp_ia_tr_dx=NULL)
  {
    const char *eyec = "AzTrTreeEnsemble::warmup"; 
    if (max_t_num < inp_ens->size()) {
      throw new AzException(eyec, "maximum #tree is less than the #tree we already have"); 
    }

    reset(); 
    a_tree.alloc(&t, max_t_num, "AzTrTreeEnsemble::warmup"); 
    t_num = inp_ens->size(); 
    const_val = inp_ens->constant(); 
    org_dim = inp_ens->orgdim(); 
    if (org_dim > 0 && org_dim != data->featNum()) {
      throw new AzException(AzInputError, eyec, "feature dimensionality mismatch"); 
    }

    const AzIntArr *ia_tr_dx = inp_ia_tr_dx; 
    AzIntArr ia_temp; 
    if (ia_tr_dx == NULL) {
      ia_temp.range(0, data->dataNum()); 
      ia_tr_dx = &ia_temp; 
    }
    v_p->reform(data->dataNum()); 
    v_p->add(const_val, ia_tr_dx); 

    T dummy_tree(param); 
    if (dummy_tree.usingInternalNodes()) {
      throw new AzException(AzInputError, eyec, 
                "warm start is not allowed with use of internal nodes"); 
    }
    dummy_tree.printParam(out); 

    temp_files.reset(&dummy_tree, data->dataNum(), s_temp_prefix); 

    s_param.reset(param.c_str());   
    dt_param = s_param.c_str(); 
    AzParam p(dt_param, false); 
 
    int tx; 
    for (tx = 0; tx < t_num; ++tx) {
      t[tx] = new T(p); 
      t[tx]->forStoringDataIndexes(temp_files.point_file()); 
      if (search_t_num > 0 && tx < t_num-search_t_num) {
        t[tx]->quick_warmup(inp_ens->tree(tx), data, v_p, ia_tr_dx); 
      }
      else {
        t[tx]->warmup(inp_ens->tree(tx), data, v_p, ia_tr_dx); 
      }
    }    
  }

  void show(const AzSvFeatInfo *feat, //!< may be NULL 
            const AzOut &out, const char *header="") const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    o.printBegin(header, ", ", "="); 
    o.print("constant", const_val); 
    o.print("orgdim", org_dim); 
    o.print("#tree", t_num); 
    o.printEnd(); 

    int tx; 
    for (tx = 0; tx < t_num; ++tx) {
      AzBytArr s("tree"); s.inBrackets(tx); 
      AzPrint::writeln(out, s); 
      if (t[tx] != NULL) {
        t[tx]->show(feat, out); 
      }
    }
  }

protected:
  inline void alloc(int num, const char *msg="") {
    a_tree.alloc(&t, num, "AzTrTreeEnsemble::alloc", msg); 
  }

  inline void checkIndex(int tx, const char *msg) const {
    if (tx < 0 || tx >= t_num) {
      throw new AzException("AzTrTreeEnsemble::checkIndex", msg); 
    }
  }
}; 
#endif 
