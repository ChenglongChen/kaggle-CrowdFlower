/* * * * *
 *  AzSvFeatInfo.hpp 
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

#ifndef _AZ_SV_FEAT_INFO_HPP_
#define _AZ_SV_FEAT_INFO_HPP_

#include "AzUtil.hpp"
#include "AzStrPool.hpp"
#include "AzPrint.hpp"

//! Abstract class: interfact to access feature descriptions.  
class AzSvFeatInfo {
public:
  // Concatenate feature description to str_desc.  
  virtual void concatDesc(int ex, //!< feature id
                          AzBytArr *str_desc) const = 0; 

  //! Return number of features. 
  virtual int featNum() const = 0; /* # of features */

  void desc(int ex, AzBytArr *str_desc) const {
    str_desc->reset(); 
    concatDesc(ex, str_desc); 
  }

  int desc2fno(const char *fnm) const {
    int fx; 
    for (fx = 0; fx < featNum(); ++fx) {
      AzBytArr s; 
      desc(fx, &s); 
      if (s.compare(fnm) == 0) {
        return fx; 
      }
    }
    return -1; 
  }

  void show(const AzOut &out, const AzIntArr *ia_fxs) const {
    int ix; 
    for (ix = 0; ix < ia_fxs->size(); ++ix) {
      int fx = ia_fxs->get(ix); 
      AzBytArr s("???"); 
      if (fx>=0 && fx<featNum()) desc(fx, &s); 
      AzPrint::writeln(out, s); 
    }
  }

  void beginsWith(const char *kw, 
                  AzIntArr *ia_fxs) const {
    AzStrPool sp_kw; 
    sp_kw.put(kw); 
    beginsWith(&sp_kw, ia_fxs); 
  }

  void contains(const char *kw, 
                AzIntArr *ia_fxs) const {
    AzStrPool sp_kw; 
    sp_kw.put(kw); 
    contains(&sp_kw, ia_fxs); 
  }

  void beginsWith(const AzStrArray *sp_kw, 
                  AzIntArr *ia_fxs) const {
    ia_fxs->reset(); 
    if (sp_kw->size()==0) return; 
    int fx; 
    for (fx = 0; fx < featNum(); ++fx) {
      AzBytArr s; 
      desc(fx, &s); 
      int ix; 
      for (ix = 0; ix < sp_kw->size(); ++ix) {
        if (s.beginsWith(sp_kw->c_str(ix))) {
          ia_fxs->put(fx); 
          break; 
        }
      }
    } 
  }

  void contains(const AzStrArray *sp_kw, 
                  AzIntArr *ia_fxs) const {
    ia_fxs->reset(); 
    if (sp_kw->size()==0) return; 
    int fx; 
    for (fx = 0; fx < featNum(); ++fx) {
      AzBytArr s; 
      desc(fx, &s); 
      int ix; 
      for (ix = 0; ix < sp_kw->size(); ++ix) {
        if (s.contains(sp_kw->c_str(ix))) {
          ia_fxs->put(fx); 
          break; 
        }
      }
    } 
  }

  int equals(const char *kw) const {
    int fx; 
    for (fx = 0; fx < featNum(); ++fx) {
      AzBytArr s; 
      desc(fx, &s); 
      if (s.compare(kw) == 0) {
        return fx; 
      }
    } 
    return -1; 
  }
}; 

#endif 




