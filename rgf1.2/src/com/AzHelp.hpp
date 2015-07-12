/* * * * *
 *  AzHelp.hpp 
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

#ifndef _AZ_HELP_HPP_
#define _AZ_HELP_HPP_

#include "AzPrint.hpp"

/*----------------------------------------------------------------*/
//! Display help. 
class AzHelp {
protected:
  AzOut out; 
  int kw_width, ind; 
  static const int ind_dflt = 3; 
  static const int kw_width_dflt = 20; 
  static const int line_width = 78; 
public:
  AzHelp(const AzOut &inp_out, 
         int inp_kw_width=kw_width_dflt) {
    out = inp_out; 
    kw_width = inp_kw_width; 
    ind = ind_dflt; 
  }

  inline void set_kw_width() {
    kw_width = kw_width_dflt; 
  }
  inline void set_kw_width(int inp_kw_width) {
    kw_width = inp_kw_width; 
  }
  inline void set_indent(int inp_ind) {
    ind = inp_ind; 
  }

  /*-------------------*/
  void toplevel_header(const char *desc, AzByte dlm='*') 
  {
    if (out.isNull()) return; 

    newline(); 

    AzBytArr s; 
    s.fill(dlm, 3); 
    int dlm_len = Az64::cstrlen(desc) + s.length()*2; 
    dlm_len = MIN(line_width, dlm_len); 

    AzBytArr s_long; 
    s_long.fill(dlm, dlm_len); 

    AzPrint::writeln(out, s_long.c_str()); 
    AzPrint::write(out, s.c_str()); 
    AzPrint::write(out, desc); 
    AzPrint::writeln(out, s.c_str()); 
    AzPrint::writeln(out, s_long.c_str()); 
  }

  int getLevel() const {
    return out.getLevel(); 
  }

  /*-------------------*/
  void begin(const char *compo, const char *name, const char *desc=NULL) 
  {
    if (out.getLevel() == 0) return; 

    if (out.isNull()) return; 

    AzBytArr s(desc); 
    if (s.length() > 0) {
      s.c(" ("); s.c(name); s.c(")"); 
      AzPrint o(out); 
      o.printBegin("", " ", NULL, ind); 
      o.print("<"); o.print(s); o.print(">"); 
      o.printEnd(); 
    }
  }

  /*-------------------*/
  void end() {
    if (out.getLevel() == 0) return; 

    newline(); 
  }

  /*-------------------*/
  void newline() 
  {
    if (out.isNull()) return; 
    AzPrint::writeln(out, ""); 
  }

  /*-------------------*/
  void item_noquotes(const char *kw, const char *desc) {
    AzBytArr s_dflt; 
    _item(false, kw, desc, s_dflt, true);  
  }

  /*-------------------*/
  void item_required_lvl(const char *kw, const char *desc, int lvl) {
    if (out.getLevel() < lvl) return; 
    _item(true, kw, desc); 
  }

  /*-------------------*/
  void item_experimental(const char *kw, const char *desc, int dflt) {
    if (out.getLevel() == 0) return; 
    item(kw, desc, dflt); 
  }
  void item_experimental(const char *kw, const char *desc, double dflt) {
    if (out.getLevel() == 0) return; 
    item(kw, desc, dflt); 
  }
  void item_experimental(const char *kw, const char *desc, const char *dflt=NULL) {
    if (out.getLevel() == 0) return; 
    item(kw, desc, dflt); 
  }

  /*-------------------*/
  void item(const char *kw, const char *desc, int dflt) {
    _item(false, kw, desc, dflt); 
  }
  void item(const char *kw, const char *desc, double dflt) {
    _item(false, kw, desc, dflt); 
  }
  void item(const char *kw, const char *desc, const char *dflt=NULL) {
    _item(false, kw, desc, dflt); 
  }

  /*-------------------*/
  void item_required(const char *kw, const char *desc, int dflt) {
    _item(true, kw, desc, dflt); 
  }
  void item_required(const char *kw, const char *desc, double dflt) {
    _item(true, kw, desc, dflt); 
  }
  void item_required(const char *kw, const char *desc, const char *dflt=NULL) {
    _item(true, kw, desc, dflt); 
  }

  /*-------------------*/
  void pitem(const char *name, const char *desc) {
    AzPrint::write(out, "    "); 
    AzPrint::write(out, name); 
    AzPrint::write(out, ": "); 
    AzPrint::writeln(out, desc); 
  }

  template <class T>
  void writeln(const T inp) {
    if (out.isNull()) return; 
    AzPrint::writeln(out, inp); 
  }
  template <class T, class U>
  void writeln(const T inp0, const U inp1) {
    if (out.isNull()) return; 
    AzPrint::writeln(out, inp0, inp1); 
  }
  void writeln_desc(const char *desc) {
    writeln(desc, ind+kw_width+1); 
  }
  void writeln_header_experimental(const char *txt) {
    if (out.getLevel() == 0) return; 
    writeln_header(txt); 
  }
  void writeln_header(const char *txt) {
    writeln(txt, ind); 
  }
  void writeln(const char *txt, int ind_len) {
    if (out.isNull()) return; 
    AzBytArr s(txt); 
    _multi_lines(&s, ind_len);  
    AzBytArr s_ind; s_ind.fill(' ', ind_len); 
    AzPrint::writeln(out, s_ind, s); 
  }
  void nl() {
    if (out.isNull()) return; 
    AzPrint::writeln(out, ""); 
  }

protected:
  /*-------------------*/
  void _item(bool isRequired, 
             const char *kw, const char *desc, 
             int dflt) 
  {
    AzBytArr s(" (Default:"); s.cn(dflt); s.c(")"); 
    _item(isRequired, kw, desc, s); 
  }
  void _item(bool isRequired, 
             const char *kw, const char *desc, 
             double dflt) 
  {
    AzBytArr s(" (Default:"); s.cn(dflt); s.c(")"); 
    _item(isRequired, kw, desc, s); 
  }
  void _item(bool isRequired, 
             const char *kw, const char *desc, 
             const char *dflt=NULL) 
  {
    AzBytArr s; 
    if (dflt != NULL && strlen(dflt) > 0) {
      s.c(" (Default:"); s.c(dflt); s.c(")"); 
    }
    _item(isRequired, kw, desc, s); 
  }

  /*-------------------*/
  void _item(bool isRequired, 
             const char *kw, const char *desc, 
             const AzBytArr &s_dflt, 
             bool no_quotes=false)
  {
    if (out.isNull()) return; 
    int indent = ind; 
    if (isRequired) indent-=2; 
    AzPrint o(out); 
    o.printBegin("", " ", "", indent); 
    if (isRequired) o.print("*"); 

    AzBytArr s_kw; 
    if (no_quotes) s_kw.concat(kw); 
    else           s_kw.inQuotes(kw, "\""); 
    int len = kw_width - s_kw.length(); 
    if (len > 0) {
      AzBytArr s_fill; 
      s_fill.fill(' ', len); 
      s_kw.concat(&s_fill); 
    }
    AzBytArr s_desc(desc); 
    s_desc.concat(&s_dflt);    
    _multi_lines(&s_desc, ind+kw_width+1);  

    o.print(s_kw); 
    if (len >= 0) {
      o.print(s_desc); 
    }
    else {
      o.printEnd(); 
      o.printBegin("", "", "", ind+kw_width+1); 
      o.print(s_desc); 
    }
    o.printEnd();   
  }

  static void _multi_lines(AzBytArr *s, /* inout */
                           int indent) {
    AzBytArr s_txt(s);  
    s->reset(); 
    int width = line_width - indent; 
    const AzByte *txt = s_txt.point(); 
    int txt_len = s_txt.length(); 
    int offs; 
    for (offs = 0; offs < txt_len; ) {
      int next_offs = MIN(offs + width, txt_len); 

      if (next_offs < txt_len && 
          txt[next_offs] != ' ') {
        int oo; 
        for (oo = next_offs-1; oo >= offs; --oo) {
          if (txt[oo] == ' ') break; 
        }
        if (oo > offs) next_offs = oo; 
      }

      s->concat(txt + offs, next_offs - offs); 
      offs = next_offs; 

      for ( ; offs < txt_len; ++offs) {
        if (txt[offs] != ' ') break; 
      }
      if (offs < txt_len) {
        s->nl(); 
        AzBytArr s_fill; 
        s_fill.fill(' ', indent); 
        s->concat(&s_fill); 
      }
    }
  }
}; 
#endif 

