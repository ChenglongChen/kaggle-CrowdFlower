/* * * * *
 *  AzReg_TreeRegArr.hpp 
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

#ifndef _AZ_REG_TREE_REG_ARR_HPP_
#define _AZ_REG_TREE_REG_ARR_HPP_

#include "AzUtil.hpp"
#include "AzReg_TreeReg.hpp"

class AzReg_TreeRegArr
{
public:
  virtual void reset(int tree_num) = 0; 
  virtual AzReg_TreeReg *reg(int tx) = 0; 
  virtual AzReg_TreeReg *reg_forNewLeaf(int tx) = 0; 
  virtual int size() const = 0; 
}; 
#endif 
