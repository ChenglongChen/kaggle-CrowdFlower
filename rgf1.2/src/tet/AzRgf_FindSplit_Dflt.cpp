/* * * * *
 *  AzRgf_FindSplit_Dflt.cpp 
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

#include "AzRgf_FindSplit_Dflt.hpp"
#include "AzHelp.hpp"
#include "AzRgf_kw.hpp"

/*--------------------------------------------------------*/
void AzRgf_FindSplit_Dflt::begin(
                   const AzTrTree_ReadOnly *inp_tree, 
                   const AzRgf_FindSplit_input &inp, /* tx is not used */
                   int inp_min_size)
{
  AzFindSplit::_begin(inp_tree, inp.data, inp.target, inp_min_size); 

  nlam = inp.nn*lambda; 
  nsig = inp.nn*sigma; 
  if (inp.lam_scale != 1) {  /* for numerical stability for expo loss */
    nlam *= inp.lam_scale; 
    nsig *= inp.lam_scale; 
  }

  doUseInternalNodes = tree->usingInternalNodes(); 
}

/*--------------------------------------------------------*/
double AzRgf_FindSplit_Dflt::getBestGain(double wsum,  /* some of data weights */
                                    double wrsum, /* weighted sum of residual */
                                    double *best_q) const
{
  double p = p_node->weight; /* parent's weight */
  double gain = 0; 
  double q = 0; 

  if (doUseInternalNodes) {
    q = wrsum/(wsum+c_nlam); 
    gain = q*q*(wsum+c_nlam); /* n*gain */
  }
  else if (nsig <= 0) { /* L2 only */
    q = (wrsum-c_nlam*p)/(wsum+c_nlam); 
    gain = q*q*(wsum+c_nlam)+(p_nlam-2*c_nlam)*p*p/2; /* "/2" for two child nodes */
              /* n*gain */
    q += p; 
  }
  else { /* L1 and L2; not tested after code change */
    double _wysum = wrsum + wsum*p; 
    if      (_wysum >  c_nsig) q = (_wysum-c_nsig)/(wsum+c_nlam); 
    else if (_wysum < -c_nsig) q = (_wysum+c_nsig)/(wsum+c_nlam); 
    else                       q = 0; 
    double org_losshat = -2*p*_wysum+p*p*(wsum+p_nlam)+2*p_nsig*fabs(p); 
    double new_losshat = -q*q*(wsum+c_nlam); 
    gain = org_losshat - new_losshat; 
  }

  *best_q = q;
  return gain; 
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void AzRgf_FindSplit_Dflt::resetParam(AzParam &p)
{
  /*---  reg param shared with optimizer  ---*/
  p.vFloat(kw_lambda, &lambda); 
  p.vFloat(kw_sigma, &sigma); 

  /*---  override ... ---*/
  p.vFloat(kw_s_lambda, &lambda); 
  p.vFloat(kw_s_sigma, &sigma); 

  if (lambda < 0) {
    throw new AzException(AzInputMissing, "AzRgf_FindSplit_Dflt", 
               kw_lambda, "must be non-negative"); 
  }
  if (sigma < 0) {
    throw new AzException(AzInputNotValid, "AzRgf_FindSplit_Dflt", 
               kw_sigma, "must be non-negative"); 
  }
}

/*--------------------------------------------------------*/
void AzRgf_FindSplit_Dflt::printParam(const AzOut &out) const
{
  if (out.isNull()) return; 

  AzPrint o(out); 
  o.reset_options(); 
  o.set_precision(5); 
  o.ppBegin("AzRgf_FindSplit_Dflt", "Node split", ", "); 
  o.printV(kw_lambda, lambda); 
  o.printV_posiOnly(kw_sigma, sigma); 
  o.ppEnd(); 
}

/*--------------------------------------------------------*/
void AzRgf_FindSplit_Dflt::printHelp(AzHelp &h) const
{
  h.begin(Azsplit_config, "AzRgf_FindSplit_Dflt", "Regularization at node split"); 
  h.item_required(kw_lambda, help_lambda);  
  h.item_experimental(kw_sigma, help_sigma, sigma_dflt); 
  h.item(kw_s_lambda, help_s_lambda);  
  h.item_experimental(kw_s_sigma, help_s_sigma); 
  h.end(); 
}
