/* * * * *
 *  AzReg_TreeRegArrImp.hpp 
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

#ifndef _AZ_REG_TREE_REG_ARR_IMP_HPP_
#define _AZ_REG_TREE_REG_ARR_IMP_HPP_

#include "AzUtil.hpp"
#include "AzRgfTreeEnsemble.hpp"
#include "AzReg_TreeRegArr.hpp"

template<class T>
class AzReg_TreeRegArrImp : /* implements */ public virtual AzReg_TreeRegArr 
{
protected: 
  AzPtrPool<T> areg; 
  T template_reg; 
  T temporary_reg; 
  AzReg_TreeRegShared_Dflt shared; 

public:
  T *tmpl_u() { return &template_reg; }
  const T *tmpl() const { return &template_reg; }

  inline int size() const { return areg.size(); }
  void reset(int tree_num) {
    areg.reset(); 
    int tx; 
    for (tx = 0; tx < tree_num; ++tx) {
      T *reg = areg.new_slot(); 
      reg->copyParam_from(&template_reg); 
      reg->set_shared(&shared); 
    }
    temporary_reg.set_shared(&shared); 
  }
  inline AzReg_TreeReg *reg(int tx) {
    return areg.point_u(tx); 
  }
  inline AzReg_TreeReg *reg_forNewLeaf(int tx) {
    int t_num = areg.size(); 
    if (tx < t_num) return areg.point_u(tx); 

    temporary_reg.copyParam_from(&template_reg); 
    return &temporary_reg; /* should be root-only tree */
  }
}; 
#endif 
