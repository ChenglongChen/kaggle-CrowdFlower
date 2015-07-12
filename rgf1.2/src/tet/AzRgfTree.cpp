/* * * * *
 *  AzRgfTree.cpp 
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

#include "AzRgfTree.hpp"
#include "AzHelp.hpp"
#include "AzRgf_kw.hpp"

/*--------------------------------------------------------*/
void AzRgfTree::findSplit(AzRgf_FindSplit *fs, 
                          const AzRgf_FindSplit_input &inp, 
                          bool doRefreshAll, 
                          /*---  output  ---*/
                          AzTrTsplit *best_split) const 
{
  const char *eyec = "AzRgfTree::findSplit"; 
  if (nodes_used <= 0) {
    return; 
  }

  AzTrTree::_checkNodes(eyec); 
  int leaf_num = leafNum(); 
  if (max_leaf_num > 0) {
    if (leaf_num >= max_leaf_num) {
      return;   
    }
  }

  _findSplit_begin(fs, inp); 

  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (!nodes[nx].isLeaf()) continue; 

    /*---  ---*/
    if (max_depth > 0 && nodes[nx].depth >= max_depth) {
      continue; 
    }
    if (min_size > 0 && nodes[nx].dxs_num < min_size*2) {
      continue; 
    }

    _findSplit(fs, nx, doRefreshAll); 

    if (split[nx]->fx >= 0 && 
        split[nx]->gain > best_split->gain) {
      best_split->reset(split[nx], inp.tx, nx); 
    }
  }                              
  _findSplit_end(fs); 
}

/*--------------------------------------------------------*/
void AzRgfTree::removeSplitAssessment() 
{
  if (split != NULL) {
    int nx; 
    for (nx = 0; nx < nodes_used; ++nx) {
      delete split[nx]; split[nx] = NULL; 
    }
  }
}

/*--------------------------------------------------------*/
void AzRgfTree::adjustParam()
{
  if (max_depth > 0) {
    int num = (int)pow((double)2, max_depth); 

    if (max_leaf_num < 0) {
      max_leaf_num = num; 
    }
    else {
      max_leaf_num = MIN(max_leaf_num, num); 
    }
  }
}

/*--------------------------------------------------------*/
void AzRgfTree::resetWeights()
{
  AzTrTree::_checkNodes("AzRgfTree::resetWeights"); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    nodes[nx].weight = 0; 
  }
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void AzRgfTree::storeDataIndexes()
{
  if (!wk.canStore()) return; 
  if (wk.isStored()) {
    if (wk.node_num != nodes_used) {
      throw new AzException("AzRgfTree::storeDataIndexes", "conflict in #node"); 
    }
    return; 
  }

  AzIntArr ia_dxs_num; 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    ia_dxs_num.put(nodes[nx].dxs_num); 
  }

#if 0 
  wk.file->open("ab"); 
#endif 

  int fsize = wk.file->size_under2G("AzRgfTree::storeDataIndexes tempfile"); 
  wk.set(fsize, nodes_used); 
  wk.file->seek(fsize); 
  ia_root_dx.write(wk.file); 
  ia_dxs_num.write(wk.file); 

#if 0 
  wk.file->close(true); 
#endif 

  releaseDataIndexes(); 
}

/*--------------------------------------------------------*/
int AzRgfTree::estimateSizeofDataIndexes(int data_num) const
{
  return (data_num+1) * sizeof(int) * 2; 
}

/*--------------------------------------------------------*/
void AzRgfTree::releaseDataIndexes()
{
  if (!wk.isStored()) return; 

  ia_root_dx.reset(); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    nodes[nx].reset_data_indexes(NULL); 
  }
}

/*--------------------------------------------------------*/
void AzRgfTree::restoreDataIndexes()
{
  if (!wk.isStored()) return; 

  const char *eyec = "AzRgfTree::restoreDataIndexes"; 
  if (ia_root_dx.size() > 0) {
    throw new AzException(eyec, "no need to restore?!"); 
  }
  AzIntArr ia_dxs_num; 
#if 0 
  wk.file->open("rb"); 
#endif 
  wk.file->seek(wk.offset); 
  ia_root_dx.read(wk.file); 
  ia_dxs_num.read(wk.file); 
#if 0 
  wk.file->close(); 
#endif 
  if (ia_dxs_num.size() != nodes_used) {
    throw new AzException(eyec, "conflict in #node"); 
  }
  const int *dxs_num = ia_dxs_num.point(); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].dxs_num != dxs_num[nx]) {
      throw new AzException(eyec, "conflict in #data"); 
    }
    if (nodes[nx].dxs_offset+nodes[nx].dxs_num > ia_root_dx.size()) {
      throw new AzException(eyec, "conflict in offset"); 
    }
    nodes[nx].reset_data_indexes(ia_root_dx.point() + nodes[nx].dxs_offset); 

  }
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void AzRgfTree::resetParam(AzParam &p)
{
  p.vInt(kw_max_depth, &max_depth); 
  p.vInt(kw_min_size, &min_size); 
  p.vInt(kw_max_leaf_num, &max_leaf_num); 

  p.swOn(&doUseInternalNodes, kw_doUseInternalNodes); 
  p.swOn(&beVerbose, kw_tree_beVerbose); 

  if (!beVerbose) {
    my_dmp_out.deactivate(); 
    out.deactivate(); 
  }

  adjustParam(); 
}

/*--------------------------------------------------------*/
void AzRgfTree::printParam(const AzOut &out) const
{
  if (out.isNull()) return; 

  AzPrint o(out); 
  o.reset_options(); 
  o.ppBegin("AzRgfTree", "Tree-level", ", "); 
  o.printV(kw_max_depth, max_depth); 
  o.printV(kw_min_size, min_size); 
  o.printV(kw_max_leaf_num, max_leaf_num); 
  o.printSw(kw_doUseInternalNodes, doUseInternalNodes); 
  o.printSw(kw_tree_beVerbose, beVerbose); 
  o.ppEnd(); 
}

/*--------------------------------------------------------*/
void AzRgfTree::printHelp(AzHelp &h) const
{
  h.begin(Aztree_config, "AzRgfTree", "Tree-wise control"); 
  h.item(kw_min_size, help_min_size, min_size_dflt); 
  h.item_experimental(kw_max_depth, help_max_depth, "-1: Don't care"); 
  h.item_experimental(kw_max_leaf_num, help_max_leaf_num, "-1: Don't care"); 
  h.item_experimental(kw_doUseInternalNodes, help_doUseInternalNodes); 
  h.item_experimental(kw_tree_beVerbose, help_tree_beVerbose); 
  h.end(); 
}
