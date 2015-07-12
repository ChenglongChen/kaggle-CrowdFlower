/* * * * *
 *  AzTrTreeEnsemble_ReadOnly.hpp 
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

#ifndef _AZ_TR_TREE_ENSEMBLE_READONLY_HPP_
#define _AZ_TR_TREE_ENSEMBLE_READONLY_HPP_

#include "AzTrTree_ReadOnly.hpp"
#include "AzTreeEnsemble.hpp"
#include "AzSvFeatInfo.hpp"

//! Abstract class: interface for read-only access to trainalbe tree ensemble.  
class AzTrTreeEnsemble_ReadOnly {
public:
  virtual bool usingTempFile() const { return false; }
  virtual const AzTrTree_ReadOnly *tree(int tx) const = 0; 
  virtual int leafNum() const = 0; 
  virtual int leafNum(int tx0, int tx1) const = 0; 
  virtual int size() const = 0; 
  virtual int max_size() const = 0;  
  virtual int lastIndex() const = 0; 
  virtual void copy_to(AzTreeEnsemble *out_ens, 
                       const char *config, const char *sign) const = 0; 
  virtual void show(const AzSvFeatInfo *feat, 
                    const AzOut &out, const char *header="") const = 0; 
  virtual double constant() const = 0; 
  virtual int orgdim() const = 0; 
  virtual const char *param_c_str() const = 0; 
}; 
#endif 
