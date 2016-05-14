
"""
__file__
	
	compare_MSE_decoding.py

__description__

	This file compares various decoding method for MSE, i.e., rounding, ceiling, and cdf decoding methods.

__author__

	Chenglong Chen < c.chenglong@gmail.com >

"""

import sys
sys.path.append("../Code/Model/")
from ml_metrics import quadratic_weighted_kappa
import numpy as np
import pandas as pd
import scipy as sp
from matplotlib import pyplot as plt
from matplotlib.figure import figaspect

#####################
## Decoding Method ##
#####################
#### decoding method for ranking and regression
def getScore(pred, cdf, valid=False):
    num = pred.shape[0]
    output = np.asarray([4]*num, dtype=int)
    rank = pred.argsort()
    output[rank[:int(num*cdf[0]-1)]] = 1
    output[rank[int(num*cdf[0]):int(num*cdf[1]-1)]] = 2
    output[rank[int(num*cdf[1]):int(num*cdf[2]-1)]] = 3
    if valid:
        cutoff = [ pred[rank[int(num*cdf[i]-1)]] for i in range(3) ]
        return output, cutoff
    return output


n_runs = 3
n_folds = 3
## use (reproduced) best single model
best = "valid.raw.pred.[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_linear]_[Id@24].csv"


###################
## Compute Kappa ##
###################
kappa_cdf_cv = np.zeros((n_runs, n_folds))
kappa_rounding_cv = np.zeros((n_runs, n_folds))
kappa_ceiling_cv = np.zeros((n_runs, n_folds))
for run in range(n_runs):
	for fold in range(n_folds):
		pred_file = "../Output/Best_Single_Model/Run%s/Fold%s/%s" % (run+1, fold+1, best)
		cdf_file = "../Output/Best_Single_Model/Run%s/Fold%s/valid.cdf" % (run+1, fold+1)

		df = pd.read_csv(pred_file)
		Y_valid = df["target"].values
		Y_pred_raw = df["prediction"].values

		cdf_valid = np.loadtxt(cdf_file, dtype=float)

		Y_pred_cdf = getScore(Y_pred_raw, cdf_valid)
		Y_pred_rounding = np.round(Y_pred_raw).astype(int)
		Y_pred_ceiling = np.ceil(Y_pred_raw).astype(int)

		Y_pred_rounding = sp.maximum(Y_pred_rounding, 1)
		Y_pred_ceiling = sp.maximum(Y_pred_ceiling, 1)

		kappa_cdf_cv[run, fold] = quadratic_weighted_kappa(Y_valid, Y_pred_cdf)
		kappa_rounding_cv[run, fold] = quadratic_weighted_kappa(Y_valid, Y_pred_rounding)
		kappa_ceiling_cv[run, fold] = quadratic_weighted_kappa(Y_valid, Y_pred_ceiling)


print("Kappa using cdf decoding method: %.6f (%.6f)" % (np.mean(kappa_cdf_cv), np.std(kappa_cdf_cv)))
print("Kappa using rounding decoding method: %.6f (%.6f)" % (np.mean(kappa_rounding_cv), np.std(kappa_rounding_cv)))
print("Kappa using ceiling decoding method: %.6f (%.6f)" % (np.mean(kappa_ceiling_cv), np.std(kappa_ceiling_cv)))


###################
## Visualization ##
###################
w, h = figaspect(1)
plt.figure(figsize=(w,h))
f, axarr = plt.subplots(4,4, sharex=True)
## raw prediction
axarr[0,0].hist(Y_pred_raw[Y_valid==1])
axarr[0,0].set_title("Relevance = 1")
axarr[0,0].set_ylabel("Raw")
axarr[0,1].hist(Y_pred_raw[Y_valid==2])
axarr[0,1].set_title("Relevance = 2")
axarr[0,2].hist(Y_pred_raw[Y_valid==3])
axarr[0,2].set_title("Relevance = 3")
axarr[0,3].hist(Y_pred_raw[Y_valid==4])
axarr[0,3].set_title("Relevance = 4")
## rounding decoding
axarr[1,0].hist(Y_pred_rounding[Y_valid==1])
axarr[1,0].set_ylabel("Rounding")
axarr[1,1].hist(Y_pred_rounding[Y_valid==2])
axarr[1,2].hist(Y_pred_rounding[Y_valid==3])
axarr[1,3].hist(Y_pred_rounding[Y_valid==4])
## ceiling decoding
axarr[2,0].hist(Y_pred_ceiling[Y_valid==1])
axarr[2,0].set_ylabel("Ceiling")
axarr[2,1].hist(Y_pred_ceiling[Y_valid==2])
axarr[2,2].hist(Y_pred_ceiling[Y_valid==3])
axarr[2,3].hist(Y_pred_ceiling[Y_valid==4])
## cdf decoding
axarr[3,0].hist(Y_pred_cdf[Y_valid==1])
axarr[3,0].set_ylabel("CDF")
axarr[3,1].hist(Y_pred_cdf[Y_valid==2])
axarr[3,2].hist(Y_pred_cdf[Y_valid==3])
axarr[3,3].hist(Y_pred_cdf[Y_valid==4])
plt.show()