/* * * * *
 *  AzSvDataS.cpp 
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

#include "AzSvDataS.hpp"
#include "AzTools.hpp"

/*------------------------------------------------------------------*/
void AzSvDataS::reset()
{
  m_feat.reform(0,0);  
  sp_f_dic.reset(); 
  v_y.reform(0); 
}

/*------------------------------------------------------------------*/
void AzSvDataS::destroy()
{
  reset(); 
}

/*------------------------------------------------------------------*/
void AzSvDataS::checkIfReady(const char *msg) const
{
  if (m_feat.colNum() <= 0 || 
      m_feat.colNum() != v_y.rowNum() || 
      sp_f_dic.size() > 0 && sp_f_dic.size() != m_feat.rowNum()) {
    throw new AzException("AzSvDataS::checkIfReady", "failed", msg); 
  }
}

/*------------------------------------------------------------------*/
void AzSvDataS::concatDesc(int ex, AzBytArr *str_desc) const
{
  if (ex < 0 || ex >= m_feat.rowNum()) {
    str_desc->concatInt(ex); 
    str_desc->concat("?"); 
    return; 
  }
  
  if (sp_f_dic.size() > 0) {
    str_desc->concat(sp_f_dic.c_str(ex)); 
  }
  else {
    str_desc->concat("T"); 
    str_desc->concatInt(ex); 
  }
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
void AzSvDataS::_read(const char *feat_fn, 
                     const char *y_fn, 
                     const char *fdic_fn,
                     int max_data_num)
{
  reset(); 

  read_feat(feat_fn, fdic_fn, &m_feat, &sp_f_dic, max_data_num); 
  read_target(y_fn, &v_y, max_data_num); 

  /*---  check the dimensionalty  ---*/
  int f_data_num = m_feat.colNum(); 
  int y_data_num = v_y.rowNum(); 
  if (f_data_num != y_data_num) {
    AzBytArr s("Data conflict: "); 
    s.c(feat_fn); s.c(" has "); s.cn(f_data_num); s.c(" data points, whereas "); 
    s.c(y_fn);    s.c(" has "); s.cn(y_data_num); s.c(" data points."); 
    throw new AzException(AzInputNotValid, "AzSvDataS::read", s.c_str()); 
  }
}

/*------------------------------------------------------------------*/
void AzSvDataS::read_features_only(const char *feat_fn, 
                                   const char *fdic_fn,
                                   int max_data_num)
{
  reset(); 
  read_feat(feat_fn, fdic_fn, &m_feat, &sp_f_dic, max_data_num); 
  int data_num = m_feat.colNum(); 
  v_y.reform(data_num); /* fill with dummy value zero */
}

/*------------------------------------------------------------------*/
void AzSvDataS::read_targets_only(const char *y_fn, 
                                  int max_data_num)
{
  reset(); 
  read_target(y_fn, &v_y, max_data_num); 
  int data_num = v_y.rowNum(); 
  m_feat.reform(1, data_num); /* dummy features */
}

/*------------------------------------------------------------------*/
/* static */
void AzSvDataS::read_feat(const char *feat_fn, 
                          const char *fdic_fn, 
                          /*---  output  ---*/
                          AzSmat *m_feat, 
                          AzStrPool *sp_f_dic, 
                          int max_data_num)
{
  /*---  read feature names  ---*/
  int f_num = -1; 
  if (fdic_fn != NULL && strlen(fdic_fn) > 0) {
    AzTools::readList(fdic_fn, sp_f_dic); 
    if (sp_f_dic->size() > 0) {
      f_num = sp_f_dic->size(); 
      int fx; 
      for (fx = 0; fx < sp_f_dic->size(); ++fx) {
        if (sp_f_dic->getLen(fx) <= 0) {
          AzBytArr s("No blank line is allowed in feature name files: "); s.c(fdic_fn); 
          throw new AzException(AzInputNotValid, "AzSvDataS::read_feat", s.c_str()); 
        }
      }
    }
  }

  /*---  read feature file  ---*/
  readData(feat_fn, -1, m_feat, max_data_num); 

  if (f_num > 0 && f_num != m_feat->rowNum()) {
    AzBytArr s("Conflict in #feature: "); s.c(feat_fn); s.c(" vs. "); s.c(fdic_fn); 
    throw new AzException(AzInputNotValid, "AzSvDataS::read_feat", s.c_str()); 
  }
}

/*------------------------------------------------------------------*/
/* static */
void AzSvDataS::read_target(const char *y_fn, AzDvect *v_y, int max_data_num)
{
  AzSmat m_y; 
  readData(y_fn, 1, &m_y, max_data_num); 
  int y_data_num = m_y.colNum(); 
  v_y->reform(y_data_num); 
  int dx; 
  for (dx = 0; dx < y_data_num; ++dx) {
    double val = m_y.get(0, dx); 
    v_y->set(dx, val); 
  }
}                       

/*------------------------------------------------------------------*/
void AzSvDataS::_parseDataLine(const AzByte *inp, 
                              int inp_len, 
                              int f_num, 
                              const char *data_fn, 
                              int line_no, 
                              /*---  output  ---*/
                              AzIFarr &ifa_ex_val)
{
  const char *eyec = "AzSvDataS::_parseDataLine"; 

  ifa_ex_val.prepare(f_num); 
  const AzByte *wp = inp, *line_end = inp + inp_len; 
//  AzIFarr ifa_ex_val; 
  int ex = 0; 
  for ( ; ; ) {
    if (wp >= line_end) break; 

#if 1
    int str_len; 
    const AzByte *str = AzTools::getString(&wp, line_end, &str_len); 
    if (str_len > 0) {
      if (ex >= f_num) {
        AzBytArr s("Error in "); s.c(data_fn); s.c(": Line#="); s.cn(line_no); 
        AzPrint::writeln(log_out, s); 
        s.reset(); s.c("Too many values per line: expected "); s.cn(f_num); s.c(" values.");  
        throw new AzException(AzInputNotValid, eyec, s.c_str()); 
      }
      
#if 1
      double val = my_atof((char *)str, eyec, line_no);    
#else
      double val = atof((char *)str);      
      if (val == 0 && *str != '0' && *str != '+' && *str != '-') {
        AzBytArr s("Invalid token "); s.c(str, str_len); s.c(" in "); s.c(data_fn); s.c(": Line#="); s.cn(line_no); 
        AzPrint::writeln(log_out, s); 
        throw new AzException(AzInputNotValid, eyec, s.c_str()); 
      } 
#endif       
      
      if (val != 0) {
        ifa_ex_val.put(ex, val); 
      }
      ++ex;
    }
#else    
    AzBytArr str_token; 
    AzTools::getString(&wp, line_end, &str_token); 
    if (str_token.getLen() > 0) {
      if (ex >= f_num) {
        AzBytArr s("Error in "); s.c(data_fn); s.c(": Line#="); s.cn(line_no); 
        AzPrint::writeln(log_out, s); 
        s.reset(); s.c("Too many values per line: expected "); s.cn(f_num); s.c(" values.");  
        throw new AzException(AzInputNotValid, eyec, s.c_str()); 
      }
      /* double val = atof(str_token.c_str()); */
      double val = my_atof(str_token.c_str(), eyec, line_no);    
      if (val != 0) {
        ifa_ex_val.put(ex, val); 
      }
      ++ex; 
    }
#endif     
  }
  if (ex < f_num) {
    AzTimeLog::print("Error in Line#=", line_no, log_out); 
    throw new AzException(AzInputNotValid, eyec, "Too few values"); 
  }

//  m_feat->load(col, &ifa_ex_val); 
}

/*------------------------------------------------------------------*/
int AzSvDataS::countFeatures(const AzByte *line, 
                             const AzByte *line_end)
{
  const AzByte *wp = line; 
  int count = 0; 
  for ( ; wp < line_end; ) {
    AzBytArr s; 
    AzTools::getString(&wp, line_end, &s); 
    if (s.length() > 0) {
      ++count; 
    }
  }

  return count;
}

#if 0 
/*------------------------------------------------------------------*/
void AzSvDataS::readData_Small(const char *data_fn, 
                         int expected_f_num, 
                         /*---  output  ---*/
                         AzSmat *m_feat)
{
  const char *eyec = "AzSvDataS::readData_Small"; 

  AzFile file(data_fn); 
  file.open("rb"); 
  int file_size = file.size(); 
  AzBytArr bq_data; 
  AzByte *data = bq_data.reset(file_size+1, 0); 
  file.seekReadBytes(0, file_size, data); 
  file.close(); 

  const char *data_end = (char *)data + file_size; 
  const char *wp = (char *)data; 
  AzIIarr iia_begin_end; 
  for ( ; ; ) {
    if (wp >= data_end) break; 
    const char *next_wp = strchr(wp, '\n'); 
    if (next_wp == NULL) next_wp = data_end; 
    iia_begin_end.put(Az64::ptr_diff(wp-(char *)data), 
                      Az64::ptr_diff(next_wp-(char *)data)); 
    wp = next_wp+1; 
  }

  int data_num = iia_begin_end.size(); 
  if (data_num <= 0) {
    throw new AzException(AzInputNotValid, eyec, "Empty data"); 
  }

  bool isSparse = false; 
  int offs0, offs1; 
  iia_begin_end.get(0, &offs0, &offs1); 
  AzBytArr s_first_line(data+offs0, offs1-offs0); 
  int f_num = if_sparse(s_first_line, expected_f_num); 
  if (f_num > 0) {
    isSparse = true; 
    --data_num; /* b/c 1st line is information */
    if (data_num <= 0) {
      throw new AzException(AzInputNotValid, eyec, "Empty sparse data file"); 
    }
  }
  else {
    f_num = expected_f_num; 
    if (f_num <= 0) {
      f_num = countFeatures(data+offs0, data+offs1); 
    }
    if (f_num <= 0) {
      throw new AzException(AzInputNotValid, eyec, "No feature in the first line"); 
    }
  }

  m_feat->reform(f_num, data_num); 

  /*---  read features  ---*/
  int dx; 
  for (dx = 0; dx < data_num; ++dx) {
    int line_no = dx + 1; 
    if (isSparse) {
      int offs0, offs1; 
      iia_begin_end.get(dx+1, &offs0, &offs1); /* +1 for 1st line */
      parseDataLine_Sparse(data+offs0, offs1-offs0, f_num, data_fn, 
                    line_no+1, /* "+1" for the header */
                    m_feat, dx); 
    }
    else {
      int offs0, offs1; 
      iia_begin_end.get(dx, &offs0, &offs1); 
      parseDataLine(data+offs0, offs1-offs0, f_num, data_fn, line_no, 
                    m_feat, dx); 
    }
  }
}
#endif 

/*------------------------------------------------------------------*/
void AzSvDataS::readData_Large(const char *data_fn, 
                         int expected_f_num, 
                         /*---  output  ---*/
                         AzSmat *m_feat,
                         int max_data_num)
{
  const char *eyec = "AzSvDataS::readData_Large"; 

  /*---  find the number of lines and the maximum line length  ---*/
  AzIntArr ia_line_len; 
  AzFile::scan(data_fn, 1024*1024, &ia_line_len, max_data_num+1);  /* +1 for sparse indicator */
  
  int data_num = ia_line_len.size(); 
  int max_line_len = ia_line_len.max(); 
  if (data_num <= 0) {
    throw new AzException(AzInputNotValid, eyec, "Empty data"); 
  }

  AzBytArr ba_buff; 
  AzByte *buff = ba_buff.reset(max_line_len+256, 0); /* +256 just in case */
  
  /*---  1st line indicates sparse/dense  ---*/
  AzFile file(data_fn); 
  
  file.open("rb"); 
  int line0_len = file.gets(buff, max_line_len); 
  AzBytArr s_line0(buff, line0_len); 
  
  int line_no = 0; 
  bool isSparse = false;
  int f_num = if_sparse(s_line0, expected_f_num); 
  if (f_num > 0) {
    isSparse = true; 
    --data_num; /* b/c 1st line is information */
    if (data_num <= 0) {
      throw new AzException(AzInputNotValid, eyec, "Empty sparse data file"); 
    }
    line_no = 1; /* first data line */
  }
  else {
    f_num = expected_f_num; 
    if (f_num <= 0) {
      const AzByte *line0 = s_line0.point(); 
      f_num = countFeatures(line0, line0+line0_len); 
    }
    if (f_num <= 0) {
      throw new AzException(AzInputNotValid, eyec, "No feature in the first line"); 
    }
    file.seek(0);  /* rewind to the beginning */
  }

  /*---  read features  ---*/
  if (max_data_num > 0) {
    data_num = MIN(data_num, max_data_num); 
  }
  m_feat->reform(f_num, data_num); 
  
  int dx; 
  for (dx = 0; dx < data_num; ++dx, ++line_no) {
    int len = ia_line_len.get(line_no); 
    file.readBytes(buff, len); 
    buff[len] = '\0';  /* to make it a C string */
    if (isSparse) {
      parseDataLine_Sparse(buff, len, f_num, data_fn, line_no+1,
                    m_feat, dx); 
    }
    else {
      parseDataLine(buff, len, f_num, data_fn, line_no+1, 
                    m_feat, dx); 
    }
  }
  file.close(); 
}                            

/*------------------------------------------------------------------*/
void AzSvDataS::_parseDataLine_Sparse(const AzByte *inp, 
                              int inp_len, 
                              int f_num, 
                              const char *data_fn, 
                              int line_no, 
                              /*---  output  ---*/
                              AzIFarr &ifa_ex_val) 
{
  const char *eyec = "AzSvDataS::_parseDataLine_Sparse"; 

  AzIntArr ia_isUsed; 
  ia_isUsed.reset(f_num, 0); 
  int *isUsed = ia_isUsed.point_u(); 

  const AzByte *wp = inp, *line_end = inp + inp_len; 
//  AzIFarr ifa_ex_val; 
  for ( ; ; ) {
    if (wp >= line_end) break; 

    AzBytArr str_token; 
    AzTools::getString(&wp, line_end, &str_token); 
    if (str_token.getLen() > 0) {
      int ex; 
      double val; 
      decomposeFeat(str_token.c_str(), line_no, &ex, &val); 
      if (ex < 0 || ex >= f_num) {
        AzBytArr s("Error in line# "); s.cn(line_no); 
        s.c(": invalid feature# "); s.cn(ex); 
        throw new AzException(AzInputError, eyec, s.c_str());     
      }
      if (isUsed[ex]) {
        AzBytArr s("Error in line# "); s.cn(line_no); 
        s.c(": feature# "); s.cn(ex); s.c(" appears more than once."); 
        throw new AzException(AzInputError, eyec, s.c_str()); 
      }
      if (val != 0) {
        ifa_ex_val.put(ex, val); 
      }
      isUsed[ex] = 1; 
    }
  }
//  m_feat->load(col, &ifa_ex_val); 
}

/*-------------------------------------------------------------*/
/* ex:val */
void AzSvDataS::decomposeFeat(const char *token,
                              int line_no,  
                              /*---  output  ---*/
                              int *ex, 
                              double *val)
{
  /* *ex = atol(token); */
  *ex = my_fno(token, "AzSvDataS::decomposeFeat", line_no); 

  *val = 1; 
  const char *ptr = strchr(token, ':');   
  if (ptr != NULL) {
    /* *val = atof(ptr + 1); */
    *val = my_atof(ptr+1, "AzSvDataS::decomposeFeat", line_no); 
  }
}

/*------------------------------------------------------------------*/
int AzSvDataS::if_sparse(AzBytArr &s_line, 
                         int expected_f_num, 
                         const char *str)
{
  const char *eyec = "AzSvDataS::if_sparse"; 

  int sparse_f_num = -1; 
  AzBytArr s_sparse(str);  
  AzStrPool sp_tok; 
  AzTools::getStrings(s_line.point(), s_line.length(), &sp_tok); 
  if (sp_tok.size() > 0 && 
      s_sparse.compare(sp_tok.c_str(0)) == 0) {
    if (sp_tok.size() >= 2) {
      sparse_f_num = atol(sp_tok.c_str(1)); 
    }
    if (sparse_f_num <= 0) {
      throw new AzException(AzInputError, eyec, 
            "1st line of sparse data file must be \"sparse dd\" where dd is the feature dimensionality."); 
    }
    if (expected_f_num > 0 && sparse_f_num != expected_f_num) {
      throw new AzException(AzInputError, eyec, 
            "Conflict in feature dim: feature definition file vs. data file.");
    }
  }
  return sparse_f_num; 
}


/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/* static */
void AzSvDataS::mergeData(int argc, 
                         const char *argv[])
{
  if (argc < 9) {
    cout << "x_fn x_names_fn fn_template str doSparse digits out_x_fn out_n_fn nm1 [nm2 ...]"<<endl; 
    return; 
  }
  int argx = 0; 
  const char *x_fn = argv[argx++]; 
  const char *x_names_fn = argv[argx++]; 
  const char *fn_template = argv[argx++]; 
  const char *str = argv[argx++]; 
  const char *doSparse_str = argv[argx++]; 
  int digits = atol(argv[argx++]); 
  const char *out_x_fn = argv[argx++]; 
  const char *out_n_fn = argv[argx++]; 
  int num = argc-argx; 
  const char **names = argv+argx; 
  bool doSparse = false; 
  if (*doSparse_str == '1' || *doSparse_str == 'Y') doSparse = true; 
  mergeData(x_fn, x_names_fn, fn_template, str, doSparse, digits, out_x_fn, out_n_fn, num, names); 
}

/*----------------------------------------------------------------------*/
void AzSvDataS::mergeData(const char *x_fn, 
                         const char *x_names_fn, 
                         const char *fn_template, 
                         const char *str, 
                         bool doSparse, 
                         int digits, 
                         const char *out_x_fn, 
                         const char *out_n_fn, 
                         int num,
                         const char *names[])
{
  AzSvDataS dataset; 
  dataset.read_features_only(x_fn, x_names_fn); 
  mergeData(dataset.feat(), dataset.featInfo(), fn_template, str, 
            doSparse, digits, out_x_fn, out_n_fn, num, names); 
}

/*----------------------------------------------------------------------*/
void AzSvDataS::mergeData(const AzSmat *m_x, 
                         const AzSvFeatInfo *feat, 
                         const char *fn_template, 
                         const char *str, 
                         bool doSparse, 
                         int digits, 
                         const char *out_x_fn, 
                         const char *out_n_fn,
                         int num,
                         const char *names[])
{
  const char *eyec = "AzSvDataS::mergeData"; 

  int data_num = m_x->colNum(); 
  int f_num = m_x->rowNum(); 
  if (feat->featNum() != f_num) {
    throw new AzException(eyec, "Conflict btw m_x and featInfo"); 
  }
  AzFile n_file(out_n_fn); 
  n_file.open("wb"); 
  int fx; 
  for (fx = 0; fx < feat->featNum(); ++fx) {
    AzBytArr s; feat->desc(fx, &s); s.nl(); 
    s.writeText(&n_file); 
  }

  AzSmat m;
  m_x->transpose(&m); 
  m.resize(data_num, f_num+num); 
  AzStrPool sp_names; 
  for (fx = 0; fx < num; ++fx) {
    AzBytArr s_fn(fn_template); 
    s_fn.replace("*", names[fx]); 

    AzDvect v; 
    AzSvDataS::readVector(s_fn.c_str(), &v); 
    if (v.rowNum() != m.rowNum()) {
      throw new AzException(AzInputError, eyec, "conflict in #data:", s_fn.c_str()); 
    }
    m.col_u(f_num+fx)->set(&v);    

    AzBytArr s_nm;
    if (AzTools::isSpecified(str)) s_nm.c(str);  
    s_nm.c(names[fx]); s_nm.nl(); 
    s_nm.writeText(&n_file);  
  }
  n_file.close(true); 

  AzSmat m1; 
  m.transpose(&m1); 
  m1.writeText(out_x_fn, digits, doSparse); 
}
 