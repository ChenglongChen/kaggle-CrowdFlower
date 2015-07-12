/* * * * *
 *  AzLoss.cpp 
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

#include "AzLoss.hpp"

/*--------------------------------------------------------*/
double AzLoss::getLoss(AzLossType loss_type, 
                       double p, double y, 
                       double py_adjust) /* used only by Expo */
{
  const char *eyec = "AzLoss::getLoss"; 

  double py = p*y;
  double loss = 0; 

  if (loss_type == AzLoss_Square || 
           loss_type == AzLoss_LS) {
    double r = y-p; /* residual */
    loss = r*r/2; 
  }
  else if (loss_type == AzLoss_Expo) { /* y in {+1,-1} */
    py -= py_adjust; 
    double ee = my_exp(-py); 
    loss = ee; 
  }
  else if (loss_type == AzLoss_Logistic1) {
    double ee = my_exp(-py); 
    loss = log(1+ee); 
  }
  else if (loss_type == AzLoss_LogRe) {
    double ee = my_exp(-p); 
    double q = 1/(1+ee); 
    loss = -y*log(q)-(1-y)*log(1-q); 
  }
  else if (loss_type == AzLoss_LogRe2) {
    double aa = 1; 
    double dd = my_exp(aa*(p-y));  
    loss = log(1+dd)*2/aa - (p-y) - 2*log((double)2)/aa; 
  }
  else if (loss_type == AzLoss_Logistic2) {
    double ee = my_exp(-2*py); 
    loss = log(1+ee); 
  }
  else if (loss_type == AzLoss_ModSquare) { /* for classification */
    if (py <= 1) {
      double r = y-p; 
      loss = r*r/2; 
    }
    else {
      loss = 0; 
    }
  }
  else if (loss_type == AzLoss_ModHuber) {
    if (py <= -1) {
      loss = -2*py; 
    }
    else if (py > -1 && py < 1) {
      double r = y-p; 
      loss = r*r/2; 
    }
  }
  else {
    throw new AzException(eyec, "unsupported loss type"); 
  }
  return loss; 
}

/*--------------------------------------------------------*/
AzLosses AzLoss::getLosses(AzLossType loss_type, 
                           double p, double y, 
                           double py_adjust)
{
  const char *eyec = "AzLoss::getLosses"; 

  AzLosses o; 

  if (loss_type == AzLoss_Square || 
           loss_type == AzLoss_LS) {
    double r = y-p; /* residual */
    o.loss2 = 1; 
    o._loss1 = r; 
  }
  else if (loss_type == AzLoss_Expo) { /* y in {+1,-1} */
    double py = p*y - py_adjust; 
    double ee = my_exp(-py); 
    o.loss2 = ee*y*y; 
    o._loss1 = y*ee; 
  }
  else if (loss_type == AzLoss_Logistic1) {  
    double py = p*y; 
    double ee = my_exp(-py);  
    o.loss2 = y*y*ee/(1+ee)/(1+ee); 
    o._loss1 = y*ee/(1+ee); 
  }
  else if (loss_type == AzLoss_LogRe) {  
    double ee = my_exp(-p);  
    double q = 1/(1+ee); 
    o.loss2 = q*(1-q); 
    o._loss1 = y - q; 
  }
  else if (loss_type == AzLoss_LogRe2) {
    double aa = 1; 
    double ee = my_exp(-aa*(p-y));  
    double q = 1/(1+ee); 
    o.loss2 = 2*aa*q*(1-q); 
    o._loss1 = 1 - 2*q; 
  }
  else if (loss_type == AzLoss_Logistic2) {
    double py = p*y; 
    double ee = my_exp(-2*py); 
    o.loss2 = 4*y*y*ee/(1+ee)/(1+ee); 
    o._loss1 = 2*y*ee/(1+ee); 
  }
  else {
    throw new AzException(eyec, "unsupported loss type"); 
  }
  return o; 
}

/*------------------------------------------------------------------*/
/* 
 * This is for speeding up AzOptOntTree.  It's the same as calling 
 * getLosses from the loop but faster especially for square loss. 
 *
 */
void AzLoss::sum_deriv(AzLossType loss_type, 
                       const int *dxs, 
                       int dx_num, 
                       const double *p, 
                       const double *y, 
                       double py_avg, 
                       /*---  output  ---*/
                       double &nega_dL, 
                       double &ddL) 
{
  nega_dL = 0; 
  ddL = 0; 
  if (loss_type == AzLoss_Square || 
      loss_type == AzLoss_LS) {
    /* double r = y-p; */
    /* o.loss2 = 1; */
    /* o._loss1 = r; */
    ddL = (double)dx_num; 
    int ix; 
    for (ix = 0; ix < dx_num; ++ix) {
      int dx = dxs[ix]; 
      nega_dL += (y[dx]-p[dx]); 
    }
  }
  else if (loss_type == AzLoss_Expo) {
    int ix; 
    for (ix = 0; ix < dx_num; ++ix) {
      int dx = dxs[ix];       
      double py = p[dx]*y[dx]; 
      py -= py_avg; /* for numerical stability */
      double ee = my_exp(-py); 
      ddL += ee;            /* exp(-py)*y*y */
      nega_dL += y[dx]*ee;  /* exp(-py)*y */
    }
  }
  else {
    int ix; 
    for (ix = 0; ix < dx_num; ++ix) {
      int dx = dxs[ix]; 
      AzLosses o = getLosses(loss_type, p[dx], y[dx], py_avg); 
      ddL += o.loss2; 
      nega_dL += o._loss1; 
    }
    /* throw new AzException("AzLoss::sum_deriv", "unsupported loss type"); */
  }
}

/*------------------------------------------------------------------*/
/* 
 * This is for speeding up AzOptOntTree.  It's the same as calling 
 * getLosses from the loop but faster especially for square loss. 
 *
 */
void AzLoss::sum_deriv_weighted(AzLossType loss_type, 
                       const int *dxs, 
                       int dx_num, 
                       const double *p, 
                       const double *y, 
                       const double *dw, 
                       double py_avg, 
                       /*---  output  ---*/
                       double &nega_dL, 
                       double &ddL) 
{
  nega_dL = 0; 
  ddL = 0; 
  if (loss_type == AzLoss_Square || 
      loss_type == AzLoss_LS) {
    /* double r = y-p; */
    /* o.loss2 = 1; */
    /* o._loss1 = r; */
    int ix; 
    for (ix = 0; ix < dx_num; ++ix) {
      int dx = dxs[ix]; 
      nega_dL += dw[dx]*(y[dx]-p[dx]); 
      ddL += dw[dx]; 
    }
  }
  else if (loss_type == AzLoss_Expo) {
    int ix; 
    for (ix = 0; ix < dx_num; ++ix) {
      int dx = dxs[ix];       
      double py = p[dx]*y[dx]; 
      py -= py_avg; /* for numerical stability */
      double ee = my_exp(-py); 
      ddL += (dw[dx]*ee);            /* exp(-py)*y*y */
      nega_dL += (dw[dx]*y[dx]*ee);  /* exp(-py)*y */
    }
  }
  else {
    int ix; 
    for (ix = 0; ix < dx_num; ++ix) {
      int dx = dxs[ix]; 
      AzLosses o = getLosses(loss_type, p[dx], y[dx], py_avg); 
      ddL += (dw[dx]*o.loss2); 
      nega_dL += (dw[dx]*o._loss1); 
    }
    /* throw new AzException("AzLoss::sum_deriv_weighted", "unsupported loss type"); */
  }
}

/*------------------------------------------------------------------*/
void AzLoss::help_lines(int level, AzDataPool<AzBytArr> *pool_desc) {
  AzIntArr ia_l_type; 
  ia_l_type.put(AzLoss_Square); 
  ia_l_type.put(AzLoss_Logistic1); 
  ia_l_type.put(AzLoss_Expo); 
  int num; 
  const int *l_types = ia_l_type.point(&num); 

  const char *dlm = "|"; 
  AzBytArr *s = pool_desc->new_slot(); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    if (ix > 0) {
      s->concat(dlm); 
    }
    AzLossType loss_type = (AzLossType)l_types[ix]; 
    s->c(lossName(loss_type)); 
  }

  for (ix = 0; ix < num; ++ix) {
    AzLossType loss_type = (AzLossType)l_types[ix]; 
    AzBytArr *s = pool_desc->new_slot(); 
    s->c(lossName(loss_type)); s->c(": "); 
    s->c(loss_desc[loss_type]); 
  }  
}

/*------------------------------------------------------------------*/
AzLossType AzLoss::lossType(const char *param)
{
  AzBytArr str_param(param); 
  int ix; 
  for (ix = 0; ix < AzLossType_Num; ++ix) {
    AzLossType loss_type = (AzLossType)ix; 
    if (str_param.compare(lossName(loss_type)) == 0) {
      return loss_type; 
    }
  }
  return AzLoss_None; 
}

#define TRUNCATE_PY_AVG(x) MIN(500,MAX(-500,(x)))

/*--------------------------------------------------------*/
double AzLoss::py_avg(const AzDvect *v_p, 
                      const AzDvect *v_y, 
                      const AzIntArr *ia_dx) /* may be NULL */
{
  AzDvect v_py(v_y); 
  v_py.scale(v_p); /* component-wise multiplication */
  double py_sum = v_py.sum(ia_dx); 
  int num = v_py.rowNum(); 
  if (ia_dx != NULL) num = ia_dx->size();   
  double py_avg = py_sum / (double)num; 
  py_avg = TRUNCATE_PY_AVG(py_avg); 

  return py_avg; 
}

/*------------------------------------------------------------------*/
double AzLoss::negativeDeriv12(AzLossType loss_type, 
                           const AzDvect *v_p, 
                           const AzDvect *v_y, 
                           const AzIntArr *ia_dx, /* may be NULL */
                           /*---  output  ---*/
                           double *out_py_adjust, 
                           AzDvect *v_1,  /* -L' */
                           AzDvect *v_2)  /* L'' */
{
  const double *p = v_p->point(); 
  const double *y = v_y->point(); 
  int data_num = v_p->rowNum(); 
  if (v_y->rowNum() != data_num) {
    throw new AzException("AzLoss::negativeDeriv12", "dim conflict"); 
  }

  double py_adjust = 0; 
  if (out_py_adjust != NULL) {
    *out_py_adjust = 0; 
  }
#if 0 
  if (isExpoFamily(loss_type) && 
#else
  if (loss_type == AzLoss_Expo &&
#endif
      out_py_adjust != NULL) {
    py_adjust = py_avg(v_p, v_y, ia_dx); 
    *out_py_adjust = py_adjust; 
  }

  if (v_1->rowNum() != data_num) {
    v_1->reform(data_num); 
  }
  double *out1 = v_1->point_u(); 
  
  double *out2 = NULL; 
  if (v_2 != NULL) {
    if (v_2->rowNum() != data_num) {
      v_2->reform(data_num); 
    }
    out2 = v_2->point_u(); 
  }

  const int *dxs = NULL; 
  int dx_num = data_num; 
  if (ia_dx != NULL) {
    dxs = ia_dx->point(&dx_num); 
  }
  int ix; 
  for (ix = 0; ix < dx_num; ++ix) {
    int dx = ix; 
    if (dxs != NULL) dx = dxs[ix]; 

    AzLosses o; 
    o = getLosses(loss_type, p[dx], y[dx], py_adjust); 
    out1[dx] = o._loss1; 
    if (out2 != NULL) {
      out2[dx] = o.loss2; 
    }
  }

  double lam_scale = 1; 
  if (py_adjust != 0) {
    lam_scale = lamScale(py_adjust); 
  }
  return lam_scale; 
}
