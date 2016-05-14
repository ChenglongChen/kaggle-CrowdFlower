/* * * * *
 *  AzTree.cpp 
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

#include "AzTree.hpp"
#include "AzPrint.hpp"

/*--------------------------------------------------------*/
void AzTree::write(AzFile *file)
{
  file->writeInt(root_nx); 
  file->writeInt(nodes_used); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    nodes[nx].write(file); 
  }
}

/*--------------------------------------------------------*/
void AzTree::read(AzFile *file)
{
  _release(); 
  _read(file); 
}

/*--------------------------------------------------------*/
void AzTree::copy_from(const AzTreeNodes *tree_nodes)
{
  reset(); 
  root_nx = tree_nodes->root(); 
  nodes_used = tree_nodes->nodeNum(); 
  a_nodes.alloc(&nodes, nodes_used, "AzTree::copy_from", "nodes"); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    nodes[nx] = *tree_nodes->node(nx); 
  }               
}

/*--------------------------------------------------------*/
void AzTree::_read(AzFile *file)
{
  const char *eyec = "AzTree::read"; 

  root_nx = file->readInt(); 
  nodes_used = file->readInt(); 
  a_nodes.alloc(&nodes, nodes_used, eyec, "nodes"); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    nodes[nx].read(file); 
  }
}

/*--------------------------------------------------------*/
void AzTreeNode::write(AzFile *file) 
{
  file->writeDouble(border_val); 
  file->writeDouble(weight); 
  file->writeInt(fx); 
  file->writeInt(le_nx); 
  file->writeInt(gt_nx); 
  file->writeInt(parent_nx); 
}

/*--------------------------------------------------------*/
void AzTreeNode::read(AzFile *file) 
{
  border_val = file->readDouble(); 
  weight = file->readDouble(); 
  fx = file->readInt(); 
  le_nx = file->readInt(); 
  gt_nx = file->readInt(); 
  parent_nx = file->readInt(); 
}

/*--------------------------------------------------------*/
void AzTree::_release()
{
  a_nodes.free(&nodes); nodes_used = 0; 
  root_nx = AzNone; 
}

/*--------------------------------------------------------*/
/* static */
double AzTree::apply(const AzReadOnlyVector *v_data, 
                     const AzTreeNodes *tree_nodes, 
                     AzIntArr *ia_node) /* may be NULL */
{
  const char *eyec = "AzTree::apply(v)"; 
  int nx = tree_nodes->root(); 
  double p_val = 0; 
  for ( ; ; ) {
    if (nx < 0) {
      throw new AzException(eyec, "stuck"); 
    }
    const AzTreeNode *np = tree_nodes->node(nx); 
    p_val += np->weight; 
    if (ia_node != NULL && np->weight != 0) {
      ia_node->put(nx); 
    }
    if (np->isLeaf()) { /* leaf */
      break; 
    }
    if (v_data->get(np->fx) <= np->border_val) {
      nx = np->le_nx;         
    }
    else {
      nx = np->gt_nx; 
    }
  }
  return p_val; 
} 

/*--------------------------------------------------------*/
void AzTree::show(const AzSvFeatInfo *feat, const AzOut &out, 
                  const char *header) const
{
  if (out.isNull()) return; 
  AzPrint::writeln(out, header); 
  if (nodes == NULL) {
	  AzPrint::writeln(out, "AzTree::show, No tree\n"); 
    return; 
  }
  _show(feat, root_nx, 0, out); 
}

/*--------------------------------------------------------*/
void AzTree::_show(const AzSvFeatInfo *feat, 
                    int nx, 
                    int depth, 
                    const AzOut &out) const
{
  if (out.isNull()) return; 

  const AzTreeNode *np = &nodes[nx]; 

  AzPrint o(out); 
  o.printBegin("", ", ", "=", depth*2); 
  /* [nx], (weight), depth=d, desc,border */
  o.inBrackets(nx,3); 
  if (np->weight != 0) {
    o.inParen(np->weight,3); 
  }
  o.printV("depth=", depth); 
  if (np->fx >= 0) {
    AzBytArr s_desc; 
    if (feat != NULL) {
      feat->concatDesc(np->fx, &s_desc); 
    }
    else {
      s_desc.c("F"); 
      s_desc.cn(np->fx); 
    }
    o.print(&s_desc); 
    o.print(np->border_val,4,false); 
  }
  o.printEnd(); 

  if (!np->isLeaf()) {
    _show(feat, np->le_nx, depth+1, out); 
    _show(feat, np->gt_nx, depth+1, out); 
  }
}

/*--------------------------------------------------------*/
int AzTree::leafNum() const
{
  checkNodes("leafNum"); 
  int leaf_num = 0; 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].isLeaf()) {
      ++leaf_num; 
    }
  }
  return leaf_num; 
}

/*--------------------------------------------------------*/
void AzTree::clean_up()
{
  checkNodes("clean_up"); 

  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (!nodes[nx].isLeaf()) continue; 

    /*---  add non-leaf weights to the leaf weight ---*/
    int px = nodes[nx].parent_nx; 
    for ( ; ; ) {
      if (px < 0) break; 
      nodes[nx].weight += nodes[px].weight; 
      px = nodes[px].parent_nx; 
    }
  }

  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].isLeaf()) continue; 
    nodes[nx].weight = 0; /* zero-out non-leaf weights */
  }
}

/*--------------------------------------------------------*/
void AzTree::finfo(AzIFarr *ifa_fx_count, 
                   AzIFarr *ifa_fx_w) /* appended */
const 
{
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (!nodes[nx].isLeaf()) continue;
    double w = nodes[nx].weight;  
    int nx1 = nodes[nx].parent_nx; 
    while(nx1 >= 0) {
      ifa_fx_count->put(nodes[nx1].fx, 1); 
      ifa_fx_w->put(nodes[nx1].fx, fabs(w)); 
      nx1 = nodes[nx1].parent_nx; 
    }
  }
}

/*--------------------------------------------------------*/
void AzTree::finfo(AzIntArr *ia_fxs) const /* appended */
{
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].fx >= 0) {
      ia_fxs->put(nodes[nx].fx); 
    }
  }
}

/* pairs of features used in the same path (from the root to the leaf */
/*--------------------------------------------------------*/
void AzTree::cooccurrences(AzIIFarr *iifa_fx1_fx2_count) const
{
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (!nodes[nx].isLeaf()) continue; 

    AzIntArr ia_fxs; 
    int nx1 = nodes[nx].parent_nx; 
    while(nx1>=0) {
      if (nodes[nx1].fx >= 0) ia_fxs.put(nodes[nx1].fx); 
      nx1 = nodes[nx1].parent_nx; 
    }
    ia_fxs.sort(true); 
    const int *fxs = ia_fxs.point(); 
    int ix1, ix2; 
    for (ix1 = 0; ix1 < ia_fxs.size(); ++ix1) {
      for (ix2 = ix1+1; ix2 < ia_fxs.size(); ++ix2) {
        iifa_fx1_fx2_count->put(fxs[ix1], fxs[ix2], 1); 
      }
    }
  }
  iifa_fx1_fx2_count->squeeze_Sum(); 
}

/*--------------------------------------------------------*/
void AzTree::genDesc(const AzSvFeatInfo *feat, 
                  int nx, 
                  AzBytArr *s) /* output */
const
{
  s->reset(); 
  if (feat == NULL) {
    s->c("not_available"); 
  }
  _checkNode(nx, "AzTree::concatDesc"); 
  if (nx == root_nx) {
    s->c("ROOT"); 
  }
  else {
    _genDesc(feat, nx, s); 
  }
}

/*--------------------------------------------------------*/
void AzTree::_genDesc(const AzSvFeatInfo *feat, 
                      int nx, 
                      AzBytArr *s) /* output */
const
{
  int px = nodes[nx].parent_nx; 
  if (px < 0) return; 

  _genDesc(feat, px, s); 
  if (s->getLen() > 0) {
    s->c(";"); 
  }
  feat->concatDesc(nodes[px].fx, s); 
  if (nodes[px].le_nx == nx) {
    s->c("<="); 
  }
  else {
    s->c(">"); 
  }
  s->cn(nodes[px].border_val, 5); 
}

