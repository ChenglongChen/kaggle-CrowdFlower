/* * * * *
 *  AzTETmain.cpp 
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

#include "AzUtil.hpp"
#include "AzSvDataS.hpp"
#include "AzTETmain.hpp"
#include "AzParam.hpp"
#include "AzTETmain_kw.hpp"
#include "AzTaskTools.hpp"
#include "AzHelp.hpp"
#include "AzTETproc.hpp"

static int exe_argx = 0; 
static int action_argx = 1; 
static int config_argx = 2; 

static const char *pred_fn_suffix = ".pred"; 
static const char *info_fn_suffix = ".info"; 

/*------------------------------------------------*/
void AzTETmain::train(const char *argv[], int argc) 
{
  bool for_train_test = false; 
  bool success = resetParam_train(argv, argc, for_train_test); 
  if (!success) return; 

  prepareLogDmp(doLog, doDump);

  printParam_train(log_out, for_train_test); 
  print_hline(log_out); 
  checkParam_train(for_train_test); 

  /*---  read training data  ---*/
  AzDvect v_tr_y, v_fixed_dw; 
  AzSmat m_tr_x; 
  AzSvFeatInfoClone featInfo; 
  AzTimeLog::print("Reading training data ... ", log_out); 
  readData(s_train_x_fn.c_str(), s_train_y_fn.c_str(), s_fdic_fn.c_str(), 
           &m_tr_x, &v_tr_y, &featInfo); 
  readDataWeights(s_dw_fn, v_tr_y.rowNum(), &v_fixed_dw); 

  /*---  for wamr start  ---*/
  AzTreeEnsemble *prev_ens_ptr = NULL, prev_ens; 
  if (s_prev_model_fn.length() > 0) {
    prev_ens.read(s_prev_model_fn.c_str()); 
    prev_ens_ptr = &prev_ens; 
  }

  /*---  select algorithm  ---*/
  AzTETrainer *trainer = alg_sel->select(s_alg_name.c_str()); 

  /*---  training  ---*/
  print_config(s_tet_param, log_out); 
  AzTimeLog::print("Start ... #train=", m_tr_x.colNum(), log_out); 
  print_hline(log_out); 
  clock_t t0 = clock(); 
  AzTETproc::train(log_out, trainer, s_tet_param.c_str(), 
                   &m_tr_x, &v_tr_y, &featInfo, 
                   s_model_stem.c_str(), s_model_names_fn.c_str(), 
                   &v_fixed_dw, prev_ens_ptr); 
  AzTimeLog::print("Done ... ", log_out); 
  clock_t clk = clock() - t0; 
  show_elapsed(log_out, clk); 
}

/*------------------------------------------------------------------*/
void AzTETmain::readData(const char *x_fn, 
                         const char *y_fn, 
                         const char *fdic_fn, 
                         /*---  output  ---*/
                         AzSmat *m_x, 
                         AzDvect *v_y, 
                         AzSvFeatInfoClone *featInfo) /* may be NULL */
const 
{
  AzSvDataS dataset; 
  dataset.read(x_fn, y_fn, fdic_fn); 
  m_x->set(dataset.feat()); 
  v_y->set(dataset.targets()); 
  if (featInfo != NULL) {
    featInfo->reset(dataset.featInfo()); 
  }
}

/*------------------------------------------------------------------*/
void AzTETmain::readDataWeights(AzBytArr &s_fn, 
                            int data_num, 
                            AzDvect *v_fixed_dw)
const
{
  if (s_fn.length() <= 0) return; 
  AzSvDataS::readVector(s_fn.c_str(), v_fixed_dw); 
  if (v_fixed_dw->rowNum() != data_num) {
    AzBytArr s("Conflict in dimensionality: #data = "); s.cn(data_num); 
    s.c(", # of values in "); s.c(&s_fn); s.c(" = "); 
    s.cn(v_fixed_dw->rowNum()); 
    throw new AzException(AzInputNotValid, "AzTETmain::readDataWeights", s.c_str()); 
  }
}

/*------------------------------------------------*/
void AzTETmain::print_config(const AzBytArr &s_config, 
                             const AzOut &out) const 
{
  if (out.isNull()) return; 
  if (out.getLevel() > 0) {
    AzPrint::writeln(out, "core parameters: ", s_config.c_str()); 
    print_hline(out); 
  }
}

/*------------------------------------------------*/
void AzTETmain::print_hline(const AzOut &out) const
{
  if (out.isNull()) return; 
  AzPrint::writeln(out, "--------------------");  
}

/*------------------------------------------------*/
void AzTETmain::predict_single(const char *argv[], int argc)
{
  bool success = resetParam_predict_single(argv, argc); 
  if (!success) return; 

  prepareLogDmp(doLog, doDump);

  printParam_predict_single(log_out); 
  print_hline(log_out); 
  checkParam_predict_single();

  /*---  read test data  ---*/
  AzTimeLog::print("Reading test data ... ", log_out); 
  AzSvDataS dataset; 
  bool doEval = false;
  if (s_test_y_fn.length() > 0) {
    dataset.read(s_test_x_fn.c_str(), s_test_y_fn.c_str());     
    doEval = true; 
    eval->reset(dataset.targets(), s_eval_fn.c_str(), doAppend_eval); 
    eval->begin(); 
  }
  else {
    dataset.read_features_only(s_test_x_fn.c_str()); 
  }

  AzTimeLog::print("Predicting ... ", log_out); 
  _predict(&dataset, s_model_fn.c_str(), s_pred_fn.c_str(), log_out, doEval); 

  if (doEval) {
    eval->end(); 
  }

  AzTimeLog::print("Done ... ", log_out); 
}

/*------------------------------------------------------------------*/
void AzTETmain::writePrediction_single(const AzDvect *v_p, 
                                       AzFile *file)
{
  /* one prediction value per line */
  int width = 8; 
  AzBytArr s; 
  int dx; 
  for (dx = 0; dx < v_p->rowNum(); ++dx) {
    double val = v_p->get(dx); 
    s.concatFloat(val, width); 
    s.nl(); /* new line */
  } 
  s.writeText(file); 
}

/*------------------------------------------------*/
void AzTETmain::_predict(const AzSvDataS *dataset, 
                         const char *model_fn, 
                         const char *pred_fn, 
                         const AzOut &out, 
                         bool doEval) const
{
  AzTreeEnsemble ens(model_fn); 
  if (ens.orgdim() > 0 && 
      ens.orgdim() != dataset->featNum()) {
    AzBytArr s("#feature in test data is "); s.cn(dataset->featNum()); 
    s.c(", whereas #feature in training data was "); s.cn(ens.orgdim()); 
    throw new AzException(AzInputError, "AzTETmain::_predict", s.c_str()); 
  }
  AzDvect v_test_p; 
  clock_t t0 = clock(); 
  ens.apply(dataset->feat(), &v_test_p); 
  clock_t apply_clk = clock() - t0; 

  /*---  write predictions  ---*/
  AzFile pred_file(pred_fn);  
  pred_file.open("wb"); 
  writePrediction_single(&v_test_p, &pred_file); 
  pred_file.close(true); 

  if (!out.isNull()) {
    show_elapsed(out, apply_clk); 
    AzBytArr s(pred_fn); s.c(": "); 
    AzBytArr s_info; 
    format_info(model_fn, &ens, "=", ",", &s_info); 
    AzPrint::writeln(out, s, s_info); 
  }
  if (doEval) {
    /*---  write evaluation if required  ---*/
    AzTE_ModelInfo info; 
    ens.info(&info); 
    eval->evaluate(&v_test_p, &info, model_fn); 
  }
}

/*------------------------------------------------*/
void AzTETmain::batch_predict(const char *argv[], int argc)
{
  bool success = resetParam_batch_predict(argv, argc); 
  if (!success) return; 

  prepareLogDmp(doLog, doDump);

  printParam_batch_predict(log_out); 
  print_hline(log_out); 
  checkParam_batch_predict();

  /*---  read test data  ---*/
  bool doEval = false;
  AzSvDataS dataset; 
  if (s_test_y_fn.length() > 0) {
    dataset.read(s_test_x_fn.c_str(), s_test_y_fn.c_str(), s_fdic_fn.c_str());     
    doEval = true; 
    eval->reset(dataset.targets(), s_eval_fn.c_str(), doAppend_eval); 
    eval->begin(); 
  }
  else {
    dataset.read_features_only(s_test_x_fn.c_str(), s_fdic_fn.c_str()); 
  }

  if (!log_out.isNull()) {
    AzPrint::writeln(log_out, "#test=", dataset.size()); 
  }
  AzStrPool sp_model_fn; 
  AzTools::readList(s_model_names_fn.c_str(), 
                    &sp_model_fn); 
  int num = sp_model_fn.size(); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    const char *model_fn = sp_model_fn.c_str(ix); 
    AzBytArr s_pred_fn(model_fn); 
    s_pred_fn.concat(&s_pred_fn_suffix); 
    _predict(&dataset, model_fn, s_pred_fn.c_str(), log_out, doEval); 
  }
  if (doEval) {
    eval->end(); 
  }
  AzTimeLog::print("Done ... ", log_out); 
}

/*------------------------------------------------*/
void AzTETmain::train_test(const char *argv[], int argc) 
{
  bool for_train_test = true; 
  bool success = resetParam_train(argv, argc, for_train_test); 
  if (!success) return; 

  prepareLogDmp(doLog, doDump);

  printParam_train(log_out, for_train_test); 
  print_hline(log_out); 
  checkParam_train(for_train_test); 

  clock_t clocks = 0; 

  /*---  read training data  ---*/
  AzSmat m_tr_x; 
  AzDvect v_tr_y, v_fixed_dw; 
  AzSvFeatInfoClone featInfo;  
  AzTimeLog::print("Reading training data ... ", log_out); 
  readData(s_train_x_fn.c_str(), s_train_y_fn.c_str(), s_fdic_fn.c_str(), 
           &m_tr_x, &v_tr_y, &featInfo); 
  readDataWeights(s_dw_fn, v_tr_y.rowNum(), &v_fixed_dw); 

  /*---  for wamr start  ---*/
  AzTreeEnsemble *prev_ens_ptr = NULL, prev_ens; 
  if (s_prev_model_fn.length() > 0) {
    prev_ens.read(s_prev_model_fn.c_str()); 
    prev_ens_ptr = &prev_ens; 
  }

  /*---  select algorithm  ---*/
  AzTETrainer *trainer = alg_sel->select(s_alg_name.c_str()); 

  /*---  read test data  ---*/
  AzSmat m_test_x; 
  AzDvect v_test_y;
  AzTimeLog::print("Reading test data ... ", log_out); 
  readData(s_test_x_fn.c_str(), s_test_y_fn.c_str(), "", 
           &m_test_x, &v_test_y); 
 
  eval->reset(&v_test_y, s_eval_fn.c_str(), doAppend_eval); 

  print_config(s_tet_param, log_out); 

  AzBytArr s; 
  s.c("#train=");  s.cn(m_tr_x.colNum()); 
  s.c(", #test="); s.cn(m_test_x.colNum()); 
  AzTimeLog::print("Start ... ", s.c_str(), log_out); 
  print_hline(log_out); 

  clock_t b_clk = clock(); 
  if (s_model_stem.length() > 0) {
    AzTETproc::train_test_save(log_out, trainer, s_tet_param.c_str(), 
                               &m_tr_x, &v_tr_y, &featInfo, 
                               &m_test_x, eval, 
                               doSaveLastModelOnly, 
                               s_model_stem.c_str(), s_model_names_fn.c_str(), 
                               &v_fixed_dw, prev_ens_ptr); 
  }
  else {
    AzTETproc::train_test(log_out, trainer, s_tet_param.c_str(), 
                          &m_tr_x, &v_tr_y, &featInfo, 
                          &m_test_x, eval, &v_fixed_dw, prev_ens_ptr); 
  }
  AzTimeLog::print("Done ...", log_out); 
  clocks += (clock() - b_clk); 
  show_elapsed(log_out, clocks); 
}

/*------------------------------------------------*/
void AzTETmain::xv(const char *argv[], int argc) 
{
  bool for_train_test = true; 
  bool success = resetParam_xv(argv, argc); 
  if (!success) return; 

  prepareLogDmp(doLog, doDump);

  printParam_xv(log_out); 
  print_hline(log_out); 
  checkParam_xv(); 

  clock_t clocks = 0; 

  /*---  read training data  ---*/
  AzSmat m_tr_x; 
  AzDvect v_tr_y, v_fixed_dw; 
  AzSvFeatInfoClone featInfo;  
  AzTimeLog::print("Reading training data ... ", log_out); 
  readData(s_train_x_fn.c_str(), s_train_y_fn.c_str(), s_fdic_fn.c_str(), 
           &m_tr_x, &v_tr_y, &featInfo); 
  readDataWeights(s_dw_fn, v_tr_y.rowNum(), &v_fixed_dw); 

  /*---  select algorithm  ---*/
  AzTETrainer *trainer = alg_sel->select(s_alg_name.c_str()); 

  print_config(s_tet_param, log_out); 

  AzBytArr s; 
  s.c("#data=");  s.cn(m_tr_x.colNum()); 
  AzTimeLog::print("Start cross validation ... ", s.c_str(), log_out); 
  print_hline(log_out); 

  clock_t b_clk = clock(); 
  AzTETproc::xv(log_out, xv_num, s_xv_fn.c_str(), xv_doShuffle, 
                trainer, s_tet_param.c_str(), 
                &m_tr_x, &v_tr_y, &featInfo, &v_fixed_dw); 
  AzTimeLog::print("Done ...", log_out); 
  clocks += (clock() - b_clk); 
  show_elapsed(log_out, clocks); 
}

/*------------------------------------------------------------------*/
void AzTETmain::show_elapsed(const AzOut &out, 
                             clock_t clocks) const
{
  double seconds = (double)clocks / (double)CLOCKS_PER_SEC; 
  AzPrint::writeln(out, "elapsed: ", seconds); 
}

/*------------------------------------------------------------------*/
void AzTETmain::format_info(const char *model_fn, 
                          const AzTreeEnsemble *ens, 
                          const char *name_dlm, 
                          const char *dlm, 
                          AzBytArr *s) const 
{
  AzTE_ModelInfo info; 
  ens->info(&info);
  s->c(model_fn); s->c(dlm); 
  s->c("#leaf"); s->c(name_dlm); s->cn(info.leaf_num); s->c(dlm); 
  s->c("#tree"); s->c(name_dlm); s->cn(info.tree_num);
}

/*------------------------------------------------*/
void AzTETmain::train_predict(const char *argv[], int argc, 
                              bool do2) 
{
  bool success = resetParam_train_predict(argv, argc); 
  if (!success) return; 

  prepareLogDmp(doLog, doDump);

  printParam_train_predict(log_out); 
  print_hline(log_out); 
  checkParam_train_predict(); 

  clock_t clocks = 0; 

  /*---  read training data  ---*/
  AzSmat m_tr_x; 
  AzDvect v_tr_y, v_fixed_dw; 
  AzSvFeatInfoClone featInfo;  
  AzTimeLog::print("Reading training data ... ", log_out); 
  readData(s_train_x_fn.c_str(), s_train_y_fn.c_str(), s_fdic_fn.c_str(), 
           &m_tr_x, &v_tr_y, &featInfo); 
  readDataWeights(s_dw_fn, v_tr_y.rowNum(), &v_fixed_dw); 

  /*---  for wamr start  ---*/
  AzTreeEnsemble *prev_ens_ptr = NULL, prev_ens; 
  if (s_prev_model_fn.length() > 0) {
    prev_ens.read(s_prev_model_fn.c_str()); 
    prev_ens_ptr = &prev_ens; 
  }

  /*---  select algorithm  ---*/
  AzTETrainer *trainer = alg_sel->select(s_alg_name.c_str()); 

  /*---  read test data  ---*/
  AzSmat m_test_x; 
  AzSvDataS dataset; 
  dataset.read_features_only(s_test_x_fn.c_str()); 
  m_test_x.set(dataset.feat()); 
  dataset.destroy(); 

  print_config(s_tet_param, log_out); 

  AzBytArr s; 
  s.c("#train=");  s.cn(m_tr_x.colNum()); 
  s.c(", #test="); s.cn(m_test_x.colNum()); 
  AzTimeLog::print("Start train_predict ... ", s.c_str(), log_out); 
  print_hline(log_out); 

  clock_t b_clk = clock(); 
  if (do2) {
    AzTETproc::train_predict2(log_out, trainer, s_tet_param.c_str(), 
                           &m_tr_x, &v_tr_y, &featInfo, 
                           doSaveLastModelOnly, 
                           &m_test_x, s_model_stem.c_str(),
                           pred_fn_suffix, info_fn_suffix, &v_fixed_dw, prev_ens_ptr);
  }
  else {
    AzTETproc::train_predict(log_out, trainer, s_tet_param.c_str(), 
                           &m_tr_x, &v_tr_y, &featInfo, 
                           doSaveLastModelOnly, 
                           &m_test_x, s_model_stem.c_str(),
                           pred_fn_suffix, info_fn_suffix, &v_fixed_dw, prev_ens_ptr); 
  }
  AzTimeLog::print("Done ...", log_out); 
  clocks += (clock() - b_clk); 
  show_elapsed(log_out, clocks); 
}

/*------------------------------------------------*/
/*------------------------------------------------*/
bool AzTETmain::isHelpNeeded(const char *param) const
{
  if (param == NULL || strlen(param) == 0 || 
      strchr(param, '=') == NULL) {
    return true; 
  }
  return false; 
}

/*------------------------------------------------*/
bool AzTETmain::resetParam_train(const char *argv[], int argc, 
                                 bool for_train_test) 
{
  if (argc-config_argx != 1) {
    printHelp_train(log_out, argv, argc, for_train_test); 
    return false; /* failed */
  }

  const char *param = argv[config_argx]; 
  if (isHelpNeeded(param)) {
    printHelp_train(log_out, argv, argc, for_train_test); 
    return false; /* failed */    
  }

  AzParam p(param); 
  p.vStr(kw_alg_name, &s_alg_name); 
  p.vStr(kw_train_x_fn, &s_train_x_fn); 
  p.vStr(kw_train_y_fn, &s_train_y_fn); 
  p.vStr(kw_fdic_fn, &s_fdic_fn); 
  p.vStr(kw_dw_fn, &s_dw_fn); 
  if (for_train_test) {
    p.vStr(kw_test_x_fn, &s_test_x_fn); 
    p.vStr(kw_test_y_fn, &s_test_y_fn); 
    p.vStr(kw_eval_fn, &s_eval_fn); 
    p.swOn(&doAppend_eval, kw_doAppend_eval); 
    p.swOn(&doSaveLastModelOnly, kw_doSaveLastModelOnly); 
  }

  p.vStr(kw_model_stem, &s_model_stem); 
  p.vStr(kw_model_names_fn, &s_model_names_fn); 

  p.vStr(kw_prev_model_fn, &s_prev_model_fn); 
  p.swOff(&doLog, kw_not_doLog); 
  p.swOn(&doDump, kw_doDump); 

  /*---  separate unused parameters to pass to TreeEnsembleTrainer  ---*/
  s_tet_param.reset(); 
  p.check(log_out, &s_tet_param); 

  return true; /* success */
}

/*------------------------------------------------*/
void AzTETmain::printParam_train(const AzOut &out, 
                                 bool for_train_test) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 

  if (for_train_test) {
    o.ppBegin("AzTETmain::train", "\"train_test\""); 
  }
  else {
    o.ppBegin("AzTETmain::train", "\"train\""); 
  }
  o.printV(kw_alg_name, s_alg_name); 
  o.printV(kw_train_x_fn, s_train_x_fn); 
  o.printV(kw_train_y_fn, s_train_y_fn); 
  o.printV_if_not_empty(kw_fdic_fn, s_fdic_fn); 
  o.printV_if_not_empty(kw_dw_fn, s_dw_fn); 
  o.printSw(kw_doLog, doLog); 
  o.printSw(kw_doDump, doDump); 
  if (for_train_test) {
    o.printV_if_not_empty(kw_test_x_fn, s_test_x_fn); 
    o.printV_if_not_empty(kw_test_y_fn, s_test_y_fn); 
    o.printV_if_not_empty(kw_eval_fn, s_eval_fn); 
    o.printSw(kw_doAppend_eval, doAppend_eval); 
    if (s_model_stem.length() <= 0 && doSaveLastModelOnly) {
      AzBytArr s("Warning: "); s.c(kw_doSaveLastModelOnly); s.c(" is ignored as "); 
      s.c(kw_model_stem); s.c(" is not specified.");
      o.printV(s.c_str(), "");  
    }
    else {
      o.printSw(kw_doSaveLastModelOnly, doSaveLastModelOnly); 
    }
  }
  o.printV_if_not_empty(kw_model_stem, s_model_stem); 
  o.printV_if_not_empty(kw_model_names_fn, s_model_names_fn); 
  o.printV_if_not_empty(kw_prev_model_fn, s_prev_model_fn); 

  o.ppEnd(); 
}

/*------------------------------------------------*/
void AzTETmain::checkParam_train(bool for_train_test) const
{
  const char *eyec = "AzTETmain::checkParam_train"; 
  throw_if_missing(kw_train_x_fn, s_train_x_fn, eyec); 
  throw_if_missing(kw_train_y_fn, s_train_y_fn, eyec); 
  if (for_train_test) {
    throw_if_missing(kw_test_x_fn, s_test_x_fn, eyec); 
    throw_if_missing(kw_test_y_fn, s_test_y_fn, eyec); 
  }
  else {
    throw_if_missing(kw_model_stem, s_model_stem, eyec); 
  }
}

/*------------------------------------------------*/
bool AzTETmain::resetParam_train_predict(const char *argv[], int argc)
{
  bool for_train_test = false, for_train_predict = true; 

  if (argc-config_argx != 1) {
    printHelp_train(log_out, argv, argc, for_train_test, for_train_predict); 
    return false; /* failed */
  }

  const char *param = argv[config_argx]; 
  if (isHelpNeeded(param)) {
    printHelp_train(log_out, argv, argc, for_train_test, for_train_predict); 
    return false; /* failed */    
  }

  AzParam p(param); 
  p.vStr(kw_alg_name, &s_alg_name); 
  p.vStr(kw_train_x_fn, &s_train_x_fn); 
  p.vStr(kw_train_y_fn, &s_train_y_fn); 
  p.vStr(kw_fdic_fn, &s_fdic_fn); 
  p.vStr(kw_dw_fn, &s_dw_fn); 
  p.vStr(kw_test_x_fn, &s_test_x_fn); 
  p.vStr(kw_model_stem, &s_model_stem); 
  p.vStr(kw_prev_model_fn, &s_prev_model_fn); 
  p.swOn(&doSaveLastModelOnly, kw_doSaveLastModelOnly); 
  p.swOff(&doLog, kw_not_doLog); 
  p.swOn(&doDump, kw_doDump); 

  /*---  separate unused parameters to pass to TreeEnsembleTrainer  ---*/
  s_tet_param.reset(); 
  p.check(log_out, &s_tet_param); 

  return true; /* success */
}

/*------------------------------------------------*/
void AzTETmain::printParam_train_predict(const AzOut &out) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 

  o.ppBegin("AzTETmain::train", "\"train_predict\""); 

  o.printV(kw_alg_name, s_alg_name); 
  o.printV(kw_train_x_fn, s_train_x_fn); 
  o.printV(kw_train_y_fn, s_train_y_fn); 
  o.printV(kw_test_x_fn, s_test_x_fn); 
  o.printV(kw_model_stem, s_model_stem); 
  o.printSw(kw_doSaveLastModelOnly, doSaveLastModelOnly); 
  o.printV_if_not_empty(kw_fdic_fn, s_fdic_fn); 
  o.printV_if_not_empty(kw_dw_fn, s_dw_fn); 
  o.printSw(kw_doLog, doLog); 
  o.printSw(kw_doDump, doDump); 
  o.printV_if_not_empty(kw_prev_model_fn, s_prev_model_fn); 

  o.ppEnd(); 
}

/*------------------------------------------------*/
void AzTETmain::checkParam_train_predict() const
{
  const char *eyec = "AzTETmain::checkParam_train_predict"; 
  throw_if_missing(kw_train_x_fn, s_train_x_fn, eyec); 
  throw_if_missing(kw_train_y_fn, s_train_y_fn, eyec); 
  throw_if_missing(kw_test_x_fn, s_test_x_fn, eyec); 
  throw_if_missing(kw_model_stem, s_model_stem, eyec); 
}

/*------------------------------------------------*/
void AzTETmain::print_usage(const AzOut &out, 
                            const char *argv[], int argc)
const 
{
  if (out.isNull()) return; 

  const char *exe = "exe"; 
  if (exe_argx < argc) exe = argv[exe_argx]; 
  const char *action = "action"; 
  if (action_argx < argc) action = argv[action_argx]; 
  AzBytArr s_action(action); 

  AzBytArr s; 
  s.nl(); 
  s.c("Usage: "); s.c(exe); s.c("  "); s.c(action); s.c("  parameters"); 
  AzPrint::writeln(out, s); 

  int kw_width=15; 
  AzHelp h(out, kw_width); 
  h.nl(); 
  AzBytArr s_kw(action); 
  AzBytArr s_desc; 
  if      (s_action.compare(kw_train) == 0)         s_desc.c(help_train); 
  else if (s_action.compare(kw_train_test) == 0)    s_desc.c(help_train_test); 
  else if (s_action.compare(kw_predict) == 0)       s_desc.c(help_predict); 
  else if (s_action.compare(kw_batch_predict) == 0) s_desc.c(help_batch_predict); 
  if (s_desc.length() > 0) {
    h.item(s_kw.c_str(), s_desc.c_str()); 
  }
  h.nl(); 
  s_kw.reset("parameters:"); 
  s_desc.reset("keyword-value pairs (e.g., \"algorithm=RGF\") and options (e.g., \"NormalizeTarget\") delimited by \",\" described below."); 
  h.item_noquotes(s_kw.c_str(), s_desc.c_str()); 
  h.nl(); 
  h.writeln_header("  Example parameters:"); 
  s.reset(); 
  s.c("    "); 
  if (s_action.beginsWith("train")) {  
    const char *dflt_name = alg_sel->dflt_name(); 
    s.c("algorithm="); s.c(dflt_name); s.c(",train_x_fn=data.x,train_y_fn=data.y,"); 
    if (alg_sel->isRGFfamily(dflt_name)) {
      s.c("reg_L2=0.1,"); 
    }
    else if (strcmp(dflt_name, "GB") == 0) {
      s.c("shrink=0.1,"); 
    }
    s.c("..."); 
  }
  else {
    s.c("model_fn=model.bin-01,test_x_fn=test-data.x,..."); 
  }
  h.writeln_header(s.c_str()); 
  h.nl();  
  h.writeln_header("Below, \"*\" indicates the required parameters that cannot be omitted.");
  h.nl(); 
  s.reset(); s.c("[ Parameters for \""); s.c(action); s.c("\" ]"); 
  h.writeln_header(s.c_str());  
}

/*------------------------------------------------*/
void AzTETmain::printHelp_train(const AzOut &out,
                          const char *argv[], int argc, 
                          bool for_train_test, 
                          bool for_train_predict) const
{
  if (exe_argx >= argc || action_argx >= argc) {
    AzPrint::writeln(out, "No help (detected in AzTETmain::printHelp_train)"); 
    return; 
  }

  print_usage(out, argv, argc); 

  AzBytArr s_alg_options; 
  alg_sel->printOptions("|", &s_alg_options); 

  AzHelp h(out); 
  if (s_alg_name.length() > 0) {
    h.item(kw_alg_name, s_alg_options.c_str(), s_alg_name.c_str()); 
  }
  else {
    h.item_required(kw_alg_name, s_alg_options.c_str()); 
  }
  h.item_required(kw_train_x_fn, help_train_x_fn); 
  h.item_required(kw_train_y_fn, help_train_y_fn);
  h.item_experimental(kw_fdic_fn, help_fdic_fn); 
  if (for_train_test) {
    h.item_required(kw_test_x_fn, help_test_x_fn); 
    h.item_required(kw_test_y_fn, help_test_y_fn); 
    h.item(kw_eval_fn, help_eval_fn, "stdout"); 
    h.item(kw_doAppend_eval, help_doAppend_eval); 

    h.nl(); 
    h.writeln_header("To optionally save the models to files:"); 
    h.item(kw_model_stem, help_model_stem, dflt_model_stem);
    h.item(kw_doSaveLastModelOnly, help_doSaveLastModelOnly_traintest);
    h.item_experimental(kw_model_names_fn, help_model_names_fn_out);
  }
  else if (for_train_predict) {
    h.item_required(kw_test_x_fn, help_test_x_fn);     
    h.item_required(kw_model_stem, help_model_stem_tp, dflt_model_stem);
    h.item(kw_doSaveLastModelOnly, help_doSaveLastModelOnly);
  }
  else {
    h.item_required(kw_model_stem, help_model_stem, dflt_model_stem);
    h.item_experimental(kw_model_names_fn, help_model_names_fn_out); 
  }

  h.nl(); 
  h.writeln_header("To optionally specify the weights of individual data points:"); 
  h.item(kw_dw_fn, help_dw_fn);

  h.nl(); 
  h.writeln_header("To optionally do warm-start with an existing model:"); 
  if (alg_sel->isRGFfamily(alg_sel->dflt_name())) {
    h.item(kw_prev_model_fn, help_prev_model_fn); 
  }
  else {
    /* not all algorithms in others supports warm-start */
    h.item(kw_prev_model_fn, help_prev_model_fn_others); 
  }

  h.item_experimental(kw_not_doLog, help_not_doLog); 
  h.item_experimental(kw_doDump, help_doDump); 
  h.end(); 

  /*---  ---*/
  const char *alg_name = NULL; 
  AzTETrainer *trainer = NULL;   

  /*---  select algorithm  ---*/
  if (config_argx < argc) {
    alg_name = argv[config_argx]; 
    bool dontThrow = true; 
    trainer = alg_sel->select(alg_name, dontThrow); 
  }
  if (trainer == NULL) {
    /*---  help for the default algorithm  ---*/
    const char *dflt_alg = alg_sel->dflt_name(); 
    trainer = alg_sel->select(dflt_alg); 
    h.nl(); h.nl();  
    AzBytArr s("[ Parameters for ");  s.c(dflt_alg); s.c(" (default algorithm) ]");
    h.writeln_header(s.c_str()); 
    trainer->printHelp(h); 
    h.nl(); 

    const char *other_alg = alg_sel->another_name(); 
    if (other_alg != NULL) {
      s.reset();  
      s.nl(); 
      s.c("   ---------------------------------------------------------"); s.nl(); 
      s.c("   To display parameters for other algorithms, enter: "); s.nl(); 
      s.c("             "); s.c(argv[exe_argx]); s.c(" "); s.c(argv[action_argx]);
      s.c("  algorithm_name"); s.nl(); s.nl(); 
      s.c("      Example:  "); s.c(argv[exe_argx]); s.c(" "); s.c(argv[action_argx]); 
      s.c(" "); s.c(other_alg); s.nl(); 
      s.c("   ---------------------------------------------------------"); s.nl(); 
      s.nl(); 
      s.c("   List of algorithm names: "); s.nl(); 
      AzPrint::write(out, s); 
      alg_sel->printHelp(h); 
    }
  }
  else {
    AzBytArr s; 
    s.nl(); 
    s.c("   [ Parameters for "); 
    s.c(alg_name); s.c(" ]"); s.nl(); 
    AzPrint::write(out, s); 
    trainer->printHelp(h); 
  }
}

/*------------------------------------------------*/
/*------------------------------------------------*/
bool AzTETmain::resetParam_batch_predict(const char *argv[], int argc)
{
  if (argc-config_argx != 1) {
    printHelp_batch_predict(log_out, argv, argc); 
    return false; /* faied */
  }

  const char *param = argv[config_argx]; 
  if (isHelpNeeded(param)) {
    printHelp_batch_predict(log_out, argv, argc); 
    return false; /* failed */    
  }

  AzParam p(param); 

  p.vStr(kw_model_names_fn, &s_model_names_fn); 
  p.vStr(kw_test_x_fn, &s_test_x_fn); 
  p.vStr(kw_pred_fn_suffix, &s_pred_fn_suffix); 

  p.vStr(kw_test_y_fn, &s_test_y_fn); 
  p.vStr(kw_eval_fn, &s_eval_fn); 
  p.swOn(&doAppend_eval, kw_doAppend_eval); 

  p.swOff(&doLog, kw_not_doLog); 
  p.swOn(&doDump, kw_doDump); 

  p.check(log_out); 

  return true; 
}

/*------------------------------------------------*/
void AzTETmain::printParam_batch_predict(const AzOut &out) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 

  o.ppBegin("AzTETmain::predict", "\"batch_predict\""); 
  o.printV(kw_model_names_fn, s_model_names_fn); 
  o.printV(kw_test_x_fn, s_test_x_fn); 
  o.printV(kw_pred_fn_suffix, s_pred_fn_suffix); 

  o.printV_if_not_empty(kw_test_y_fn, s_test_y_fn);
  o.printV_if_not_empty(kw_eval_fn, s_eval_fn); 
  o.printSw(kw_doAppend_eval, doAppend_eval); 

  o.printSw(kw_doLog, doLog); 
  o.printSw(kw_doDump, doDump); 

  o.ppEnd(); 
}

/*------------------------------------------------*/
void AzTETmain::printHelp_batch_predict(const AzOut &out, 
                const char *argv[], int argc) const
{
  print_usage(out, argv, argc); 

  /*---  driver config  ---*/
  AzHelp h(out);

  h.begin("batch_predict", "AzTETmain"); 
  h.item_required(kw_model_names_fn, help_model_names_fn_inp); 
  h.item_required(kw_test_x_fn, help_test_x_fn); 
  h.item_required(kw_pred_fn_suffix, help_pred_fn_suffix); 

  h.nl(); 
  h.writeln_header("To optionally evaluate the prediction values: "); 
  h.item(kw_test_y_fn, help_test_y_fn); 
  h.item(kw_eval_fn, help_eval_fn, "stdout"); 
  h.item_experimental(kw_doAppend_eval, help_doAppend_eval); 
  h.item_experimental(kw_not_doLog, help_not_doLog); 
  h.item_experimental(kw_doDump, help_doDump); 

  h.end(); 
}

/*------------------------------------------------*/
void AzTETmain::checkParam_batch_predict() const
{
  const char *eyec = "AzTETmain::checkParam_batch_predict"; 
  throw_if_missing(kw_test_x_fn, s_test_x_fn, eyec); 
  throw_if_missing(kw_model_names_fn, s_model_names_fn, eyec); 
  throw_if_missing(kw_pred_fn_suffix, s_pred_fn_suffix, eyec); 

  if (s_eval_fn.length() > 0) {
    if (s_test_y_fn.length() <= 0) {
      AzBytArr s_kw; s_kw.inQuotes(kw_test_y_fn, "\""); 
      throw new AzException(AzInputMissing, eyec, s_kw.c_str(), 
            "is missing though evaluation is requested"); 
    }
  }
}

/*------------------------------------------------*/
/*------------------------------------------------*/
bool AzTETmain::resetParam_predict_single(const char *argv[], int argc)
{
  if (argc-config_argx != 1) {
    printHelp_predict_single(log_out, argv, argc); 
    return false; /* faied */
  }

  const char *param = argv[config_argx]; 
  if (isHelpNeeded(param)) {
    printHelp_predict_single(log_out, argv, argc); 
    return false; /* failed */    
  }

  AzParam p(param); 

  p.vStr(kw_model_fn, &s_model_fn); 
  p.vStr(kw_test_x_fn, &s_test_x_fn); 
  p.vStr(kw_pred_fn, &s_pred_fn); 

  p.vStr(kw_test_y_fn, &s_test_y_fn); 
  p.vStr(kw_eval_fn, &s_eval_fn); 
  p.swOn(&doAppend_eval, kw_doAppend_eval); 

  p.swOff(&doLog, kw_not_doLog); 
  p.swOn(&doDump, kw_doDump); 
  p.check(log_out); 

  return true; 
}

/*------------------------------------------------*/
void AzTETmain::printParam_predict_single(const AzOut &out) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 

  o.ppBegin("AzTETmain::predict", "\"predict\""); 
  o.printV(kw_model_fn, s_model_fn); 
  o.printV(kw_test_x_fn, s_test_x_fn); 
  o.printV(kw_pred_fn, s_pred_fn);  

  o.printV_if_not_empty(kw_test_y_fn, s_test_y_fn);
  o.printV_if_not_empty(kw_eval_fn, s_eval_fn); 
  o.printSw(kw_doAppend_eval, doAppend_eval); 

  o.printSw(kw_doLog, doLog); 
  o.printSw(kw_doDump, doDump); 

  o.ppEnd(); 
}

/*------------------------------------------------*/
void AzTETmain::printHelp_predict_single(const AzOut &out, 
                const char *argv[], int argc) const
{
  print_usage(out, argv, argc); 

  AzHelp h(out);
  h.begin("predict (single)", "AzTETmain"); 
  h.item_required(kw_model_fn, help_model_fn); 
  h.item_required(kw_test_x_fn, help_test_x_fn); 
  h.item_required(kw_pred_fn, help_pred_fn_out); 

  h.nl(); 
  h.writeln_header_experimental("To optionally evaluate the prediction values: "); 
  h.item_experimental(kw_test_y_fn, help_test_y_fn); 
  h.item_experimental(kw_eval_fn, help_eval_fn, "stdout"); 

  h.item_experimental(kw_doAppend_eval, help_doAppend_eval); 
  h.item_experimental(kw_not_doLog, help_not_doLog); 
  h.item_experimental(kw_doDump, help_doDump); 

  h.end(); 
}

/*------------------------------------------------*/
void AzTETmain::checkParam_predict_single() const
{
  const char *eyec = "AzTETmain::checkParam_predict_single"; 
  throw_if_missing(kw_test_x_fn, s_test_x_fn, eyec); 
  throw_if_missing(kw_model_fn, s_model_fn, eyec); 
  throw_if_missing(kw_pred_fn, s_pred_fn, eyec); 
  if (s_model_fn.compare(&s_pred_fn) == 0) {
    throw new AzException(AzInputNotValid, eyec, 
              "model filename and prediction filename must be different"); 
  }
  if (s_eval_fn.length() > 0) {
    if (s_test_y_fn.length() <= 0) {
      AzBytArr s_kw; s_kw.inQuotes(kw_test_y_fn, "\""); 
      throw new AzException(AzInputMissing, eyec, s_kw.c_str(), 
            "is missing though evaluation is requested"); 
    }
  }
}

/*------------------------------------------------*/
/*------------------------------------------------*/
bool AzTETmain::resetParam_xv(const char *argv[], int argc)
{
  if (argc-config_argx != 1) {
    printHelp_xv(log_out, argv, argc); 
    return false; /* failed */
  }

  const char *param = argv[config_argx]; 
  if (isHelpNeeded(param)) {
    printHelp_xv(log_out, argv, argc); 
    return false; /* failed */    
  }

  AzParam p(param); 
  p.vStr(kw_alg_name, &s_alg_name); 
  p.vStr(kw_train_x_fn, &s_train_x_fn); 
  p.vStr(kw_train_y_fn, &s_train_y_fn); 
  p.vStr(kw_fdic_fn, &s_fdic_fn); 
  p.vStr(kw_dw_fn, &s_dw_fn); 

  p.swOn(&xv_doShuffle, kw_xv_doShuffle); 
  p.vInt(kw_xv_num, &xv_num); 
  p.vStr(kw_xv_fn, &s_xv_fn); 

  p.swOff(&doLog, kw_not_doLog); 
  p.swOn(&doDump, kw_doDump); 

  /*---  separate unused parameters to pass to TreeEnsembleTrainer  ---*/
  s_tet_param.reset(); 
  p.check(log_out, &s_tet_param); 

  return true; /* success */
}

/*------------------------------------------------*/
void AzTETmain::printParam_xv(const AzOut &out) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 

  o.ppBegin("AzTETmain::xv", "\"xv\""); 

  o.printV(kw_alg_name, s_alg_name); 
  o.printV(kw_train_x_fn, s_train_x_fn); 
  o.printV(kw_train_y_fn, s_train_y_fn); 
  o.printV_if_not_empty(kw_fdic_fn, s_fdic_fn); 
  o.printV_if_not_empty(kw_dw_fn, s_dw_fn); 
  o.printSw(kw_xv_doShuffle, xv_doShuffle); 
  o.printV(kw_xv_num, xv_num);
  o.printV(kw_xv_fn, s_xv_fn); 
  o.printSw(kw_doLog, doLog); 
  o.printSw(kw_doDump, doDump); 

  o.ppEnd(); 
}

/*------------------------------------------------*/
void AzTETmain::checkParam_xv() const
{
  const char *eyec = "AzTETmain::checkParam_xv"; 
  throw_if_missing(kw_train_x_fn, s_train_x_fn, eyec); 
  throw_if_missing(kw_train_y_fn, s_train_y_fn, eyec); 
  throw_if_missing(kw_xv_fn, s_xv_fn, eyec); 
}

/*------------------------------------------------*/
/*------------------------------------------------*/
void AzTETmain::prepareLogDmp(bool doLog, bool doDump) 
{
  log_out.reset(NULL); 
  dmp_out.reset(NULL); 
  if (doLog) {
    log_out.setStdout(); 
  }
  if (doDump) {
    dmp_out.setStderr(); 
  }
}

/*------------------------------------------------------*/
/*------------------------------------------------------*/
/*  write features generated by tree ensemble to a file */
/*------------------------------------------------------*/
void AzTETmain::features(const char *argv[], int argc)
{
  bool success = resetParam_features(argv, argc); 
  if (!success) return; 

  prepareLogDmp(doLog, doDump);

  printParam_features(log_out); 
  print_hline(log_out); 
  checkParam_features();

  AzSvDataS dataset; 
  dataset.read_features_only(s_input_x_fn.c_str()); 
  AzSmat m_x(dataset.feat()); 

  AzTreeEnsemble ens(s_model_fn.c_str()); 
  AzTETproc::features(log_out, &ens, &m_x, s_output_x_fn.c_str(), 
                      features_digits, doSparse_features); 
  AzTimeLog::print("Done ... ", log_out); 
}

/*------------------------------------------------*/
/*------------------------------------------------*/
bool AzTETmain::resetParam_features(const char *argv[], int argc)
{
  if (argc-config_argx != 1) {
    printHelp_features(log_out, argv, argc); 
    return false; /* faied */
  }

  const char *param = argv[config_argx]; 
  if (isHelpNeeded(param)) {
    printHelp_features(log_out, argv, argc); 
    return false; /* failed */    
  }

  AzParam p(param); 
  p.vStr(kw_model_fn, &s_model_fn); 
  p.vStr(kw_input_x_fn, &s_input_x_fn); 
  p.vStr(kw_output_x_fn, &s_output_x_fn); 
  p.swOn(&doSparse_features, kw_doSparse_features); 
  p.vInt(kw_features_digits, &features_digits); 
  p.check(log_out); 

  return true; 
}

/*------------------------------------------------*/
void AzTETmain::printParam_features(const AzOut &out) const
{
  if (out.isNull()) return; 
  AzPrint o(out); 
  o.ppBegin("AzTETmain::features", "\"features\""); 
  o.printV(kw_model_fn, s_model_fn); 
  o.printV(kw_input_x_fn, s_input_x_fn); 
  o.printV(kw_output_x_fn, s_output_x_fn); 
  o.printSw(kw_doSparse_features, doSparse_features); 
  o.printV(kw_features_digits, features_digits); 
  o.ppEnd(); 
}

/*------------------------------------------------*/
void AzTETmain::checkParam_features() const
{
  const char *eyec = "AzTETmain::checkParam_features"; 
  throw_if_missing(kw_model_fn, s_model_fn, eyec); 
  throw_if_missing(kw_input_x_fn, s_input_x_fn, eyec); 
  throw_if_missing(kw_output_x_fn, s_output_x_fn, eyec); 
}

/*------------------------------------------------*/
void AzTETmain::printHelp_features(const AzOut &out, 
                const char *argv[], int argc) const
{
  print_usage(out, argv, argc); 
  AzHelp h(out);
  h.begin("output features", "AzTETmain"); 
  h.item_required(kw_model_fn, help_model_fn); 
  h.item_required(kw_input_x_fn, help_input_x_fn); 
  h.item_required(kw_output_x_fn, help_output_x_fn); 
  h.item(kw_features_digits, help_features_digits); 
  h.item(kw_doSparse_features, help_doSparse_features); 
  h.end(); 
}
