/* * * * *
 *  AzOut.hpp 
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

#ifndef _AZ_OUT_HPP_
#define _AZ_OUT_HPP_

class AzOut {
protected:
  bool isActive; 
  int level; 
public:
  ostream *o; 

  inline AzOut() : o(NULL), isActive(true), level(0) {}
  inline AzOut(ostream *o_ptr) : isActive(true), level(0) {
    o = o_ptr;
  }
  inline void reset(ostream *o_ptr) {
    o = o_ptr; 
    activate(); 
  }

  inline void deactivate() { 
    isActive = false; 
  }
  inline void activate() { 
    isActive = true; 
  }
  inline void setStdout() { 
    o = &cout; 
    activate(); 
  }
  inline void setStderr() { 
    o = &cerr; 
    activate(); 
  }
  inline bool isNull() const { 
    if (!isActive) return true; 
    if (o == NULL) return true; 
    return false; 
  }
  inline void flush() const { 
    if (o != NULL) o->flush(); 
  }
  inline void setLevel(int inp_level) {
    level = inp_level; 
  }
  inline int getLevel() const {
    return level; 
  }
}; 
#endif 
