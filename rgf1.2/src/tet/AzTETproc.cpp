/* * * * *
 *  AzTETproc.cpp 
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

#include "AzTETproc.hpp"
#include "AzTaskTools.hpp"

/*------------------------------------------------------------------*/
void AzTETproc::train(const AzOut &out, 
                      AzTETrainer *trainer, 
                      const char *config,                       
                      AzSmat *m_train_x, 
                      AzDvect *v_train_y, 
                      const AzSvFeatInfo *featInfo, 
                      /*---  for writing model to file  ---*/
                      const char *out_model_fn, 
                      const char *out_model_names_fn, /* may be NULL */
                      /*---  data point weights  ---*/
                      AzDvect *v_fixed_dw, /* may be NULL */
                      /*---  for warm start  ---*/
                      AzTreeEnsemble *inp_ens) /* may be NULL */
{
  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 

  AzBytArr s_model_names; 
  int seq_no = 1; 
  for ( ; ; ) {
    AzTETrainer_Ret ret = trainer->proceed_until(); 
    if (out_model_fn != NULL) {
      AzTreeEnsemble ens; 
      trainer->copy_to(&ens); 
      writeModel(&ens, seq_no, out_model_fn, NULL, &s_model_names, out); 
      ++seq_no; 
    }
    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
  }
  int model_num = seq_no - 1; 
  end_of_saving_models(model_num, s_model_names, out_model_names_fn, out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::train_test(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        /*---  for evaluation  ---*/
                        AzSmat *m_test_x, 
                        AzTET_Eval *eval, 
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens) /* may be NULL */
{
  AzTETrainer_TestData td(out, m_test_x); 

  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  eval->begin(config, trainer->lossType()); 
  int seq_no = 1; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    /*---  test the current model  ---*/
    AzDvect v_p; 
    AzTE_ModelInfo info; 
    trainer->apply(&td, &v_p, &info); 
    eval->evaluate(&v_p, &info); 

    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
  }
  eval->end(); 
}

/*------------------------------------------------------------------*/
void AzTETproc::train_test_save(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        /*---  for evaluation  ---*/
                        AzSmat *m_test_x, 
                        AzTET_Eval *eval, 
                        /*---  for saving models  ---*/
                        bool doSaveLastModelOnly, 
                        const char *out_model_fn, 
                        const char *out_model_names_fn, /* may be NULL */
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens) /* may be NULL */
{
  AzTETrainer_TestData td(out, m_test_x); 

  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  eval->begin(config, trainer->lossType()); 
  int seq_no = 1; 
  int model_num = 0; 
  AzBytArr s_model_names; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    AzTreeEnsemble ens; 
    AzDvect v_p; 
    AzTE_ModelInfo info; 
    trainer->apply(&td, &v_p, &info, &ens); 
    AzBytArr s_model_fn; 
    const char *model_fn = NULL; 
    if (!doSaveLastModelOnly || ret == AzTETrainer_Ret_Exit) {
      writeModel(&ens, seq_no, out_model_fn, &s_model_fn, &s_model_names, out); 
      ++model_num; 
      model_fn = s_model_fn.c_str(); 
    }
    eval->evaluate(&v_p, &info, model_fn); 
    ++seq_no; 

    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
  }
  eval->end(); 

  end_of_saving_models(model_num, s_model_names, out_model_names_fn, out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::end_of_saving_models(int model_num, 
                                     const AzBytArr &s_model_names, 
                                     const char *out_model_names_fn, 
                                     const AzOut &out)
{
  if (isSpecified(out_model_names_fn)) {
    AzFile file(out_model_names_fn); 
    file.open("wb"); 
    s_model_names.writeText(&file); 
    file.close(true); 
  }
  if (!out.isNull()) {
    AzBytArr s("Generated "); s.cn(model_num); s.c(" model file(s): "); 
    AzPrint::writeln(out, ""); 
    AzPrint::writeln(out, s); 
    AzPrint::writeln(out, s_model_names); 
  }
}

/*------------------------------------------------------------------*/
void AzTETproc::writeModel(AzTreeEnsemble *ens, 
                           int seq_no, 
                           const char *fn_stem, 
                           AzBytArr *s_model_fn, 
                           AzBytArr *s_model_names, 
                           const AzOut &out)
{
  AzBytArr s; 
  gen_model_fn(fn_stem, seq_no, &s); 
  AzTimeLog::print("Writing model: seq#=", seq_no, out); 
  ens->write(s.c_str()); 
  if (s_model_fn != NULL) s_model_fn->concat(&s); 
  s.nl(); 
  s_model_names->concat(&s); 
}

/*------------------------------------------------------------------*/
void AzTETproc::gen_model_fn(const char *fn_stem, 
                             int seq_no, 
                             AzBytArr *s) /* output */
{
  s->reset(fn_stem); 
  s->c("-"); s->cn(seq_no, 2, true); /* width 2, fill with zero */
}

/*------------------------------------------------------------------*/
void AzTETproc::train_predict(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        bool doSaveLastModelOnly, 
                        /*---  for prediction  ---*/
                        AzSmat *m_test_x, 
                        const char *model_fn_prefix, 
                        const char *pred_fn_suffix,
                        const char *info_fn_suffix,
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens) /* may be NULL */
{
  AzTETrainer_TestData td(out, m_test_x); 

  int model_num = 0; 
  AzBytArr s_model_names; 
  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  int seq_no = 1; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    AzTreeEnsemble ens; 
    AzDvect v_p; 
    AzTE_ModelInfo info; 
    trainer->apply(&td, &v_p, &info, &ens); 

    writePrediction(model_fn_prefix, &v_p, seq_no, pred_fn_suffix, out); 
    writeModelInfo(model_fn_prefix, seq_no, info_fn_suffix, &info, out); 

    if (ret == AzTETrainer_Ret_Exit || 
        !doSaveLastModelOnly) {
      writeModel(&ens, seq_no, model_fn_prefix, NULL, &s_model_names, out); 
      ++model_num; 
    }
    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
    ++seq_no; 
  }
  end_of_saving_models(model_num, s_model_names, "", out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::train_predict2(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        bool doSaveLastModelOnly, 
                        /*---  for prediction  ---*/
                        AzSmat *m_test_x, 
                        const char *model_fn_prefix, 
                        const char *pred_fn_suffix,
                        const char *info_fn_suffix,
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens) /* may be NULL */
{
  AzTimeLog::print("train_predict2 ... ", log_out); 
  int model_num = 0; 
  AzBytArr s_model_names; 
  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  int seq_no = 1; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    AzTreeEnsemble ens; 
    trainer->copy_to(&ens); 
    AzDvect v_p; 
    ens.apply(m_test_x, &v_p);  
    AzTE_ModelInfo info; 
    ens.info(&info); 

    writePrediction(model_fn_prefix, &v_p, seq_no, pred_fn_suffix, out); 
    writeModelInfo(model_fn_prefix, seq_no, info_fn_suffix, &info, out); 

    if (ret == AzTETrainer_Ret_Exit || 
        !doSaveLastModelOnly) {
      writeModel(&ens, seq_no, model_fn_prefix, NULL, &s_model_names, out); 
      ++model_num; 
    }
    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
    ++seq_no; 
  }
  end_of_saving_models(model_num, s_model_names, "", out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::writePrediction(
                           const char *fn_stem, 
                           const AzDvect *v_p, 
                           int seq_no, 
                           const char *pred_suffix, 
                           const AzOut &out)
{
  AzTimeLog::print("Writing prediction: seq#=", seq_no, out); 

  AzBytArr s_fn; 
  gen_model_fn(fn_stem, seq_no, &s_fn); 
  AzBytArr s_pfn; 
  s_pfn.concat(&s_fn); s_pfn.concat(pred_suffix); 
  if (s_pfn.compare(&s_fn) == 0) {
    throw new AzException(AzInputError, "AzTETproc::writePrediction", 
                          "No suffix for prediction filenames is given"); 
  }

  writePrediction(s_pfn.c_str(), v_p); 
}

/*------------------------------------------------------------------*/
void AzTETproc::writePrediction(const char *fn, 
                                const AzDvect *v_p)
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
  AzFile file(fn); 
  file.open("wb"); 
  s.writeText(&file); 
}

/*------------------------------------------------------------------*/
void AzTETproc::writeModelInfo(
                           const char *fn_stem, 
                           int seq_no, 
                           const char *info_suffix, 
                           const AzTE_ModelInfo *info, 
                           const AzOut &out)
{
  AzBytArr s_fn; 
  gen_model_fn(fn_stem, seq_no, &s_fn); 
  AzBytArr s_ifn; 
  s_ifn.concat(&s_fn); s_ifn.concat(info_suffix); 

  writeModelInfo(s_ifn.c_str(), s_fn.c_str(), info, out); 
}
 
/*------------------------------------------------------------------*/
void AzTETproc::writeModelInfo(const char *info_fn, 
                           const char *model_fn, 
                           const AzTE_ModelInfo *info, 
                           const AzOut &out)
{
  AzTimeLog::print("Writing model info", out); 

  AzBytArr s; 
  /*---  info  ---*/
  AzBytArr s_cfg(&info->s_config); 
  s_cfg.replace(',', ';'); 

  s.reset(); 
  s.c("#tree,"); s.cn(info->tree_num); 
  s.c(",#leaf,"); s.cn(info->leaf_num); 
  s.c(",sign,"); s.c(&info->s_sign); 
  s.c(",cfg,"); s.c(&s_cfg); 
  s.c(","); s.c(model_fn); 
  s.nl(); 

  AzFile ifile(info_fn); 
  ifile.open("wb"); 
  s.writeText(&ifile); 
  ifile.close(true); 
}

/*------------------------------------------------------------------*/
void AzTETproc::xv(const AzOut &out, 
                   int xv_num, 
                   const char *xv_fn, 
                   bool doShuffle, 
                   AzTETrainer *trainer, 
                   const char *config, 
                   AzSmat *m_x, 
                   AzDvect *v_y, 
                   const AzSvFeatInfo *featInfo,
                   /*---  data point weights  ---*/
                   AzDvect *v_dw) /* may be NULL */
{
  const char *eyec = "AzTETproc::xv"; 

  int nn = m_x->colNum(); 
  int each = nn / xv_num; 
  int extra = nn % xv_num; 
  AzIntArr ia_dxs; 
  ia_dxs.range(0, nn); 
  if (doShuffle) {
    AzTools::shuffle(-1, &ia_dxs); 
  }
  const int *dxs = ia_dxs.point(); 

  AzDataPool<AzPerfResult> perf; 

  int bx = 0; 
  int xx; 
  for (xx = 0; xx < xv_num; ++xx) {
    int ex = bx+each; 
    if (extra > 0) {
      ++ex; 
      --extra; 
    }
    int tst_num = ex-bx; 
    int trn_num = nn-tst_num; 
 
    AzBytArr s("-----  "); s.cn(xx+1); s.c("/"); s.cn(xv_num); 
    s.c(" #train: "); s.cn(trn_num); s.c(" #test: "); s.cn(tst_num); 
    AzTimeLog::print(s, out); 
   
    AzSmat m_train_x(m_x->rowNum(), trn_num), m_test_x(m_x->rowNum(), tst_num); 
    AzDvect v_train_y(trn_num), v_test_y(tst_num); 
    AzDvect v_fixed_dw; 
    if (!AzDvect::isNull(v_dw)) {
      v_fixed_dw.reform(trn_num); 
    }
    int trn_col=0, tst_col=0; 
	int ix; 
    for (ix = 0; ix < nn; ++ix) {
      if (ix >= bx && ix < ex) {
        m_test_x.col_u(tst_col)->set(m_x->col(ix)); 
        v_test_y.set(tst_col, v_y->get(ix)); 
        ++tst_col; 
      }
      else {
        m_train_x.col_u(trn_col)->set(m_x->col(ix)); 
        v_train_y.set(trn_col, v_y->get(ix)); 
        if (!AzDvect::isNull(v_dw)) {
          v_fixed_dw.set(trn_col, v_dw->get(ix)); 
        }
        ++trn_col; 
      }
    }
    if (trn_col != m_train_x.colNum() || tst_col != m_test_x.colNum()) {
      throw new AzException(eyec, "dimension mismatch"); 
    }    

    /*---  ---*/
    AzTETrainer_TestData td(out, &m_test_x); 
    trainer->startup(out, config, &m_train_x, &v_train_y, featInfo, 
                     &v_fixed_dw, NULL); 
    int seq = 0; 
    for ( ; ; ++seq) {
      AzTETrainer_Ret ret = trainer->proceed_until(); 
      AzDvect v_p; 
      AzTE_ModelInfo info; 
      trainer->apply(&td, &v_p, &info); 

      AzPerfResult res = AzTaskTools::eval(&v_p, &v_test_y, trainer->lossType()); 
      AzBytArr s("seq,");s.cn(seq+1);
      s.c(",acc,");s.cn(res.acc,6);
      s.c(",rmse,");s.cn(res.rmse,6); 
      s.c(",loss,");s.cn(res.loss,6); 
      s.c(",#leaf,"); s.cn(info.leaf_num); 
      s.c(",#tree,"); s.cn(info.tree_num); 
      AzPrint::writeln(out, s); 

      res.multiply(1/(double)xv_num); 
      if (xx == 0) {
        *(perf.new_slot()) = res; 
      }
      else {
        perf.point_u(seq)->add(&res); 
      }

      if (ret == AzTETrainer_Ret_Exit) {
        break;   
      }
    }
    if (xx > 0 && perf.size() != seq+1) {
      throw new AzException(eyec, "the number of results is different?"); 
    }
  }

  /*--- ---*/
  AzFile file(xv_fn); 
  file.open("wb"); 
  AzBytArr s("seq  acc  rmse  loss\n"); 
  s.writeText(&file);  
  int ix; 
  for (ix = 0; ix < perf.size(); ++ix) {
    const AzPerfResult *res = perf.point(ix); 
    s.reset(); 
    s.cn(ix+1);s.c(" ");s.cn(res->acc,6);s.c(" ");s.cn(res->rmse,6);s.c(" "); 
    s.cn(res->loss,6);s.nl(); 
    s.writeText(&file); 
  }
  file.close(true); 
}

/*------------------------------------------------------------------*/
void AzTETproc::features(const AzOut &out, 
                      const AzTreeEnsemble *ens, 
                      const AzSmat *m_x, 
                      const char *out_fn, 
                      int digits, 
                      bool doSparse)
{
  AzSmat m;
  ens_feats(out, ens, m_x, &m); 
  m.writeText(out_fn, digits, doSparse); 
}

/*------------------------------------------------------------------*/
void AzTETproc::ens_feats(
                      const AzOut &out, 
                      const AzTreeEnsemble *ens, 
                      const AzSmat *m_x, 
                      AzSmat *m_out, 
                      int offs, 
                      double value)
{
  const char *eyec = "AzTETproc::ens_feats"; 
  if (ens->orgdim() != m_x->rowNum()) {
    throw new AzException(AzInputError, eyec, "dimensionality mismatch"); 
  }
  int f_num = offs; 
  AzIntPool ip; 
  int tx; 
  for (tx = 0; tx < ens->size(); ++tx) {
    const AzTree *tree = ens->tree(tx); 
    AzIntArr ia_nx2fx; 
    ia_nx2fx.reset(tree->nodeNum(), -1); 
    int nx; 
    for (nx = 0; nx < tree->nodeNum(); ++nx) {
      if (tree->node(nx)->weight != 0) {
        ia_nx2fx.update(nx, f_num); 
        ++f_num; 
      }
    }
    ip.put(&ia_nx2fx); 
  }

  set_feat(m_x, ens, &ip, f_num, value, m_out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::set_feat(const AzSmat *m_x, 
                     const AzTreeEnsemble *ens, 
                     const AzIntPool *ip, 
                     int f_num, 
                     double value, 
                     AzSmat *m_out) /* output */
{
  m_out->reform(f_num, m_x->colNum()); 
  int dx; 
  for (dx = 0; dx < m_x->colNum(); ++dx) {
    AzDvect v(m_x->col(dx)); 
    AzIFarr ifa; 
    int tx; 
    for (tx = 0; tx < ens->size(); ++tx) {
      const int *nx2fx = ip->point(tx); 
      AzIntArr ia_nodes; 
      ens->tree(tx)->apply(&v, &ia_nodes); 
      int ix; 
      for (ix = 0; ix < ia_nodes.size(); ++ix) {
        int nx = ia_nodes.get(ix); 
        int fx = nx2fx[nx]; 
        if (fx >= 0) {
          ifa.put(fx, value); 
        }
      }
    }
    m_out->col_u(dx)->load(&ifa); 
  }
}
