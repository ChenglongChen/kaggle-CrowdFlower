/* * * * *
 *  AzTaskTools.hpp 
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

#ifndef _AZ_TASK_TOOLS_HPP_
#define _AZ_TASK_TOOLS_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzSvFeatInfo.hpp" 
#include "AzLoss.hpp"
#include "AzPerfResult.hpp"
#include "AzPrint.hpp"

/* Some functions are overlapping with the legacy code AzCatProc */
/*--------------------------------------------------*/
//! Tools related to classification or regression tasks. 
class AzTaskTools 
{
public: 
  static double analyzeLoss(AzLossType loss_type, 
                            const AzDvect *v_p, 
                            const AzDvect *v_y, 
                            const AzIntArr *inp_ia_dx, 
                            double p_coeff); 

  static void showDist(const AzStrArray *sp_cat, 
                       const AzIntArr *ia_cat, 
                       const char *header, 
                       const AzOut &out); 

  static double eval_breakEven(
                           const AzIntArr *ia_gold, 
                           const AzDvect *v_pval, 
                           const AzStrArray *sp_cat, 
                           const char *eyecatcher, 
                           double *out_best_f=NULL, 
                           double *out_best_acc=NULL); 

  static AzPerfResult eval(const AzDvect *v_p, 
                   const AzDvect *v_y, /* assume y in {+1,-1} */ 
                   AzLossType loss_type) {
    AzOut null_out; 
    AzPerfResult res; 
    eval("", loss_type, NULL, NULL, v_p, v_y, "", null_out, &res); 
    return res; 
  }
  static void eval(const AzDvect *v_p, 
                   const AzDvect *v_y, /* assume y in {+1,-1} */ 
                   AzPerfResult *result) {
    AzOut null_out; 
    eval("", AzLoss_None, NULL, NULL, v_p, v_y, "", null_out, result); 
  }
  static void eval(const AzDvect *v_p, 
                   const AzDvect *v_y, /* assume y in {+1,-1} */ 
                   const AzOut &test_out, 
                   AzPerfResult *result=NULL) {
    AzOut null_out; 
    eval("", AzLoss_None, NULL, NULL, v_p, v_y, "", test_out, result); 
  }
  static void eval(const char *ite_str, 
                   AzLossType loss_type, 
                   const AzIntArr *ia_dx, 
                   const double p_coeff[2], 
                     const AzDvect *v_test_pval, 
                     const AzDvect *v_test_yval, /* assume y in {+1,-1} */ 
                     const char *tt_eyec, 
                     const AzOut &test_out, 
                     AzPerfResult *result=NULL); 

  static int genY(const AzIntArr *ia_cat, 
                             int focus_cat, 
                             double y_posi_val, 
                             double y_nega_val, 
                             AzDvect *v_yval); /* output */

  /*---  for displaying the weights, feature names, etc.  ---*/
  static void dumpWeights(const AzOut &out, 
                        const AzDvect *v_w, 
                        const char *name, 
                        const AzSvFeatInfo *feat, 
                        int print_max, 
                        bool changeLine); 
  static void printPR(AzPrint &o, 
                   int ok, 
                   int t,
                   int g); 
protected:
  static void formatWeight(const AzSvFeatInfo *feat, 
                           int ex, 
                           double val, 
                           AzBytArr *str_out); 
}; 
#endif 
