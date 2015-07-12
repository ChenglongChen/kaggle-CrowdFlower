/* * * * *
 *  AzTreeRule.hpp 
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

#ifndef _AZ_TREE_RULE_HPP_
#define _AZ_TREE_RULE_HPP_

#include "AzUtil.hpp"

class AzTreeRule {
protected:
  AzBytArr ba; 

public:
  inline void reset() {
    ba.clear(); 
  }
  inline const AzBytArr *bytarr() {
    return &ba; 
  }
  inline void reset(const AzTreeRule *inp) {
    ba.reset(); 
    if (inp == NULL) return; 
    ba.reset(&inp->ba); 
  }
  inline void append(int fx, 
                     bool isLE, 
                     double border_val)
  {
    /*---  feat#, isLE, border_val  ---*/
    ba.concat((AzByte *)(&fx), sizeof(fx)); 
    ba.concat((AzByte *)(&isLE), sizeof(isLE)); 
    ba.concat((AzByte *)(&border_val), sizeof(border_val)); 
  }
  inline void append(const AzTreeRule *inp) {
    if (inp != NULL) {
      ba.concat(&inp->ba); 
    }
  }
  inline void finalize() {
    if (ba.getLen() == 0) {
      ba.concat('_'); /* root node (CONST) */
    }
  }
  inline const AzBytArr *byteArr() {
    return &ba; 
  }
}; 
#endif 
