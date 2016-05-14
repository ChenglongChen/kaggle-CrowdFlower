/* * * * *
 *  AzRgf_FindSplit_TreeReg.cpp 
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

#include "AzRgf_FindSplit_TreeReg.hpp"

/*--------------------------------------------------------*/
void AzRgf_FindSplit_TreeReg::findSplit(int nx, 
                           /*---  output  ---*/
                           AzTrTsplit *best_split)
{
  if (tree->usingInternalNodes()) {
    throw new AzException("AzRgf_FindSplit_TreeReg::findSplit", 
                          "can't coexist with UseInternalNodes"); 
  }

  reg->reset_forNewLeaf(nx, tree, reg_depth); 
  dR = ddR = 0; 
  reg->penalty_deriv(&dR, &ddR); 
  AzRgf_FindSplit_Dflt::findSplit(nx, best_split); 
}

/*--------------------------------------------------------*/
double AzRgf_FindSplit_TreeReg::evalSplit(
                             const Az_forFindSplit i[2],
                             double bestP[2])
 const
{
  double d[2]; /* delta */
  int ix; 
  for (ix = 0; ix < 2; ++ix) {
    double wrsum = i[ix].wy_sum; 
    d[ix] = (wrsum-nlam*dR)/(i[ix].w_sum+nlam*ddR); 
    bestP[ix] = p_node->weight + d[ix]; 
  }

  double penalty_diff = reg->penalty_diff(d); /* new - old */

  double gain = 2*d[0]*i[0].wy_sum - d[0]*d[0]*i[0].w_sum
              + 2*d[1]*i[1].wy_sum - d[1]*d[1]*i[1].w_sum; 

  gain -= 2 * nlam * penalty_diff; 
  /* "2*" b/c penalty is sum v^2/2 */

  return gain; 
}
