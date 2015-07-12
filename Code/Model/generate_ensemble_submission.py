
"""
__file__

    generate_ensemble_submission.py

__description__

    This file generates submission via ensemble selection.

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import os
import sys
import numpy as np
import pandas as pd
from utils import *
from ensemble_selection import *
from model_library_config import feat_folders, feat_names


##
## config
model_folder = "../../Output"
subm_folder = "../../Output/Subm"
if not os.path.exists(subm_folder):
    os.makedirs(subm_folder)


## load test info
feat_folder = feat_folders[0]
info_test = pd.read_csv("%s/All/test.info" % feat_folder)
id_test = info_test["id"]
numTest = info_test.shape[0]
## load cdf
cdf_test = np.loadtxt("%s/All/test.cdf" % feat_folder, dtype=float)  

#pdf_test = [0.089, 0.112, 0.19, 0.609]
#cdf_test = np.cumsum(pdf_test)
#cdf_valid = cdf_test
cdf_valid = None


## reg
model_list = []
# try 5/10/50
id_sizes = 10*np.ones(len(feat_names), dtype=int)
for feat_name,id_size in zip(feat_names, id_sizes):
    ## get the top 10 model ids
    log_file = "%s/Log/%s_hyperopt.log" % (model_folder, feat_name)
    try:
        dfLog = pd.read_csv(log_file)
        dfLog.sort("kappa_mean", ascending=False, inplace=True)
        ind = np.min([id_size, dfLog.shape[0]])
        ids = dfLog.iloc[:ind]["trial_counter"]
        #print dfLog[:ind]
        model_list += ["%s_[Id@%d]" % (feat_name, id) for id in ids]
    except:
        pass

  

bagging_size = 100
bagging_fraction = 1.0
prunning_fraction = 1.
bagging_replacement = True
init_top_k = 5

subm_prefix = "%s/test.pred.[ensemble_selection]_[Solution]" % (subm_folder)
best_kappa_mean, best_kappa_std, best_bagged_model_list, best_bagged_model_weight = \
    ensembleSelection(feat_folder, model_folder, model_list, cdf=cdf_valid, cdf_test=cdf_test, subm_prefix=subm_prefix, \
        hypteropt_max_evals=1, w_min=-1, w_max=1, bagging_replacement=bagging_replacement, bagging_fraction=bagging_fraction, \
        bagging_size=bagging_size, init_top_k=init_top_k, prunning_fraction=prunning_fraction)