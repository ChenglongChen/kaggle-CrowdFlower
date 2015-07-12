/* * * * *
 *  AzRgfTrainerSel.hpp 
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

#ifndef _AZ_RGF_TRAINER_SEL_HPP_
#define _AZ_RGF_TRAINER_SEL_HPP_

#include "AzRgforest.hpp"
#include "AzRgforest_TreeReg.hpp"
#include "AzReg_TsrOpt.hpp"
#include "AzReg_TsrSib.hpp"

#include "AzTETselector.hpp"
#include "AzPrint.hpp"

//! Training algorithm selector.  
class AzRgfTrainerSel : /* implements */ public virtual AzTETselector  {
protected: 
  AzRgforest rgf; 
  AzRgforest_TreeReg<AzReg_TsrSib> rgf_sib; 
  AzRgforest_TreeReg<AzReg_TsrOpt> rgf_opt; 

  #define kw_rgf "RGF"
  #define kw_rgf_sib "RGF_Sib"
  #define kw_rgf_opt "RGF_Opt"

  AzStrPool sp_name; 
  AzDataArray<AzTETrainer *> alg; 

  virtual void reset() {
    int id = 0; 
    sp_name.putv(kw_rgf, id++);          *alg.new_slot() = &rgf; 
    sp_name.putv(kw_rgf_sib, id++);      *alg.new_slot() = &rgf_sib; 
    sp_name.putv(kw_rgf_opt, id++);      *alg.new_slot() = &rgf_opt; 
    sp_name.commit(); 
  }

public:
  AzRgfTrainerSel() {
    reset(); 
  }

  virtual const char *dflt_name() const {
    return kw_rgf; 
  }
  virtual const char *another_name() const {
    return kw_rgf_sib; 
  }
  const AzStrArray *names() const {
    return &sp_name; 
  }

  virtual void printOptions(const char *dlm, AzBytArr *s) const {
    int ix; 
    for (ix = 0; ix < sp_name.size(); ++ix) {
      if (ix > 0) s->c(dlm); 
      s->c(sp_name.c_str(ix)); 
    }
  }

  virtual void printHelp(AzHelp &h) const {
    h.begin("", "", ""); 
    int ix; 
    for (ix = 0; ix < sp_name.size(); ++ix) {
      int id = sp_name.getValue(ix); 
      const AzTETrainer *trainer = *alg.point(id); 
      h.item(sp_name.c_str(ix), trainer->description()); 
    }    
    h.end(); 
  }

  virtual AzTETrainer *select(const char *alg_name, //!< name of algorithm. 
                              //! if true, don't throw exception at error. 
                              bool dontThrow=false) const
  {
    AzTETrainer *trainer = NULL; 

    int ex = sp_name.find(alg_name); 
    if (ex < 0 && !dontThrow) {
      throw new AzException(AzInputNotValid, "algorithm name", alg_name); 
    }
    if (ex >= 0) {
      int id = sp_name.getValue(ex); 
      trainer = *alg.point(id); 
    }
    return trainer; 
  }
}; 

#endif 

