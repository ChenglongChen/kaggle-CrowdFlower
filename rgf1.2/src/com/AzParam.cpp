/* * * * *
 *  AzParam.cpp 
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

#include "AzParam.hpp"

/*-------------------------------------------------------------*/
void AzParam::check(const AzOut &out, 
                     AzBytArr *s_unused)
{
  if (param == NULL) return; 

  AzStrPool sp_unused, sp_kw; 
  analyze(&sp_unused, &sp_kw); 

  bool error = false; 

  int ix; 
  for (ix = 0; ix < sp_unused.size(); ++ix) {
    if (sp_unused.getLen(ix) <= 0) continue; 

    /*---  this parameter wasn't used by anyone.  ---*/
    if (s_unused != NULL) {
      /*---  may be used by someone else  ---*/
      if (s_unused->length() > 0) s_unused->concat(dlm); 
      s_unused->concat(sp_unused.c_str(ix));     
    }
    else {
      /*---  it could be a typo  ---*/
      AzBytArr s; 
      s.concat("!Warning! Unknown parameter: \""); 
      s.concat(sp_unused.c_str(ix)); s.concat("\""); 
      AzPrint::writeln(out, s); 
    }
  }

  AzBytArr s_err; 
  int kw_num = sp_kw.size(); 
  sp_kw.commit(); 
  if (sp_kw.size() != kw_num) {
    for (ix = 0; ix < sp_kw.size(); ++ix) {
      AZint8 count = sp_kw.getCount(ix); 
      if (count > 1) {
        /*---  this keyword appears more than once.  ---*/
        if (s_err.length() <= 0) s_err.newline(); 
        s_err.concat("  Duplicated keyword: "); 
        s_err.concat(sp_kw.c_str(ix)); 
        s_err.newline(); 
        error = true; 
      }
    }
  }

  if (error) {
    throw new AzException(AzInputError, "AzParam::check", s_err.c_str()); 
  }
}

/*-------------------------------------------------------------*/
void AzParam::analyze(AzStrPool *sp_unused, 
                      AzStrPool *sp_kw)
{
  if (param == NULL) return; 
  sp_used_kw.commit(); 

  AzStrPool sp_kwval; 
  AzTools::getStrings(param, dlm, &sp_kwval); 
  int ix; 
  for (ix = 0; ix < sp_kwval.size(); ++ix) {
    int len; 
    const AzByte *ptr = sp_kwval.point(ix, &len); 
    if (len <= 0) continue; 

    AzBytArr s_kw; 
    AzTools::getString(&ptr, ptr+len, kwval_dlm, &s_kw);
    if (s_kw.length() < len) s_kw.concat(kwval_dlm); 
    if (sp_used_kw.find(&s_kw) < 0) {
      /*---  this parameter wasn't used by anyone.  ---*/
      sp_unused->put(sp_kwval.c_str(ix)); 
    }
    if (sp_kw != NULL) sp_kw->put(&s_kw); 
  }
}
 
/* static */
/*--------------------------------------------------------------------------*/
/* remove comments (beginning with cmt); insert dlm; concatenate parameters */
/*--------------------------------------------------------------------------*/
void AzParam::concat(const AzByte *inp, int len, 
                     AzBytArr *s_out,
                     AzByte dlm, 
                     AzByte cmt)                     
{
  const AzByte *linep = inp, *inp_end = inp+len; 
  for ( ; linep < inp_end; ) {
    const AzByte *line_end = (AzByte *)memchr(linep, '\n', inp_end-linep); 
    if (line_end == NULL) line_end = inp_end; 
    
    const AzByte *wp; 
    for (wp = linep; wp < line_end; ++wp) {
      if (*wp > 0x20) break; /* remove blank at the beginning */
    }
    linep = wp; 
    for (wp = linep; wp < line_end; ++wp) {
      if (*wp == cmt || *wp <= 0x20) break; 
    }
    if (wp - linep > 0) {
      if (s_out->length() > 0) {
        s_out->concat(dlm); 
      }
      s_out->concat(linep, (int)(wp-linep)); 
    }    
    linep = line_end+1; /* +1 for \n */
  }
}

/*-------------------------------------------------------------*/
void AzParam::read(const char *fn, 
                   AzBytArr *s_out, 
                   AzByte dlm, 
                   AzByte cmt)
{
  AzFile file(fn); 
  file.open("rb"); 
  int len = file.size_under2G("AzParam::read, parameter file"); 
  AzBytArr ba_buff; 
  AzByte *buff = ba_buff.reset(len, 0); 
  file.seekReadBytes(0, len, buff); 
  file.close(); 
  concat(buff, len, s_out, dlm, cmt); 
}                   

/*-------------------------------------------------------------*/
void AzParam::concat_args(int argc, 
                          const char *argv[], 
                          AzBytArr *s_out, /* output */ 
                          char file_mark, 
                          char dlm, 
                          char cmt)
{
  s_out->reset(); 
  int ax; 
  for (ax = 0; ax < argc; ++ax) {
    if (*argv[ax] == file_mark) {
      const char *fn = argv[ax]+1; 
      AzParam::read(fn, s_out, dlm, cmt); 
    }
    else {
      if (s_out->length() > 0) s_out->c(dlm); 
      s_out->c(argv[ax]); 
    }
  }
}                          
