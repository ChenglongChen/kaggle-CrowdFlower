/* * * * *
 *  AzLoss.hpp 
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

#ifndef _AZ_LOSS_HPP_
#define _AZ_LOSS_HPP_

#include "AzUtil.hpp"
#include "AzDmat.hpp"

enum AzLossType {
  /*---  for classification  ---*/
  AzLoss_ModHuber = 0, 
  AzLoss_Logistic1 = 1,  /* log loss for classification: y in {1,-1} */
  AzLoss_Expo = 2,
  AzLoss_ModSquare = 3,

  /*---  for regression  ---*/
  AzLoss_Square = 4, 
  AzLoss_Huber = 5,

  AzLoss_LS = 6, 
  AzLoss_Logistic2 = 7, 
  AzLoss_LogRe = 8, /* log loss for regression: y in [0,1] */
  AzLoss_LogRe2 = 9, 
  AzLoss_L1L2 = 10, 
  AzLoss_Xtemp = 11, 
  AzLoss_None = 12, 
};
#define AzLossType_Num 13
static const char *loss_str[AzLossType_Num] = {
  "ModHuber", "Log", "Expo", "ModLS", "LS", "Huber", "DummyLS", 
  "Logit", "LogRe", "LogRe2", "L1L2", "Xtemp", "None", 
}; 
static const char *loss_desc[AzLossType_Num] = {
  /* ModHuber */  "Modified Huber loss -2py if py<=-1;(p-y)^2/2 if -1<py<1;otherwise 0", 
  /* Log   */     "Log loss log(1+exp(-py)) for y=1,-1", 
  /* Expo  */     "Exponential loss exp(-py) for y=1,-1", 
  /* ModLS */     "Modified square loss (p-y)^2 if py<=1;otherwise 0", 
  /* LS   */      "Square loss (p-y)^2/2", 
  /* Huber */     "Huber loss for regression -2py if py<=-1;(p-y)^2/2 if -1<py<1;otherwise 0", 
  /* DummyLS */   "Square loss for maintenance purpose only", 
  /* Logit */     "for LogitBoost log(1+exp(-2py))", 
  /* LogRe */     "Log loss for regression for y in [0,1]", 
  /* LogRe2 */    "Like |r| with large r (residual); for regression for real y", 
  /* L1L2 */      "L1-L2 hybrid", 
  /* Xtemp */     "Xtemp", 
  /* None" */     "Unknown", 
}; 

//! data structure for loss and derivatives of loss. 
class AzLosses {
public:
  double loss2;         //! 2nd derivative of loss
  double _loss1; //! negative derivative of loss
  AzLosses() {
    _loss1 = loss2 = 0; 
  }
}; 


#define my_exp(x) exp(MAX(-500,MIN(500,(x))))

//! Compute loss and loss derivatives.   
class AzLoss
{
public: 
  static bool isExpoFamily(AzLossType loss_type) {
    if (loss_type == AzLoss_Logistic1||
        loss_type == AzLoss_Expo || 
        loss_type == AzLoss_Logistic2 || 
        loss_type == AzLoss_LogRe || 
        loss_type == AzLoss_LogRe2 || 
        loss_type == AzLoss_Xtemp) {
      return true; 
    }
    return false; 
  }

  static double py_avg(const AzDvect *v_p, 
                       const AzDvect *v_y, 
                       const AzIntArr *ia_dx=NULL); 
  inline static double lamScale(double py_avg) {
    return exp(py_avg); 
  }

  static void sum_deriv(AzLossType loss_type, 
                       const int *dxs, 
                       int dx_num, 
                       const double *p, 
                       const double *y, 
                       double py_avg, 
                       /*---  output  ---*/
                       double &nega_dL, 
                       double &ddL); 
  static void sum_deriv_weighted(AzLossType loss_type, 
                       const int *dxs, 
                       int dx_num, 
                       const double *p, 
                       const double *y, 
                       const double *dw, 
                       double py_avg, 
                       /*---  output  ---*/
                       double &nega_dL, 
                       double &ddL); 

  static AzLosses getLosses(AzLossType loss_type, 
                            double p, double y, 
                            double py_adjust=0); 
  static double getLoss(AzLossType loss_type,
                        double p_val, double y_val, 
                        double py_adjust=0); 

  /*---  ---*/
  static void help_lines(int level, AzDataPool<AzBytArr> *pool_desc); 
  static AzLossType lossType(const char *param); 
  static double negativeDeriv12(AzLossType loss_type, 
                           const AzDvect *v_p, 
                           const AzDvect *v_y,  
                           const AzIntArr *ia_dx, //!< NULL: all 
                           /*---  output  ---*/
                           double *out_py_adjust, 
                           AzDvect *v_1,  /* -L' */
                           AzDvect *v_2); /* L'' */

  inline static const char *lossName(AzLossType loss_type) {
    if (loss_type < 0 || loss_type >= AzLossType_Num) {
      return "???"; 
    }
    return loss_str[loss_type]; 
  }
}; 

#endif 


