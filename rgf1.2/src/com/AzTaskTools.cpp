/* * * * *
 *  AzTaskTools.cpp 
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


#include "AzStrPool.hpp"
#include "AzTools.hpp"
#include "AzTaskTools.hpp"
#include "AzPrint.hpp"

/*------------------------------------------------------------------*/
void AzTaskTools::showDist(const AzStrArray *sp_cat, 
                           const AzIntArr *ia_cat, 
                           const char *header, 
                           const AzOut &out)
{
  if (out.isNull()) return; 
  AzPrint o(out); 
  o.printBegin(header, ", ", ":"); 
  int cx; 
  for (cx = 0; cx < sp_cat->size(); ++cx) {
    int pop = ia_cat->count(cx); 
    o.print(sp_cat->c_str(cx), pop, 5); 
    double ratio = (double)pop/(double)ia_cat->size(); 
    o.inParen(ratio, 3); 
  }
  o.printEnd(); 
}

/*------------------------------------------------------------------*/
int AzTaskTools::genY(const AzIntArr *ia_cat, 
                      int focus_cat, 
                      double y_posi_val, 
                      double y_nega_val, 
                      AzDvect *v_yval) /* output */
{
  int posi_count = 0; 
  int data_num = ia_cat->size(); 
  v_yval->reform(data_num); 
  double *y_val = v_yval->point_u(); 
  const int *cat = ia_cat->point(); 
  int dx; 
  for (dx = 0; dx < data_num; ++dx) {
    if (cat[dx] == focus_cat) {
      y_val[dx] = y_posi_val; 
      ++posi_count; 
    }
    else {
      y_val[dx] = y_nega_val; 
    }
  }

  return posi_count; 
}

/*------------------------------------------------------------------*/
double AzTaskTools::analyzeLoss(AzLossType loss_type, 
                            const AzDvect *v_p, 
                            const AzDvect *v_y, 
                            const AzIntArr *inp_ia_dx, 
                            double p_coeff)
{
  if (loss_type == AzLoss_None) {
    return -1; 
  }
  const double *p = v_p->point(); 
  const double *y = v_y->point(); 

  AzIntArr iq; 
  const AzIntArr *ia_dx = inp_ia_dx; 
  if (ia_dx == NULL) {
    iq.range(0, v_p->rowNum()); 
    ia_dx = &iq;     
  }

  double total_loss = 0; 
  int num; 
  const int *dxs = ia_dx->point(&num); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int dx = dxs[ix]; 

    double loss = AzLoss::getLoss(loss_type, p[dx]*p_coeff, y[dx]); 
    total_loss += loss; 
  }

  double avg_loss = 0; 
  if (num > 0) {
    avg_loss = total_loss / (double)num; 
  }
  return avg_loss; 
}

/*------------------------------------------------------------------*/
void AzTaskTools::eval(const char *ite_str, 
                     AzLossType loss_type, 
                     const AzIntArr *ia_dx, 
                     const double p_coeff[2], /* for normalized loss, may be NULL */
                     const AzDvect *v_test_pval, 
                     const AzDvect *v_test_yval, /* assume y in {+1,-1} */ 
                     const char *tt_eyec, 
                     const AzOut &test_out, 
                     AzPerfResult *result)
{
  bool doBreakEven = true; 

  int data_num = v_test_pval->rowNum(); 
  const double *p = v_test_pval->point(); 
  const double *y = v_test_yval->point(); 
  if (v_test_yval->rowNum() != data_num) {
    throw new AzException("AzTaskTools::eval", "#data conflict"); 
  }

  AzIntArr ia_cat_breakEven;  

  double tp=0, tn=0, fp=0, fn=0; 
  double posi=0, rmse=0; 

  int num = data_num; 
  const int *dxs = NULL; 
  if (ia_dx != NULL) {
    dxs = ia_dx->point(&num); 
  }

  AzDvect v_p(num); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int dx = ix; 
    if (dxs != NULL) dx = dxs[ix]; 

    double p_val = p[dx]; 
    if (p_val > 0) {
      if (y[dx] > 0) ++tp; 
      else           ++fp; 
    }
    else {
      if (y[dx] > 0) ++fn; 
      else           ++tn;       
    }
    if (y[dx] > 0) {
      ++posi; 
    }
    if (doBreakEven) {
      if (y[dx] > 0) ia_cat_breakEven.put(0);  /* target */
      else           ia_cat_breakEven.put(1);  /* other */
      v_p.set(ix, p_val); 
    }

    double diff = p_val - y[dx]; 
    rmse += (diff*diff); 
  }
  double uloss_avg = 
  analyzeLoss(loss_type, v_test_pval, v_test_yval, ia_dx, 1); 
  double nloss1_avg = 0, nloss2_avg = 0; 
  if (AzLoss::isExpoFamily(loss_type) && 
      p_coeff != NULL) {
    nloss1_avg = analyzeLoss(loss_type, v_test_pval, v_test_yval, ia_dx, p_coeff[0]); 
    nloss2_avg = analyzeLoss(loss_type, v_test_pval, v_test_yval, ia_dx, p_coeff[1]); 
  }

  rmse = sqrt(rmse/(double)num); 
  double acc = (tp+tn) / (double)num; 
  double precision = 0; 
  if (tp + fp > 0) {
    precision = tp / (tp + fp); 
  }
  double recall = 0; 
  if (posi > 0) {
    recall = tp / posi; 
  }
  double f1 = 0; 
  if (precision + recall > 0) {
    f1 = 2 * precision * recall / (precision + recall); 
  }

  double breakEven_f=-1,breakEven_acc=-1; 
  if (doBreakEven) {
    AzStrPool sp_cat_breakEven; 
    sp_cat_breakEven.put("target"); 
    sp_cat_breakEven.put(AzOtherCat); 
    eval_breakEven(&ia_cat_breakEven, &v_p, 
                   &sp_cat_breakEven, tt_eyec, 
                   &breakEven_f, &breakEven_acc); 
  }

  if (!test_out.isNull()) {
    AzPrint o(test_out); 
    o.printBegin("", ","); 
    o.print(ite_str); o.print_cont(tt_eyec, "-ite", ite_str); 
    o.print("prf"); o.print(precision,4); o.print(recall,4); o.print(f1,4); 
    o.print("acc", acc,4); 
    o.print("be_f", breakEven_f,4); 
    o.print("be_acc", breakEven_acc,4); 
    o.print("rmse", rmse,4); 
    o.print(AzLoss::lossName(loss_type)); 
    o.print("uloss", uloss_avg, 5); 
    if (AzLoss::isExpoFamily(loss_type)) {  
      o.print("nloss1", nloss1_avg, 5); 
      o.print("nloss2", nloss2_avg, 5); 
    }
    o.print("ok",(int)tp); o.print("t",(int)(tp+fp)); o.print("g",(int)posi); 
    o.print("#data", num); 
    o.printEnd(); 
  }
  if (result != NULL) {
    result->put(precision, recall, f1, acc, breakEven_f, breakEven_acc, rmse, uloss_avg); 
  }
}

/* only for binary categorization, and the first cat is the target */
/*--------------------------------------------------------*/
double AzTaskTools::eval_breakEven(const AzIntArr *ia_gold, 
                           const AzDvect *v_pval, 
                           const AzStrArray *sp_cat, 
                           const char *eyecatcher, 
                           double *out_best_f, 
                           double *out_best_acc)
{
  int cat_num = sp_cat->size(); 
  if (cat_num != 2) return -1; 

  int other_cat = -1; 
  int cx; 
  for (cx = 0; cx < cat_num; ++cx) {
    AzBytArr str_cat; 
    sp_cat->get(cx, &str_cat); 
    if (str_cat.compare(AzOtherCat) == 0) {
      other_cat = cx; 
      break; 
    }
  }
  if (other_cat != 1) {
    return -1; 
  }

  int target_cat = 0; 

  int data_num; 
  const int *gold_cat = ia_gold->point(&data_num); 
  if (v_pval->rowNum() != data_num) {
    throw new AzException("AzTaskTools::eval", "number mismatch"); 
  }
  const double *p_val = v_pval->point(); 

  double g = 0; 
  AzIFarr ifa_cat_pval; 
  int dx; 
  for (dx = 0; dx < data_num; ++dx) {
    ifa_cat_pval.put(gold_cat[dx], p_val[dx]); 
    if (gold_cat[dx] == target_cat) {
      ++g; 
    }
  }
  ifa_cat_pval.sort_Float(false); 

  double best_f=0, best_p=0, best_r=0, best_f_border=0;
  double best_acc=0, best_a_border=0;  
  double ok = 0; 
  int ix; 
  for (ix = 0; ix < data_num; ++ix) {
    /*---  cut after this  ---*/
    double t = (double)(ix+1); 
    int my_cat; 
    double val = ifa_cat_pval.get(ix, &my_cat); 
    if (my_cat == target_cat) {
      ++ok; 
    }

    double prec = ok / t; 
    double recall = ok / g; 
    double f1 = (2*prec*recall)/(prec+recall); 
    if (f1 > best_f) {
      best_f = f1; 
      best_p = prec; 
      best_r = recall; 
      best_f_border = val; 
    }

    double n = (double)data_num; 
    double acc = (n-t-g+2*ok)/n; 
    if (acc > best_acc) {
      best_acc = acc; 
      best_a_border = val; 
    }
  }

  if (out_best_f != NULL) *out_best_f = best_f; 
  if (out_best_acc != NULL) *out_best_acc = best_acc; 

  return best_f; 
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
void AzTaskTools::dumpWeights(const AzOut &out, 
                        const AzDvect *v_w,
                        const char *name, 
                        const AzSvFeatInfo *feat, 
                        int print_max, 
                        bool changeLine) 
{
  if (out.isNull()) return; 

  AzPrint o(out); 
  o.write(name); 
  o.writeln("  -----"); 

  AzIFarr ifa_ex_weight; 
  v_w->nonZero(&ifa_ex_weight); 

  int num = ifa_ex_weight.size(); 
  ifa_ex_weight.sort_Float(false); /* descending order */

  /*-----  print largest weights  -----*/
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    if (ix >= print_max) break;

    int ex; 
    double val = ifa_ex_weight.get(ix, &ex); 
    AzBytArr str_weight; 
    formatWeight(feat, ex, val, &str_weight); 
    o.write(str_weight); 
    if (changeLine) {
      o.newLine(); 
    }
  }
  o.newLine(); o.newLine(); 

  if (num > print_max) {
    /*-----  print smallest weights  -----*/
    for (ix = num - 1; ix >= 0; --ix) {
      if (num - ix > print_max) break;

      int ex; 
      double val = ifa_ex_weight.get(ix, &ex); 
      AzBytArr str_weight; 
      formatWeight(feat, ex, val, &str_weight); 
      o.write(str_weight); 
      if (changeLine) {
        o.newLine(); 
      }
    }
    o.newLine(); o.newLine(); 
  }
  o.flush(); 
}

/*------------------------------------------------------------------*/
void AzTaskTools::formatWeight(const AzSvFeatInfo *feat, 
                              int ex, 
                              double val, 
                              AzBytArr *o) 
{
  /* sprintf(work, "(%5.3f,", val); */
  o->c("("); o->cn(val,4); o->c(","); 
  if (feat == NULL) {
    /* sprintf(work, "%ld) ", ex); */
    o->cn(ex); o->c(") "); 
  }
  else {
    feat->concatDesc(ex, o); 
    o->c(") "); 
  }
}

/*------------------------------------------------------------------*/
void AzTaskTools::printPR(AzPrint &o, 
                   int ok, 
                   int t,
                   int g)
{
  double p=0,r=0; 
  if (t > 0) p = (double)ok/(double)t; 
  if (g > 0) r = (double)ok/(double)g; 

  o.print("pr"); 
  o.reset_options(); 
  o.print((double)(p*100), 3); 
  o.print((double)(r*100), 3); 
}
