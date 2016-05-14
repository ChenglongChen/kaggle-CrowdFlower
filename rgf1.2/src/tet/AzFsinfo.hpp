/* * * * *
 *  AzFsinfo.hpp 
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

#ifndef _AZ_FSINFO_HPP_
#define _AZ_FSINFO_HPP_

#include "AzUtil.hpp"
#include "AzFsinfo.hpp"

/*------------------------------------------------------------------*/
class AzFsinfo_ {
public:
  const int *dxs; 
  int dx_num; 
  double wy_sum, w_sum; 
  AzFsinfo_() : dxs(NULL), dx_num(0), wy_sum(0), w_sum(0) {}
}; 

class AzFsinfo {
public:
  double value; 
  AzFsinfo_ i[2]; 
  AzFsinfo() : value(0) {}
}; 

/*------------------------------------------------------------------*/
class AzFsinfoOnFeat {
protected:  
  AzDataArray<AzFsinfo> arr; 
  int dx_num; 
  int le_idx, gt_idx;  
  AzIntArr ia_sorted_dx; /* sorted by values */

  static const int src_idx = 1; 
  static const int dest_idx = 0; 

public:
  AzFsinfoOnFeat() : le_idx(-1), gt_idx(-1), dx_num(0) {}
  inline int le_index() const { 
    if (le_idx < 0) throw new AzException("AzFsinfoOnFeat::le_index", "not ready"); 
    return le_idx; 
  }
  inline int gt_index() const { 
    if (gt_idx < 0) throw new AzException("AzFsinfoOnFeat::gt_index", "not ready"); 
    return gt_idx; 
  }
  inline static int source_index() { return src_idx; }
  inline static int dest_index() { return dest_idx; }
  inline static int le_index(bool sorted_isForward) {
    if (sorted_isForward) return dest_idx; 
    else                  return src_idx; 
  }
  inline static int gt_index(bool sorted_isForward) {
    if (sorted_isForward) return src_idx; 
    else                  return dest_idx; 
  }

  void reset(int split_num_max, 
             int inp_dx_num, 
             bool sorted_isForward) 
  {
    le_idx = le_index(sorted_isForward); 
    gt_idx = gt_index(sorted_isForward); 
    arr.reset(split_num_max); 
    dx_num = inp_dx_num; 
    ia_sorted_dx.reset(); 
  }

  void put(const AzFsinfo *info) {
    *(arr.new_slot()) = *info; 
  }

  inline void commit(const AzIntArr *inp_ia_sorted_dx) {
    ia_sorted_dx.reset(inp_ia_sorted_dx); 
    int num = size(); 
    int sx; 
    for (sx = 0; sx < num; ++sx) {
      AzFsinfo *ptr = arr.point_u(sx); 
      ptr->i[dest_idx].dxs = ia_sorted_dx.point(); 
    }
  }
  inline int size() const {
    return arr.cursor(); 
  }

  inline const AzFsinfo *point(int sx) const {
    return arr.point(sx); 
  }
}; 

/*------------------------------------------------------------------*/
class AzFsinfoOnNode {
protected:
  AzObjPtrArray<AzFsinfoOnFeat> a; 
  AzFsinfoOnFeat **arr; 

public:
  AzFsinfoOnNode() : arr(NULL) {}
  inline void reset(int f_num) {
    a.free(&arr); 
    a.alloc(&arr, f_num, "AzFsinfoOnNode::reset", "reset"); 
  }
  inline const AzFsinfoOnFeat *point(int fx) const {
    checkIndex(fx, "point"); 
    return arr[fx]; 
  }
  inline AzFsinfoOnFeat *point_u(int fx) {
    checkIndex(fx, "point_u"); 
    if (arr[fx] == NULL) {
      arr[fx] = new AzFsinfoOnFeat(); 
    }
    return arr[fx]; 
  }
  inline int size() const {
    return a.size(); 
  }
protected:
  inline void checkIndex(int fx, const char *msg) const {
    if (fx < 0 || fx >= a.size()) {
      throw new AzException("AzFsinfoOnNode::checkIndex", msg); 
    }
  }  
};
 
/*------------------------------------------------------------------*/
class AzFsinfoOnTree {
protected:
  AzObjPtrArray<AzFsinfoOnNode> a; 
  AzFsinfoOnNode **arr; 
  AzFsinfoOnFeat temp_fof; 

public:
  AzFsinfoOnTree() : arr(NULL) {}

  AzFsinfoOnFeat *temp_on_feat() {
    return &temp_fof; 
  }

  inline void realloc(int new_num) {
    if (new_num > size()) {
      int num = (new_num+255)/256*256; 
      a.realloc(&arr, num, "AzFsinfoOnNodeTree::realloc");     
    }
  }
  inline bool isAvailable(int nx) const {
    if (nx < 0 || nx >= a.size()) return false; 
    if (arr[nx] == NULL) return false; 
    if (arr[nx]->size() <= 0) return false; 
    return true; 
  }
  inline const AzFsinfoOnNode *point(int nx) const { /* may be NULL */
    checkIndex(nx, "point"); 
    return arr[nx]; 
  }
  inline AzFsinfoOnNode *point_u(int nx) {
    checkIndex(nx, "point_u"); 
    if (arr[nx] == NULL) {
      arr[nx] = new AzFsinfoOnNode(); 
    }
    return arr[nx]; 
  }
  inline int size() const {
    return a.size(); 
  }
  inline void release(int nx) {
    if (nx >= size()) return; 
    checkIndex(nx, "release"); 
    delete arr[nx]; arr[nx] = NULL; 
  }
  inline void release() {
    a.free(&arr); 
  }
  inline void reset() {
    release(); 
  }

protected:
  inline void checkIndex(int nx, const char *msg) const {
    if (nx < 0 || nx >= a.size()) {
      throw new AzException("AzFsinfoOnTree::checkIndex", msg); 
    }
  } 
};

#endif 
