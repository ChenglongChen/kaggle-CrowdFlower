/* * * * *
 *  AzRgf_Optimizer_Dflt.cpp 
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

#include "AzRgf_Optimizer_Dflt.hpp"
#include "AzRgfTreeEnsImp.hpp"
#include "AzHelp.hpp"

/*------------------------------------------------------------------*/
void AzRgf_Optimizer_Dflt::cold_start(AzLossType loss_type, 
                     const AzDataForTrTree *data, 
                     const AzRegDepth *reg_depth, 
                     AzParam &param, 
                     const AzDvect *v_y, /* for training */
                     const AzDvect *v_fixed_dw, /* user-assigned data point weights */
                     const AzOut out_req, 
                     AzDvect *out_v_pval) /* output */
{
  out = out_req; 
  bool beVerbose = resetParam(param); 
  trainer->reset(loss_type, v_y, v_fixed_dw, reg_depth, param, beVerbose, out); 
  trainer->copyPred_to(out_v_pval); 
  feat1.reset(data, param, out); 

  if (!beVerbose) {
    out.deactivate(); 
  }
}

/*------------------------------------------------------------------*/
void AzRgf_Optimizer_Dflt::warm_start(AzLossType loss_type, 
                     const AzDataForTrTree *data, 
                     const AzRegDepth *reg_depth, 
                     AzParam &param, 
                     const AzDvect *v_y, /* for training */
                     const AzDvect *v_fixed_dw, /* user-assigned data point weights */
                     const AzOut out_req, 
                     /*---  for warm start  ---*/
                     const AzTrTreeEnsemble_ReadOnly *inp_ens, 
                     const AzDvect *inp_v_p)
{
  out = out_req; 
  bool beVerbose = resetParam(param); 
  feat1.reset(data, param, out); 
  AzIntArr ia_removed_fx; 
  feat1.update_with_ens(inp_ens, &ia_removed_fx); 

  trainer->reset(loss_type, v_y, v_fixed_dw, reg_depth, param, beVerbose, out, 
                 inp_ens, &feat1, inp_v_p); /* for warm start */
  if (!beVerbose) {
    out.deactivate(); 
  }
}

/*------------------------------------------------------------------*/
void 
AzRgf_Optimizer_Dflt::update(const AzDataForTrTree *data,          
                  AzRgfTreeEnsemble *ens, /* inout */
                  /*---  inout  ---*/
                  AzDvect *v_p) /* prediction */
{
  AzIntArr ia_removed_fx; 
  int f_num_delta = feat1.update_with_ens(ens, &ia_removed_fx); 

  if (f_num_delta > 0 || ens->size() == 0) {
    trainer->optimize(ens, &feat1); 
  }
  else {
    AzTimeLog::print("No new feature", out); 
  }

  if (v_p != NULL) {
    trainer->copyPred_to(v_p); 
  }
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
/* static */
void AzRgf_Optimizer_Dflt::_info(const AzTrTreeEnsemble_ReadOnly *ens, 
                    const AzOptimizerT *my_trainer, 
                    const AzTrTreeFeat *my_feat, 
                    int *f_num, int *nz_f_num) 
{
  const AzDvect *v_w = my_trainer->weights(); 
  *f_num = v_w->rowNum(); 
  *nz_f_num = my_feat->countNonzeroNodup(v_w, ens); 
}

/*------------------------------------------------------------------*/
void AzRgf_Optimizer_Dflt::apply(const AzDataForTrTree *test_data, 
                                 AzBmat *b_test_tran,  /* inout */
                                 const AzTrTreeEnsemble_ReadOnly *ens, 
                                 /*--- output ---*/
                                 AzDvect *v_p, 
                                 int *f_num, 
                                 int *nz_f_num) const 
{
  if (test_data != NULL) {
    feat1.updateMatrix(test_data, ens, b_test_tran); 
    trainer->resetPred(b_test_tran, v_p); 
  }

  /*---  set info  ---*/
  _info(ens, trainer, &feat1, f_num, nz_f_num); 
} 

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
bool AzRgf_Optimizer_Dflt::resetParam(AzParam &p)
{
  bool beVerbose = false; 
  p.swOn(&beVerbose, kw_opt_beVerbose);   
  return beVerbose; 
}

/*------------------------------------------------------------------*/
void AzRgf_Optimizer_Dflt::printHelp(AzHelp &h) const 
{
  trainer->printHelp(h); 
  feat1.printHelp(h); 
  h.begin(Azopt_config, "AzRgf_Optimizer_Dflt", "Weight optimization/correction"); 
  h.item_experimental(kw_opt_beVerbose, help_opt_beVerbose); 
  h.end(); 
}
