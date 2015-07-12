/* * * * *
 *  AzTreeEnsemble.cpp 
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

#include "AzTreeEnsemble.hpp"
#include "AzPrint.hpp"

static int reserved_length = 256; 

/*--------------------------------------------------------*/
void AzTreeEnsemble::info(AzTE_ModelInfo *out_info) const
{
  if (out_info == NULL) return; 

  out_info->leaf_num = leafNum(); 
  out_info->tree_num = size(); 
  out_info->s_sign.reset(&s_sign); 
  out_info->s_config.reset(&s_config); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::transfer_from(AzTree *inp_tree[], 
                                   int inp_tree_num, 
                                   double inp_const_val, 
                                   int inp_org_dim, 
                                   const char *config, 
                                   const char *sign)
{
  a_tree.free(&t); t_num = 0;  
  a_tree.alloc(&t, inp_tree_num, "AzTreeEnsemble::reset");                           
  t_num = inp_tree_num; 
  const_val = inp_const_val; 
  org_dim = inp_org_dim; 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    if (inp_tree[tx] != NULL) {
      t[tx] = inp_tree[tx]; 
      inp_tree[tx] = NULL; 
    }
  }
  s_config.reset(config); 
  s_sign.reset(sign); 

  clean_up(); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::write(AzFile *file)
{
  file->writeBinMarker(); 
  int ix; 
  for (ix = 0; ix < reserved_length; ++ix) file->writeByte(0); 
  file->writeInt(t_num); 
  file->writeDouble(const_val); 
  file->writeInt(org_dim); 
  s_config.write(file); 
  s_sign.write(file); 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    AzObjIOTools::write(t[tx], file); 
  }
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::_read(AzFile *file)
{
  file->checkBinMarker(); 
  int ix; 
  for (ix = 0; ix < reserved_length; ++ix) {
    AzByte byte = file->readByte();  
    if (byte != 0) {
      throw new AzException(AzInputNotValid, "AzTreeEnsemble::_read", 
            file->pointFileName(), 
            "Error detected in the reserved field.  Broken file or version conflict"); 
    }
  }
  t_num = file->readInt(); 
  const_val = file->readDouble(); 
  org_dim = file->readInt(); 
  s_config.read(file); 
  s_sign.read(file); 
  a_tree.alloc(&t, t_num, "AzTreeEnsemble::read");                           
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    t[tx] = AzObjIOTools::read<AzTree>(file); 
  }
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::apply(const AzSmat *m_data, 
                           AzDvect *v_pred) const
{
  int data_num = m_data->colNum(); 
  v_pred->reform(data_num); 
  double *pred = v_pred->point_u(); 
  int dx;  
  for (dx = 0; dx < data_num; ++dx) {
    pred[dx] = apply(m_data->col(dx)); 
  }
}

/*--------------------------------------------------------*/
double AzTreeEnsemble::apply(const AzSvect *v_data) const
{
  AzDvect v(v_data);  /* for efficiency of access */

  double val = const_val; 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    if (t[tx] != NULL) {
      val += t[tx]->apply(&v); 
    }
  }
  return val; 
}

/*--------------------------------------------------------*/
int AzTreeEnsemble::leafNum(int tx0, int tx1) const
{
  int l_num = 0; 
  int tx; 
  for (tx = MAX(0, tx0); tx < MIN(t_num, tx1); ++tx) {
    if (t[tx] != NULL) {
      l_num += t[tx]->leafNum(); 
    }
  }
  return l_num; 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::read(const char *fn) 
{
  AzFile file(fn); 
  file.open("rb"); 
  read(&file); 
  file.close(); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::write(const char *fn) 
{
  AzFile file(fn); 
  file.open("wb"); 
  write(&file); 
  file.close(true); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::clean_up()
{
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    if (t[tx] == NULL) continue; 
    t[tx]->clean_up(); 
  }
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::show(const AzSvFeatInfo *feat, //!< may be NULL 
            const AzOut &out, const char *header) const 
{
  if (out.isNull()) return; 
  AzPrint o(out); 
  o.printBegin(header, ", ", "="); 
  o.print("constant", const_val); 
  o.print("orgdim", org_dim); 
  o.print("#tree", t_num); 
  o.printEnd(); 

  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    AzBytArr s("tree"); s.inBrackets(tx); 
    AzPrint::writeln(out, s); 
    if (t[tx] != NULL) {
      t[tx]->show(feat, out); 
    }
  }
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::finfo(int tx0, int tx1, 
             AzIFarr *ifa_fx_count, 
             AzIFarr *ifa_fx_sum) const 
{
  if (tx0 < 0 || tx0 > t_num || 
      tx1 < 0 || tx1 > t_num) {
    throw new AzException("AzTreeEnsemble::finfo", "out of range"); 
  }
  ifa_fx_count->reset(); 
  ifa_fx_sum->reset(); 
  int tx; 
  for (tx = tx0; tx < tx1; ++tx) {
    if (t[tx] != NULL) {
      t[tx]->finfo(ifa_fx_count, ifa_fx_sum); 
    }
  }
  ifa_fx_count->squeeze_Sum(); 
  ifa_fx_sum->squeeze_Sum(); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::finfo(AzIntArr *ia_fx2tx) /* first appearance */
const 
{
  const char *eyec = "AzTreeEnsemble::finfo"; 
  ia_fx2tx->reset(org_dim, -1); 
  int *fx2tx = ia_fx2tx->point_u(); 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    AzIntArr ia_fxs;     
    t[tx]->finfo(&ia_fxs); 
    int ix; 
    for (ix = 0; ix < ia_fxs.size(); ++ix) {
      int fx = ia_fxs.get(ix); 
      if (fx < 0 || fx >= org_dim) {
        throw new AzException(eyec, "fx is out of range"); 
      }
      if (fx2tx[fx] < 0) {
        fx2tx[fx] = tx; 
      }
    }
  }
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::cooccurrences(AzIIFarr *iifa_fx1_fx2_count) 
const 
{
  iifa_fx1_fx2_count->reset(); 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    AzIIFarr ifa; 
    t[tx]->cooccurrences(&ifa); 
    iifa_fx1_fx2_count->concat(&ifa); 
  }
  iifa_fx1_fx2_count->squeeze_Sum(); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::show_weights(const AzOut &out, AzSvFeatInfo *fi) const
{
  AzIIFarr iifa_tx_nx_posiw, iifa_tx_nx_negaw; 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    int nx; 
    for (nx = 0; nx < t[tx]->nodeNum(); ++nx) {
      const AzTreeNode *np = t[tx]->node(nx); 
      if (np->weight > 0) {
        iifa_tx_nx_posiw.put(tx, nx, np->weight); 
      }
      else if (np->weight < 0) {
        iifa_tx_nx_negaw.put(tx, nx, np->weight); 
      }
    }
  }
  iifa_tx_nx_posiw.sort_Float(false); /* descending order */
  iifa_tx_nx_negaw.sort_Float(true); /* ascending order */

  AzPrint::writeln(out, "Positive weights -------------------"); 
  int ix; 
  for (ix = 0; ix < iifa_tx_nx_posiw.size(); ++ix) {
    int tx, nx; 
    double w = iifa_tx_nx_posiw.get(ix, &tx, &nx); 
    AzBytArr s_desc; 
    t[tx]->genDesc(fi, nx, &s_desc); 
    AzBytArr s; s.cn(w, 6, false); s.c(' '); s.c(&s_desc); 
    AzPrint::writeln(out, s); 
  }

  AzPrint::writeln(out, "Negative weights -------------------"); 
  for (ix = 0; ix < iifa_tx_nx_negaw.size(); ++ix) {
    int tx, nx; 
    double w = iifa_tx_nx_negaw.get(ix, &tx, &nx); 
    AzBytArr s_desc; 
    t[tx]->genDesc(fi, nx, &s_desc); 
    AzBytArr s; s.cn(w, 6, false); s.c(' '); s.c(&s_desc); 
    AzPrint::writeln(out, s); 
  }
}
