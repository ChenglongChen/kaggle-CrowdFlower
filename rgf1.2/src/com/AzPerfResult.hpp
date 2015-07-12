/* * * * *
 *  AzPerfResult.hpp 
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

#ifndef _AZ_PERF_RESULT_HPP_
#define _AZ_PERF_RESULT_HPP_

#include "AzUtil.hpp"

#define AzOtherCat "_X_" 

/*--------------------------------------------------*/
enum AzPerfType {
  AzPerfType_Acc = 0, 
  AzPerfType_RMSE = 1, 
}; 
#define AzPerfType_Num 2
static const char *perf_str[AzPerfType_Num] = {
  "acc", "rmse", 
}; 

/*--------------------------------------------------*/
class AzPerfResult {
public: 
  AzPerfResult() {
    p=r=f=acc=breakEven_f=breakEven_acc=rmse=loss=-1; 
  }
  double p, r, f, acc, breakEven_f, breakEven_acc, rmse, loss; 
  inline void put(double inp_p, double inp_r, double inp_f, double inp_acc, 
                  double inp_be_f, double inp_be_acc, 
                  double inp_rmse, double inp_loss) {
    p = inp_p; 
    r = inp_r; 
    f = inp_f; 
    acc = inp_acc; 
    breakEven_f = inp_be_f; 
    breakEven_acc = inp_be_acc; 
    rmse = inp_rmse; 
    loss=inp_loss; 
  }
  double getPerf(AzPerfType p_type) {
    if (p_type == AzPerfType_Acc) return acc; 
    if (p_type == AzPerfType_RMSE) return rmse; 
    return -1; 
  }
  static const char *getPerfStr(AzPerfType p_type) {
    if (p_type < 0 || 
        p_type >= AzPerfType_Num) return "???"; 
    return perf_str[p_type]; 
  }

  static double isBetter(AzPerfType p_type, 
                         double p, double comp_p) {
    /*---  negative means unset  ---*/
    if (p < 0) return false; 
    if (comp_p < 0) return true; 

    if (p_type == AzPerfType_RMSE) {
      if (p < comp_p) return true; 
    }
    else {
      if (p > comp_p) return true; 
    }
    return false; 
  }
  void zeroOut() {
    p=r=f=acc=breakEven_f=breakEven_acc=rmse=loss=0; 
  }
  void add(const AzPerfResult *inp) {
    p+=inp->p; 
    r+=inp->r; 
    f+=inp->f; 
    acc+=inp->acc; 
    breakEven_f=inp->breakEven_f; 
    breakEven_acc=inp->breakEven_acc; 
    rmse+=inp->rmse; 
    loss+=inp->loss; 
  }
  void multiply(double val) {
    p*=val; 
    r*=val; 
    f*=val; 
    acc*=val; 
    breakEven_f*=val; 
    breakEven_acc*=val; 
    rmse*=val; 
    loss*=val; 
  }
}; 
#endif 
