
"""
__file__

    genFeat_id_feat.py

__description__

    This file generates the following features for each run and fold, and for the entire training and testing set.

        1. one-hot encoding of query ids (qid)

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import sys
import cPickle
from sklearn.preprocessing import LabelBinarizer
sys.path.append("../")
from param_config import config


if __name__ == "__main__":

    ## config
    id_names = [ "qid" ]

    ###############
    ## Load Data ##
    ###############
    ## load data
    with open(config.processed_train_data_path, "rb") as f:
        dfTrain = cPickle.load(f)
    with open(config.processed_test_data_path, "rb") as f:
        dfTest = cPickle.load(f)
    ## load pre-defined stratified k-fold index
    with open("%s/stratifiedKFold.%s.pkl" % (config.data_folder, config.stratified_label), "rb") as f:
            skf = cPickle.load(f)

    #######################
    ## Generate Features ##
    #######################
    print("==================================================")
    print("Generate id features...")

    print("For cross-validation...")
    for run in range(config.n_runs):
        ## use 33% for training and 67 % for validation
        ## so we switch trainInd and validInd
        for fold, (validInd, trainInd) in enumerate(skf[run]):
            print("Run: %d, Fold: %d" % (run+1, fold+1))
            path = "%s/Run%d/Fold%d" % (config.feat_folder, run+1, fold+1)

            #################
            ## get id feat ##
            #################
            for id_name in id_names:
                lb = LabelBinarizer(sparse_output=True)
                X_train = lb.fit_transform(dfTrain.iloc[trainInd][id_name])
                X_valid = lb.transform(dfTrain.iloc[validInd][id_name])
                with open("%s/train.%s.feat.pkl" % (path, id_name), "wb") as f:
                    cPickle.dump(X_train, f, -1)
                with open("%s/valid.%s.feat.pkl" % (path, id_name), "wb") as f:
                    cPickle.dump(X_valid, f, -1)
                    
    print("Done.")

    print("For training and testing...")
    path = "%s/All" % config.feat_folder
    ## use full version for X_train                
    for id_name in id_names:
        X_train = lb.fit_transform(dfTrain[id_name])
        X_test = lb.transform(dfTest[id_name])
        with open("%s/train.%s.feat.pkl" % (path, id_name), "wb") as f:
            cPickle.dump(X_train, f, -1)
        with open("%s/test.%s.feat.pkl" % (path, id_name), "wb") as f:
            cPickle.dump(X_test, f, -1)
    print("Done.")
    
    print("All Done.")