/* * * * *
 *  AzTETselector.hpp 
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

#ifndef _AZ_TET_SELECTOR_HPP_
#define _AZ_TET_SELECTOR_HPP_

#include "AzTETrainer.hpp"

class AzTETselector {
public:
  //! Return trainer 
  virtual AzTETrainer *select(const char *alg_name, //! algorithm name
                              //! if true, don't throw exception on error
                              bool dontThrow=false
                              ) const = 0; 

  virtual const char *dflt_name() const = 0; 
  virtual const char *another_name() const = 0; 
  virtual const AzStrArray *names() const = 0; 
  virtual bool isRGFfamily(const char *name) const {
    AzBytArr s(name); 
    return s.beginsWith("RGF"); 
  }
  virtual bool isGBfamily(const char *name) const {
    AzBytArr s(name); 
    return s.beginsWith("GB"); 
  }

  //! Return algorithm names. 
  virtual void printOptions(const char *dlm, //! delimiter between algorithm names.  
                            AzBytArr *s) //!< output: algorithm names separated by dlm. 
                            const = 0; 

  //! Help 
  virtual void printHelp(AzHelp &h) const = 0; 
}; 

#endif 

