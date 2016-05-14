/* * * * *
 *  AzDataForTrTree.hpp 
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

#ifndef _AZ_DATA_FOR_TR_TREE_
#define _AZ_DATA_FOR_TR_TREE_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzSvFeatInfoClone.hpp"
#include "AzSortedFeat.hpp"
#include "AzParam.hpp"
#include "AzHelp.hpp"

#define kw_dataproc  "data_management="
#define help_dataproc "Sparse|Dense|Auto.  Data is treated either as \"Sparse\" data (having many zeroes), as \"Dense\" data, or as \"Auto\"matically determined.  It affects speed and memory consumption of training."

/*--------------------------------------------------------*/
class AzDataForTrTree {
protected:
  int data_num; 
  AzSmat m_tran_sparse; 
  /*-------------------------*/
  AzDmat m_tran_dense;  
  /*
   *  After construction, this matrix is locked so that any operation that 
   *  would change the pointers to column vectors is prohibited, as a safety 
   *  measure, as AzSortedFeatDense keeps those pointers.  
   *  Faster than safer design through abstraction.  
   */
  /*-------------------------*/

  AzSvFeatInfoClone feat; 
  AzSortedFeatArr sorted_arr;  /* not set if this is test data */

  enum dataproc_Type {
    dataproc_Auto = 0, 
    dataproc_Dense = 1, 
    dataproc_Sparse = 2, 
  };

  #define Az_nz_ratio_threshold 0.4
  #define Az_max_test_entries (1024*1024*16)
  dataproc_Type dataproc; 
  AzBytArr s_dataproc; 

public:
  AzDataForTrTree() : dataproc(dataproc_Auto), data_num(0) {}
  virtual void reset_data(const AzOut &out, 
                  const AzSmat *m_data, 
                  AzParam &p, 
                  bool beTight, 
                  const AzSvFeatInfo *inp_feat=NULL)
  {
    resetParam(p); 
    printParam(out); 

    /*---  count nonzero components  ---*/
    double nz_ratio; 
    m_data->nonZeroNum(&nz_ratio); 
    AzBytArr s("Training data: "); 
    s.cn(m_data->rowNum());s.c("x");s.cn(m_data->colNum()); 
    s.c(", nonzero_ratio=", nz_ratio, 4); 

    /*---  decide sparse or dense  ---*/
    AzBytArr s_dp("; managed as dense data"); 
    bool doSparse = false; 
    if (dataproc == dataproc_Auto && 
        nz_ratio < Az_nz_ratio_threshold || 
        dataproc == dataproc_Sparse) { 
      doSparse = true; 
      s_dp.reset("; managed as sparse data"); 
    }
    if (dataproc != dataproc_Auto) s_dp.concat(" as requested."); 
    else                           s_dp.concat("."); 
    AzPrint::writeln(out, "-------------"); 
    AzPrint::writeln(out, s, s_dp); 
    AzPrint::writeln(out, "-------------"); 

   /*---  pre-sort data  ---*/
    m_tran_sparse.reset(); 
    m_tran_dense.unlock(); 
    m_tran_dense.reset(); 
    data_num = m_data->colNum(); 
    if (doSparse) {
      m_data->transpose(&m_tran_sparse); 
      sorted_arr.reset_sparse(&m_tran_sparse, beTight); 
    }
    else {
      m_tran_dense.transpose_from(m_data); 
      sorted_arr.reset_dense(&m_tran_dense, beTight); 
      /* prohibit any action to change the pointers to the column vectors */
      m_tran_dense.lock(); 
    }
    if (inp_feat != NULL) {
      feat.reset(inp_feat); 
      if (feat.featNum() != m_data->rowNum()) {
        throw new AzException(AzInputError, "AzDataForTrTree::reset", "#feat mismatch"); 
      }
    }
    else {
      feat.reset(m_data->rowNum()); 
    }
  }

  virtual void reset_data_for_test(const AzOut &out, 
                     const AzSmat *m_data) {
    bool doSparse = false; 
    if (m_data->rowNum()*m_data->colNum() > Az_max_test_entries) { /* large data */
      /*---  dense is faster but uses up more memory if data is sparse  ---*/
      double nz_ratio; 
      m_data->nonZeroNum(&nz_ratio); 
      if (nz_ratio < 0.6) { /* relatively sparse */
        doSparse = true; 
        AzBytArr s; s.c("Large and sparse test data (nonzero ratio=", nz_ratio); 
        s.c("); treated as sparse data.");
        AzPrint::writeln(out, s); 
      }
    }

    data_num = m_data->colNum(); 
    m_tran_dense.reset(); 
    m_tran_sparse.reset(); 
    if (doSparse) {    
      m_data->transpose(&m_tran_sparse); 
    }
    else {
      m_tran_dense.transpose_from(m_data); 
    }
    sorted_arr.reset(); 
    feat.reset(m_data->rowNum()); 
  }

  virtual inline int dataNum() const {
    return data_num; 
  }
  virtual inline int featNum() const {
    return feat.featNum(); 
  }

  virtual inline bool isLE(int dx, 
              int fx, 
              double border_val) const
  {
    double value; 
    if (AzSmat::isNull(&m_tran_sparse)) {
      value = m_tran_dense.get(dx, fx); 
    }
    else {
      value = m_tran_sparse.get(dx, fx); 
    }
    if (value <= border_val) return true; 
    return false; 
  }

  inline const AzSvFeatInfo *featInfo() const {
    return &feat; 
  }

  virtual inline const AzSortedFeatArr *sorted_array() const {
    return &sorted_arr; 
  }
  virtual inline const AzSortedFeat *sorted(int fx) const {
    return sorted_arr.sorted(fx); 
  }

  /*------------------------------------------------*/
  virtual void printHelp(AzHelp &h) const {
    h.begin("", "AzDataForTrTree", "Data processing"); 
    h.item(kw_dataproc, help_dataproc, "Auto"); 
  }

protected: 
  /*---  for parameters  ---*/
  virtual void resetParam(AzParam &p) {
    p.vStr(kw_dataproc, &s_dataproc); 
    dataproc = dataproc_Auto; 
    if (s_dataproc.length() <= 0 || 
        s_dataproc.compare("Auto") == 0); 
    else if (s_dataproc.compare("Sparse") == 0) dataproc = dataproc_Sparse; 
    else if (s_dataproc.compare("Dense") == 0)  dataproc = dataproc_Dense; 
    else {
      throw new AzException(AzInputNotValid, kw_dataproc, 
            "must be either \"Auto\", \"Sparse\", or \"Dense\"."); 
    }
  }
  virtual void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    if (s_dataproc.length() > 0) {
      o.ppBegin("AzDataForTrTree", "Data processing"); 
      o.printV(kw_dataproc, s_dataproc); 
      o.ppEnd(); 
    }
  }
};  
#endif 
