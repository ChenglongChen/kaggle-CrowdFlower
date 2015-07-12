
"""
__file__

    gen_info.py

__description__

    This file generates the following info for each run and fold, and for the entire training and testing set.

        1. training and validation/testing data

        2. sample weight

        3. cdf of the median_relevance
        
        4. the group info for pairwise ranking in XGBoost

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import os
import sys
import cPickle
import numpy as np
import pandas as pd
sys.path.append("../")
from param_config import config

def gen_info(feat_path_name):
    ###############
    ## Load Data ##
    ###############
    ## load data
    with open(config.processed_train_data_path, "rb") as f:
        dfTrain = cPickle.load(f)
    with open(config.processed_test_data_path, "rb") as f:
        dfTest = cPickle.load(f)
    dfTrain_original = pd.read_csv(config.original_train_data_path).fillna("")
    dfTest_original = pd.read_csv(config.original_test_data_path).fillna("")
    ## insert fake label for test
    dfTest_original["median_relevance"] = np.ones((dfTest_original.shape[0]))
    dfTest_original["relevance_variance"] = np.zeros((dfTest_original.shape[0]))
    # change it to zero-based for classification
    Y = dfTrain_original["median_relevance"].values - 1

    ## load pre-defined stratified k-fold index
    with open("%s/stratifiedKFold.%s.pkl" % (config.data_folder, config.stratified_label), "rb") as f:
        skf = cPickle.load(f)
        
    #######################
    ## Generate Features ##
    #######################
    print("Generate info...")
    print("For cross-validation...")
    for run in range(config.n_runs):
        ## use 33% for training and 67 % for validation
        ## so we switch trainInd and validInd
        for fold, (validInd, trainInd) in enumerate(skf[run]):
            print("Run: %d, Fold: %d" % (run+1, fold+1))
            path = "%s/%s/Run%d/Fold%d" % (config.feat_folder, feat_path_name, run+1, fold+1)
            if not os.path.exists(path):
                os.makedirs(path)
            ##########################
            ## get and dump weights ##
            ##########################
            raise_to = 0.5
            var = dfTrain["relevance_variance"].values
            max_var = np.max(var[trainInd]**raise_to)
            weight = (1 + np.power(((max_var - var**raise_to) / max_var),1)) / 2.
            #weight = (max_var - var**raise_to) / max_var
            np.savetxt("%s/train.feat.weight" % path, weight[trainInd], fmt="%.6f")
            np.savetxt("%s/valid.feat.weight" % path, weight[validInd], fmt="%.6f")

            #############################    
            ## get and dump group info ##
            #############################
            np.savetxt("%s/train.feat.group" % path, [len(trainInd)], fmt="%d")
            np.savetxt("%s/valid.feat.group" % path, [len(validInd)], fmt="%d")
            
            ######################
            ## get and dump cdf ##
            ######################
            hist = np.bincount(Y[trainInd])
            overall_cdf_valid = np.cumsum(hist) / float(sum(hist))
            np.savetxt("%s/valid.cdf" % path, overall_cdf_valid)
                
            #############################
            ## dump all the other info ##
            #############################
            dfTrain_original.iloc[trainInd].to_csv("%s/train.info" % path, index=False, header=True)
            dfTrain_original.iloc[validInd].to_csv("%s/valid.info" % path, index=False, header=True)
    print("Done.")

    print("For training and testing...")
    path = "%s/%s/All" % (config.feat_folder, feat_path_name)
    if not os.path.exists(path):
        os.makedirs(path)
    ## weight
    max_var = np.max(var**raise_to)
    weight = (1 + np.power(((max_var - var**raise_to) / max_var),1)) / 2.
    np.savetxt("%s/train.feat.weight" % path, weight, fmt="%.6f")
    
    ## group
    np.savetxt("%s/%s/All/train.feat.group" % (config.feat_folder, feat_path_name), [dfTrain.shape[0]], fmt="%d")
    np.savetxt("%s/%s/All/test.feat.group" % (config.feat_folder, feat_path_name), [dfTest.shape[0]], fmt="%d")
    ## cdf
    hist_full = np.bincount(Y)
    print (hist_full) / float(sum(hist_full))
    overall_cdf_full = np.cumsum(hist_full) / float(sum(hist_full))
    np.savetxt("%s/%s/All/test.cdf" % (config.feat_folder, feat_path_name), overall_cdf_full)
    ## info        
    dfTrain_original.to_csv("%s/%s/All/train.info" % (config.feat_folder, feat_path_name), index=False, header=True)
    dfTest_original.to_csv("%s/%s/All/test.info" % (config.feat_folder, feat_path_name), index=False, header=True)
    
    print("All Done.")