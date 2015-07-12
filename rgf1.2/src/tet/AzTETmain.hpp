/* * * * *
 *  AzTETmain.hpp 
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

#ifndef _AZ_TET_MAIN_HPP_
#define _AZ_TET_MAIN_HPP_

#include "AzUtil.hpp"
#include "AzTETselector.hpp"
#include "AzTETrainer.hpp"
#include "AzTETmain_kw.hpp"
#include "AzTET_Eval.hpp"
#include "AzSvDataS.hpp"

#include <ctime>

//! Call tree ensemble trainer.
class AzTETmain {
protected: 
  AzBytArr s_alg_name, s_train_x_fn, s_train_y_fn, s_fdic_fn, s_pred_fn; 
  AzBytArr s_dw_fn; 
  AzBytArr s_pred_fn_suffix; 
  AzBytArr s_eval_fn; 
  AzBytArr s_model_stem, s_model_names_fn, s_model_fn; 
  AzBytArr s_prev_model_fn; 
  AzBytArr s_tet_param; 
  bool doLog, doDump; 
  bool doAppend_eval; 
  bool doSaveLastModelOnly; 
  const AzTETselector *alg_sel; 

  AzBytArr s_test_x_fn, s_test_y_fn; 

  AzTET_Eval *eval; 

  AzBytArr s_xv_fn; 
  bool xv_doShuffle; 
  int xv_num; 

  AzBytArr s_input_x_fn, s_output_x_fn; 
  bool doSparse_features; 
  int features_digits; 
public:
  AzTETmain(const AzTETselector *inp_alg_sel, 
            AzTET_Eval *inp_eval) : s_model_stem(dflt_model_stem), eval(NULL), 
                                    doLog(true), doDump(false), doAppend_eval(false), 
                                    doSaveLastModelOnly(false), 
                                    xv_doShuffle(false), xv_num(2), 
                                    doSparse_features(false), features_digits(10)
  {
    alg_sel = inp_alg_sel; 
    eval = inp_eval; 
    s_alg_name.reset(alg_sel->dflt_name()); 
  }
 
  virtual void train(const char *argv[], int argc); 
  virtual void train_test(const char *argv[], int argc); 
  virtual void train_predict(const char *argv[], int argc, bool do2=false); 
  virtual void batch_predict(const char *argv[], int argc); 
  virtual void predict_single(const char *argv[], int argc); 
  virtual void xv(const char *argv[], int argc); 

  virtual void features(const char *argv[], int argc); 

  virtual void printHelp_train(const AzOut &out, 
                               const char *argv[], int argc, 
                               bool for_train_test, 
                               bool for_train_predict=false) const; 
  virtual void printHelp_predict_single(const AzOut &out, 
                               const char *argv[], int argc) const; 
  virtual void printHelp_batch_predict(const AzOut &out, 
                               const char *argv[], int argc) const; 
  virtual void printHelp_xv(const AzOut &out, 
                            const char *argv[], int argc) const {
    throw new AzException("AzTETmain::printHelp_xv", "No help for xv"); 
  }

  static void writePrediction_single(const AzDvect *v_p, 
                                     AzFile *file);

  static void writeEvaluation(const AzTE_ModelInfo *info, 
                                const char *model_fn, 
                                const AzDvect *v_p, 
                                const AzDvect *v_y, 
                                AzFile *file); 

protected:
  virtual void readData(const char *x_fn, 
                         const char *y_fn, 
                         const char *fdic_fn, 
                         /*---  output  ---*/
                         AzSmat *m_x, 
                         AzDvect *v_y, 
                         AzSvFeatInfoClone *featInfo=NULL) const; 
  virtual void readDataWeights(AzBytArr &s_fn, 
                            int data_num, 
                            AzDvect *v_fixed_dw) const; 

  void prepareLogDmp(bool doLog, bool doDump); 

  virtual bool resetParam_train_predict(const char *argv[], int argc); 
  virtual void printParam_train_predict(const AzOut &out) const; 
  virtual void checkParam_train_predict() const; 

  virtual bool resetParam_train(const char *argv[], int argc, bool for_train_test); 
  virtual bool resetParam_batch_predict(const char *argv[], int argc); 
  virtual bool resetParam_predict_single(const char *argv[], int argc); 

  virtual void checkParam_train(bool for_train_test) const; 
  virtual void printParam_train(const AzOut &out, bool for_train_test) const;

  virtual void checkParam_batch_predict() const; 
  virtual void printParam_batch_predict(const AzOut &out) const; 

  virtual void checkParam_predict_single() const; 
  virtual void printParam_predict_single(const AzOut &out) const; 

  virtual bool resetParam_xv(const char *argv[], int argc); 
  virtual void printParam_xv(const AzOut &out) const; 
  virtual void checkParam_xv() const; 

  virtual bool resetParam_features(const char *argv[], int argc); 
  virtual void printParam_features(const AzOut &out) const; 
  virtual void checkParam_features() const; 
  virtual void printHelp_features(const AzOut &out, 
                const char *argv[], int argc) const; 

  virtual bool isHelpNeeded(const char *param) const; 

  inline virtual void throw_if_missing(const char *kw, const AzBytArr &s_val, 
                                  const char *eyec) const {
    if (s_val.length() <= 0) {
      AzBytArr s_kw; s_kw.inQuotes(kw, "\""); 
      throw new AzException(AzInputMissing, eyec, s_kw.c_str(), "is missing"); 
    }
  }
  virtual void print_usage(const AzOut &out, 
                           const char *argv[], int argc) const; 
  inline static bool isSpecified(const char *str) { 
    if (str == NULL) return false; 
    if (strlen(str) <= 0) return false; 
    return true; 
  }
  virtual void format_info(const char *model_fn, 
                          const AzTreeEnsemble *ens, 
                          const char *name_dlm, 
                          const char *dlm, 
                          AzBytArr *s_info) const; 
  virtual void _predict(const AzSvDataS *dataset, 
                         const char *model_fn, 
                         const char *pred_fn, 
                         const AzOut &out, 
                         bool doEval) const; 

  virtual void print_config(const AzBytArr &s_config, 
                            const AzOut &out) const; 
  virtual void print_hline(const AzOut &out) const; 
  virtual void show_elapsed(const AzOut &out, 
                            clock_t clocks) const; 
}; 

#endif
