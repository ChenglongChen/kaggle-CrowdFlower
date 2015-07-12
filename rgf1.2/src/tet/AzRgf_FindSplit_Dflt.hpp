/* * * * *
 *  AzRgf_FindSplit_Dflt.hpp 
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

#ifndef _AZ_RGF_FIND_SPLIT_DFLT_HPP_
#define _AZ_RGF_FIND_SPLIT_DFLT_HPP_

#include "AzFindSplit.hpp"
#include "AzTrTree_ReadOnly.hpp"
#include "AzRgf_FindSplit.hpp"
#include "AzRegDepth.hpp"
#include "AzParam.hpp"

//! Node split search for RGF.  L2 regularization.  
/*--------------------------------------------------------*/
class AzRgf_FindSplit_Dflt : /* extends */    public virtual AzFindSplit, 
                             /* implements */ public virtual AzRgf_FindSplit
{
protected:
  double lambda, sigma; 
  const AzRegDepth *reg_depth; 

  double nlam, nsig; 
  double p_nlam; //!< L2 reg param for parent (node to be split) 
  double c_nlam; //!< L2 reg param for child (new node after split)
  double p_nsig, c_nsig; 
  bool doUseInternalNodes; 
  const AzTrTreeNode *p_node; //!< parent node (node to be split) 

public:
  AzRgf_FindSplit_Dflt() : reg_depth(NULL), 
                      lambda(-1), sigma(sigma_dflt),  
                      doUseInternalNodes(false), nlam(0), nsig(0), 
                      p_nlam(0), c_nlam(0), p_nsig(0), c_nsig(0), 
                      p_node(NULL) {}
  virtual void begin(const AzTrTree_ReadOnly *tree, 
                   const AzRgf_FindSplit_input &inp, 
                   int inp_min_size); 
  virtual void end() {
    _end(); 
  }
  virtual inline 
  void findSplit(int nx, 
                 /*---  output  ---*/
                 AzTrTsplit *best_split) {
    p_node = tree->node(nx); 
    p_nlam = reg_depth->apply(nlam, p_node->depth); 
    c_nlam = reg_depth->apply(nlam, p_node->depth+1);
    p_nsig = reg_depth->apply(nsig, p_node->depth); 
    c_nsig = reg_depth->apply(nsig, p_node->depth+1); 
    AzFindSplit::_findBestSplit(nx, best_split); 
  }
  virtual void reset(AzParam &param, 
                     const AzRegDepth *inp_reg_depth, 
                     const AzOut &out) 
  {
    reg_depth = inp_reg_depth; 
    if (reg_depth ==  NULL) throw new AzException("AzRgf_FindSplit_Dflt", 
                                                 "null reg_depth"); 
    resetParam(param); 
    printParam(out); 
  }

  virtual void pickFeats(int pick_num, int f_num) {
    AzFindSplit::_pickFeats(pick_num, f_num); 
  }

  virtual void printParam(const AzOut &out) const; 
  virtual void printHelp(AzHelp &h) const; 

protected:
  virtual void resetParam(AzParam &param); 
  virtual double getBestGain(double wsum, 
                     double wysum, 
                     double *best_q) const; 
}; 
#endif 

