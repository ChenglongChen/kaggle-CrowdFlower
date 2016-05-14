/* * * * *
 *  AzTools.cpp 
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


#include "AzTools.hpp"
#include "AzPrint.hpp"

/*--------------------------------------------------------*/
int AzTools::writeList(const char *fn, 
                        const AzStrArray *sp_list)
{
  int max_len = 0; 
  AzFile file(fn); 
  file.open("wb"); 
  int num = sp_list->size(); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    AzBytArr s; 
    sp_list->get(ix, &s); 
    s.concat("\n");
    max_len = MAX(max_len, s.getLen());  
    s.writeText(&file);  
  }
  file.close(true); 
  return max_len; 
}

/*--------------------------------------------------------*/
void AzTools::readList(const char *fn, 
                       AzStrPool *sp_list, /* output */
                       const AzByte *dlm)
{
  AzFile file(fn); 
  file.open("rb"); 
  int sz = file.size_under2G("AzTools::readList, list file"); 

  AzByte *buff = NULL; 
  int buff_len = sz+1; 
  AzBaseArray<AzByte> _a(buff_len, &buff);

  for ( ; ; ) {
    int len = file.gets(buff, buff_len); 
    if (len <= 0) {
      break; 
    }
    int str_len; 
    const AzByte *str = strip(buff, buff+len, &str_len);  
    if (dlm != NULL) {
      int ix; 
      for (ix = 0; ix < str_len; ++ix) if (*(str+ix) == *dlm) break; 
      str_len = ix; 
    } 
    sp_list->put(str, str_len); 
  }
}

/*--------------------------------------------------------*/
void AzTools::flatten(const AzIFarr *ifa_dx_val, 
                       /*---  output  ---*/
                       AzIntArr *ia_index, 
                       AzDvect *v_value)
{                       
  /*---  make it flat for faster access later on  ---*/
  int num = ifa_dx_val->size(); 
  ia_index->reset(num, AzNone); 
  if (num > 0) {
    v_value->reform(num); 
    double *value = v_value->point_u(); 
    int *index = ia_index->point_u(); 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      value[ix] = ifa_dx_val->get(ix, &index[ix]); 
    }
  }
}

/*--------------------------------------------------------*/
char *AzTools::chomp(AzByte *inp, int inp_len) 
{
  AzByte *wp1, *wp2; 
  for (wp1 = inp; wp1 < inp + inp_len; ++wp1) {
    if (*wp1 > 0x20)
      break;
  }

  for (wp2 = inp + inp_len - 1; wp2 >= inp; --wp2) {
    if (*wp2 > 0x20) 
      break;
  }
  
  if (wp2 + 1 - wp1 <= 0) {
    *inp = '\0'; 
    return (char *)inp; 
  }  
  
  *(wp2+1) = '\0'; 
  return (char *)wp1; 
}

/*--------------------------------------------------------*/
/* (NOTE) This doesn't skip preceding white characters.   */
/*--------------------------------------------------------*/
const AzByte *AzTools::getString(const AzByte **wpp, const AzByte *data_end, 
                                 AzByte dlm, int *token_len) 
{
  const AzByte *token = *wpp; 
  const AzByte *wp = *wpp; 
  for ( ; wp < data_end; ++wp) {
    if (*wp == dlm) break;
  }
  *token_len = Az64::ptr_diff(wp-token, "AzTools::getString"); 

  /*-----  point the next to the delimiter  -----*/
  if (wp < data_end) {
    ++wp;  
  }
  *wpp = wp; 
  return token; 
}

/*--------------------------------------------------------*/
void AzTools::getStrings(const AzByte *data, int data_len, 
                         AzByte dlm, 
                         AzStrPool *sp_out) /* output */
{
  const AzByte *data_end = data + data_len; 
  const AzByte *wp = data; 
  for ( ; wp < data_end; ) {
    int len; 
    const AzByte *token = getString(&wp, data_end, dlm, &len); 
    sp_out->put(token, len); 
  }
}

/*--------------------------------------------------------*/
void AzTools::getStrings(const AzByte *data, const AzByte *data_end, 
                         AzByte dlm, 
                         AzDataArray<AzBytArr> *aStr_out) /* output */
{
  /*---  count  ---*/
  const AzByte *wp = data; 
  int count = 0; 
  for ( ; wp < data_end; ++count) {
    int len; 
    getString(&wp, data_end, dlm, &len); 
  }

  aStr_out->reset(count); 
  int no = 0; 
  for (wp = data; wp < data_end; ++no) {
    int len; 
    const AzByte *token = getString(&wp, data_end, dlm, &len); 
    aStr_out->point_u(no)->concat(token, len); 
  }
}

/*--------------------------------------------------------*/
int AzTools::getNumber(const AzByte **wpp, const AzByte *data_end, 
                       AzByte dlm) 
{
  const AzByte *wp = *wpp; 
  if (wp < data_end && *wp == dlm) {
    *wpp = wp + 1; 
    return AzNone; /* this is no good */
  }

  int value = 0; 
  int sign = 1; 
  if (*wp == '-') {
    sign = -1; 
    ++wp; 
  }
  else if (*wp == '+') {
    ++wp; 
  }

  for ( ; wp < data_end; ++wp) {
    if (*wp == dlm) break;
    if (*wp >= '0' && *wp <= '9') {
      value *= 10; 
      value += (*wp - '0'); 
    }
  }

  /*-----  point next to the delimiter  -----*/
  if (wp < data_end) {
    ++wp;  
  }

  *wpp = wp; 
  return value * sign; 
}

/*--------------------------------------------------------*/
/* (NOTE) This skips preceding white characters.          */
/*--------------------------------------------------------*/
const AzByte *AzTools::getString(const AzByte **wpp, const AzByte *data_end, 
                                   int *byte_len) 
{
  const AzByte *wp = *wpp; 
  for ( ; wp < data_end; ++wp) {
    if (*wp > ' ') break;
  }
  const AzByte *token = wp; 

  for ( ; wp < data_end; ++wp) {
    if (*wp <= ' ') break;
  }
  const AzByte *token_end = wp; 

  *byte_len = Az64::ptr_diff(token_end-token, "AzTools::getString2"); 

  *wpp = token_end; 
  return token; 
}

/*--------------------------------------------------------*/
void AzTools::getStrings(const AzByte *data, int data_len, 
                         AzStrPool *sp_tokens)
{
  if (data_len <= 0) return; 
  const AzByte *wp = data; 
  const AzByte *data_end = data + data_len; 
  for ( ; ; ) {
    int len; 
    const AzByte *str = getString(&wp, data_end, &len); 
    if (len <= 0) break; 
    sp_tokens->put(str, len); 
  }
}

/*--------------------------------------------------------*/
const AzByte *AzTools::strip(const AzByte *data, const AzByte *data_end,  
                               int *byte_len) 
{
  const AzByte *bp; 
  for (bp = data; bp < data_end; ++bp) {
    if (*bp > ' ') break; 
  }
  const AzByte *ep; 
  for (ep = data_end - 1; ep >= data; --ep) {
    if (*ep > ' ') break; 
  }
  ++ep; 

  *byte_len = Az64::ptr_diff(ep-bp, "AzTools::strip"); 

  if (*byte_len < 0) { /* blank line */
    bp = data; 
    *byte_len = 0; 
  }
  return bp; 
}

/*--------------------------------------------------------*/
void AzTools::shuffle(int rand_seed, 
                      AzIntArr *iq, 
                      bool withReplacement)
{
  if (rand_seed > 0) {
    srand(rand_seed); 
  }

  int num = iq->size(); 
  AzIntArr ia_temp; 

  int unit = 0; 

  ia_temp.reset(num, AzNone); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    for ( ; ; ) {
      int rand_no = rand_large(); 
      rand_no = rand_no % num; 
      if (withReplacement) {
        ia_temp.update(ix, iq->get(rand_no)); 
        break; 
      }

      if (ia_temp.get(rand_no) == AzNone) {
        ia_temp.update(rand_no, iq->get(ix)); 
        break; 
      }
    }

    if (unit > 0 && !log_out.isNull()) {
      if ((ix+1) % unit == 0) {
        AzPrint::write(log_out, "."); 
        log_out.flush(); 
      }
    }
  }
  if (unit > 0 && !log_out.isNull()) {
    AzPrint::writeln(log_out, ""); 
    log_out.flush(); 
  }

  iq->reset(); 
  iq->concat(&ia_temp); 
}

/*--------------------------------------------------------*/
void AzTools::shuffleFile(const char *fn, 
                         int buff_size,               
                         int random_seed, 
                         const char *out_fn)
{
  AzByte *buff = NULL; 
  AzBaseArray<AzByte> _a(buff_size, &buff); 

  AzFile out_file(out_fn); 
  out_file.open("wb"); 

  AzIIarr iia_offs_len; 
  AzFile file(fn); 
  file.open("X"); 
  int offs = 0; 
  int lx = 0; 
  for ( ; ; ++lx) {
    int len = file.gets(buff, buff_size); 
    if (len <= 0) break; 
    iia_offs_len.put(offs, len); 
    offs += len; 
  }

  int line_num = iia_offs_len.size(); 
  AzIntArr ia_lx;   
  ia_lx.range(0, line_num); 
  shuffle(random_seed, &ia_lx); 

  const int *my_lx = ia_lx.point(); 
  int ix; 
  for (ix = 0; ix < line_num; ++ix) {
    lx = my_lx[ix]; 
    int offs, len; 
    iia_offs_len.get(lx, &offs, &len); 

    file.seekReadBytes(offs, len, buff); 
    out_file.writeBytes(buff, len); 
  }
  file.close(); 
  out_file.close(true); 
}

/*------------------------------------------------------------------*/
void AzTools::formatRvector(const AzSvect *v, 
                            const char *dlm, 
                            AzBytArr *s,  /* appended */
                            int exclude)
{
  int row_num = v->rowNum(); 
  AzCursor cursor; 
  int curr = 0; 
  for ( ; ; ) {
    double val; 
    int ex = v->next(cursor, val); 
    if (ex < 0) break; 

    for ( ; curr < ex; ++curr) {
      if (curr != exclude) {
        s->c("0"); s->c(dlm); 
      }
    }
    int width = 8; 
    if (ex != exclude) {
      s->concatFloat(val, width); s->c(dlm); 
    }
    curr = ex + 1; 
  }
  for ( ; curr < row_num; ++curr) {
    if (curr != exclude) {
      s->c("0"); s->c(dlm); 
    }
  }
}

/*------------------------------------------------------------------*/
void AzTools::formatRvector(const AzDvect *v, 
                            const char *dlm, 
                            AzBytArr *s)  /* appended */
{
  int row_num = v->rowNum(); 
  const double *val = v->point(); 
  int row; 
  for (row = 0; row < row_num; ++row) {
    s->concatFloat(val[row], 8); s->c(dlm); 
  }
}

/*------------------------------------------------*/
void AzTools::filter_exclude(const AzIntArr *ia_fxs, 
                               AzSmat *m_x)
{
  if (ia_fxs->size()==0) return; 
  AzSmat m_x_tran; 
  m_x->transpose(&m_x_tran); 
  int ix; 
  for (ix = 0; ix < ia_fxs->size(); ++ix) {
    int fx = ia_fxs->get(ix); 
    m_x_tran.col_u(fx)->clear(); 
  }
  m_x_tran.transpose(m_x); 
}

/*------------------------------------------------*/
void AzTools::filter_include(const AzIntArr *ia_fxs, 
                               AzSmat *m_x)
{
  if (ia_fxs->size()==0) return; 
  AzSmat m_x_tran; 
  m_x->transpose(&m_x_tran);

  AzIntArr ia_onOff; 
  ia_fxs->toOnOff(&ia_onOff); 
  int fx; 
  for (fx = 0; fx < m_x_tran.colNum(); ++fx) {
    if (fx>=ia_onOff.size() || ia_onOff.get(fx)==0) {
      m_x_tran.col_u(fx)->clear(); 
    }
  }
  m_x_tran.transpose(m_x); 
}

/*------------------------------------------------*/
void AzTools::filter_include(const AzIntArr *ia_fxs, 
                             const AzSmat *m_x, 
                             AzSmat *m_out) 
{
  if (ia_fxs->size()==0) return; 
  AzSmat m_x_tran; 
  m_x->transpose(&m_x_tran);

  AzSmat m_out_tran(m_x_tran.rowNum(), ia_fxs->size()); 
  int ix; 
  for (ix = 0; ix < ia_fxs->size(); ++ix) {
    int fx = ia_fxs->get(ix);  
    m_out_tran.col_u(ix)->set(m_x_tran.col(fx)); 
  }
  m_out_tran.transpose(m_out); 
}

/*------------------------------------------------*/
void AzTools::pickData(AzSmat *m_inout, 
                       const AzIntArr *ia_cols)
{
  AzSmat m(m_inout); 
  m_inout->reform(m.rowNum(), ia_cols->size()); 
  int ix; 
  for (ix = 0; ix < ia_cols->size(); ++ix) {
    int col = ia_cols->get(ix); 
    m_inout->col_u(ix)->set(m.col(col)); 
  }  
}

/*------------------------------------------------*/
void AzTools::pickData(AzDvect *v_inout, 
                       const AzIntArr *ia_cols)
{
  AzDvect v(v_inout); 
  v_inout->reform(ia_cols->size()); 
  int ix; 
  for (ix = 0; ix < ia_cols->size(); ++ix) {
    int col = ia_cols->get(ix); 
    v_inout->set(ix, v.get(col)); 
  }  
}

/*------------------------------------------------*/
void AzTools::sample(int nn, int kk, AzIntArr *ia) 
{ 
  if (kk >= nn) {
    ia->range(0, nn); 
    return; 
  }
  ia->reset(); 
  ia->prepare(kk); 
  AzIntArr ia_is_used; ia_is_used.reset(nn, false); 
  int *is_used = ia_is_used.point_u(); 
  for ( ; ; ) {
    int val = rand_large() % nn; 
    if (!is_used[val]) {
      ia->put(val); 
      is_used[val] = true;  
      if (ia->size() >= kk) break; 
    }
  }
}

/*------------------------------------------------*/
void AzTools::colSum(const AzSmat &m, AzDvect *v_sum)
{
  v_sum->reform(m.colNum()); 
  double *sum = v_sum->point_u(); 
  int col; 
  for (col = 0; col < m.colNum(); ++col) {
    sum[col] = m.sum(col); 
  }
}

/*------------------------------------------------*/
void AzTools::rowSum(const AzSmat &m, AzDvect *v_sum)
{
  v_sum->reform(m.rowNum()); 
  int col; 
  for (col = 0; col < m.colNum(); ++col) {
    v_sum->add(m.col(col)); 
  }
}
