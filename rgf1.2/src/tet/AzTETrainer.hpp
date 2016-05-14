/* * * * *
 *  AzTETrainer.hpp 
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

#ifndef _AZ_TETRAINER_HPP_
#define _AZ_TETRAINER_HPP_

#include "AzUtil.hpp"
#include "AzBmat.hpp"
#include "AzLoss.hpp"
#include "AzHelp.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTE_ModelInfo.hpp"
#include "AzTreeEnsemble.hpp"

/*-------------------------------------------------------*/
enum AzTETrainer_Ret {
  AzTETrainer_Ret_TestNow = 1, 
  AzTETrainer_Ret_Exit = 2
}; 
/*-------------------------------------------------------*/
class AzTETrainer; 
class AzTETrainer_TestData {
public:
  AzTETrainer_TestData() : data(NULL), _t(0) {}
  AzTETrainer_TestData(const AzOut &out, AzSmat *m_test_x) 
                         : data(NULL), _t(0) { 
    reset(out, m_test_x);  
  }
  void reset(const AzOut &out, 
             AzSmat *m_test_x) {
    if (m_test_x == NULL) {
      throw new AzException("AzTETrainer_TestData::reset", "test input is null"); 
    }
    data_dflt.reset_data_for_test(out, m_test_x); 
    data = &data_dflt; 
    m_test_x->destroy(); 
    _t = 0; _b.reset(); _v.reform(0); 
  }

  friend class AzTETrainer; 

protected: 
  AzDataForTrTree data_dflt; 
  const AzDataForTrTree *data;

  AzBmat _b;  //!< used by AzTETrainer only 
  AzDvect _v; //!< used by AzTETrainer only 
  int _t;     //!< used by AzTETrainer only 
}; 

//! Abstract class: interface of Tree Ensemble Trainer (e.g., RGF, Gradient Boost, etc.)
/*-------------------------------------------------------*/
/* Abstract class: Tree ensemble trainer */
class AzTETrainer {
public:
  /*---  training  ---*/
  //! Start training.  Preparation.   
  virtual void startup(
              const AzOut &out, //!<where to write log 
              const char *param,            //!< parameter 
              AzSmat *m_x,   //!<training data; will be destroyed
              AzDvect *v_y,  //!<training targets; will be destroyed
              const AzSvFeatInfo *featInfo=NULL, //!<feature info
              AzDvect *v_data_weights=NULL,  //!<data point weights; will be destroyed
              AzTreeEnsemble *inp_ens=NULL) //!< for warm start; will be destroyed
              = 0; 

  //! Do training until it's over or it's time to test.  
  virtual AzTETrainer_Ret proceed_until() = 0; 

  //! Evaluate the tree ensemeble at the current stage of training.  
  virtual void 
  apply(AzTETrainer_TestData *td, /*!< test data */
        AzDvect *v_p,             /*!< output: prediction on the test data */
        AzTE_ModelInfo *info=NULL, 
        AzTreeEnsemble *out_ens=NULL) 
        const = 0; 

  //! Return loss type. 
  virtual AzLossType lossType() const = 0; 

  //! Print help.  
  virtual void printHelp(AzHelp &h) const = 0; 

  //! Copy the tree ensemble at the current stage of training.  
  virtual void copy_to(AzTreeEnsemble *out_ens) const = 0; 

  //! Algorithm description. 
  virtual const char *description() const = 0;         

protected:

/*------------------------------------------------------------------*/
  virtual void check_data_consistency(
                    const AzSmat *m_x, 
                    const AzDvect *v_y, 
                    const AzDvect *v_fixed_dw, 
                    const AzSvFeatInfo *featInfo, 
                    const char *eyec) const
  {
    if (v_y->rowNum() != m_x->colNum()) {
      throw new AzException(AzInputError, eyec, "#data conflict"); 
    }
    if (featInfo != NULL && featInfo->featNum() != m_x->rowNum()) {
      throw new AzException(AzInputError, eyec, "#feat conflict"); 
    }
    if (!AzDvect::isNull(v_fixed_dw)) {
      if (v_fixed_dw->rowNum() != v_y->rowNum()) {
        throw new AzException(AzInputError, eyec, "Dimensionality conflict: y and fixed_dw"); 
      }
      if (v_fixed_dw->min() <= 0) {
        throw new AzException(AzInputError, eyec, "Data point weights must be positive"); 
      }
    }

  }

  /*-----*/
  inline const AzDataForTrTree *_data(AzTETrainer_TestData *td) const {
    if (td->data == NULL) {
      throw new AzException(AzInputError, "AzTETrainer", "test input is null"); 
    }
    return td->data; 
  }

  /*-----*/
  //! to access AzTETrainer_TestData's work area for faster testing 
  inline AzBmat *_b(AzTETrainer_TestData *td) const {
    return &td->_b; 
  }
  //! to access AzTETrainer_TestData's work area for faster testing 
  inline AzDvect *_v(AzTETrainer_TestData *td) const {
    return &td->_v; 
  }
  //! to access AzTETrainer_TestData's work area for faster testing 
  inline int *_t(AzTETrainer_TestData *td) const {
    return &td->_t; 
  }
}; 

#endif
