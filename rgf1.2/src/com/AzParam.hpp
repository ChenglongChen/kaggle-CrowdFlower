/* * * * *
 *  AzParam.hpp 
 *  Copyright (C) 2011-2014 Rie Johnson
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

#ifndef _AZ_PARAM_HPP_
#define _AZ_PARAM_HPP_

#include "AzUtil.hpp"
#include "AzStrPool.hpp"
#include "AzPrint.hpp"
#include "AzTools.hpp"

//! Parse parameters 
class AzParam {
protected:
  const char *param; 
  char dlm, kwval_dlm; 
  AzStrPool sp_used_kw; 
  bool doCheck; /* check unknown/duplicated keywords */
  AzBytArr s_param; 
public:
  AzParam(const char *inp_param, 
          bool inp_doCheck=true, 
          char inp_dlm=',',
          char inp_kwval_dlm='=') : sp_used_kw(100, 30)
  {
    param = inp_param; 
    doCheck = inp_doCheck; 
    dlm = inp_dlm; 
    kwval_dlm = inp_kwval_dlm; 
  }
  AzParam(int argc, const char *argv[], 
          bool inp_doCheck=true, 
          char file_mark='@', 
          char inp_dlm=',', 
          char inp_kwval_dlm='=',
          char cmt='#') : sp_used_kw(100, 30)
  {
    doCheck = inp_doCheck; 
    dlm = inp_dlm; 
    kwval_dlm = inp_kwval_dlm; 
    concat_args(argc, argv, &s_param, file_mark, dlm, cmt); 
    param = s_param.c_str(); 
  }
  
  inline const char *c_str() const { return param; }
  inline void swOn(bool *swch, const char *kw, 
                   bool doCheckKw=true) {
    if (param == NULL) return; 
    if (doCheckKw) {
      if (strstr(kw, "Dont") == kw || 
          (strstr(kw, "No") == kw && strstr(kw, "Normalize") == NULL)) {
        throw new AzException("AzParam::swOn", 
                              "On-kw shouldn't begin with \"Dont\" or \"No\"", kw); 
      }
    }
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr != NULL && 
        (*ptr == '\0' || *ptr == dlm)) {
      *swch = true; 
    }
    if (doCheck) sp_used_kw.put(kw); 
  }
  inline void swOff(bool *swch, const char *kw, 
                    bool doCheckKw=true) {
    if (param == NULL) return; 
    if (doCheckKw) {
      if (strstr(kw, "Dont") != kw &&
          (strstr(kw, "No") != kw && strstr(kw, "Normalize") == NULL)) {
        throw new AzException("AzParam::swOff", 
                              "Off-kw should start with \"dont\" or \"No\"", kw); 
      }
    }
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr != NULL && 
        (*ptr == '\0' || *ptr == dlm) ) {
      *swch = false; 
    }
    if (doCheck) sp_used_kw.put(kw); 
  }
  inline void vStr(const char *kw, AzBytArr *s) {
    if (param == NULL) return; 
    const char *bp = pointAfterKw(param, kw); 
    if (bp == NULL) return; 
    const char *ep = pointAt(bp, dlm); 
    s->reset(); 
    s->concat(bp, Az64::ptr_diff(ep-bp, "AzParam::vStr")); 
    if (doCheck) sp_used_kw.put(kw); 
  }

  inline void vFloat(const char *kw, double *out_value) {
    if (param == NULL) return; 
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr == NULL) return; 
    *out_value = atof(ptr); 
    if (doCheck) sp_used_kw.put(kw); 
  }
  inline void vInt(const char *kw, int *out_value) {
    if (param == NULL) return; 
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr == NULL) return; 
    *out_value = atol(ptr); 
    if (doCheck) sp_used_kw.put(kw); 
  }

  void check(const AzOut &out, AzBytArr *s_unused_param=NULL); 

  static void concat(const AzByte *inp, int len, 
                     AzBytArr *s_out,
                     AzByte dlm=',', 
                     AzByte cmt='#'); 
  static void read(const char *fn, 
                   AzBytArr *s_out, 
                   AzByte dlm=',',
                   AzByte cmt='#');                  
  static void concat_args(int argc, 
                          const char *argv[], 
                          AzBytArr *s_out, /* output */ 
                          char file_mark='@', 
                          char dlm=',', 
                          char cmt='#'); 
                   
protected:
  inline const char *pointAfterKw(const char *inp_inp, const char *kw) const {
    const char *ptr = NULL; 
    const char *inp = inp_inp; 
    for ( ; ; ) {
      ptr = strstr(inp, kw); 
      if (ptr == NULL) return NULL; 
      if (ptr == inp || *(ptr-1) == dlm) {
        break; 
      }
      inp = ptr + strlen(kw); 
    }
    ptr += strlen(kw); 
    return ptr; 
  }
  inline static const char *pointAt(const char *inp, const char *kw) {
    const char *ptr = strstr(inp, kw); 
    if (ptr == NULL) return inp + strlen(inp); 
    return ptr; 
  }
  inline static const char *pointAt(const char *inp, char ch) {
    const char *ptr = strchr(inp, ch); 
    if (ptr == NULL) return inp + strlen(inp); 
    return ptr; 
  }

  void analyze(AzStrPool *sp_unused, 
               AzStrPool *sp_kw); 
}; 
#endif 

