
"""
__file__

    model_library_config.py

__description__

    This file provides model libray configurations for ensemble selection.

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import numpy as np
from hyperopt import hp


############
## Config ##
############

debug = False

## xgboost
xgb_random_seed = 2015
xgb_nthread = 2
xgb_dmatrix_silent = True

## sklearn
skl_random_seed = 2015
skl_n_jobs = 2

if debug:
    xgb_nthread = 1
    skl_n_jobs = 1
    xgb_min_num_round = 5
    xgb_max_num_round = 10
    xgb_num_round_step = 5
    skl_min_n_estimators = 5
    skl_max_n_estimators = 10
    skl_n_estimators_step = 5
    libfm_min_iter = 5
    libfm_max_iter = 10
    iter_step = 5
    hyperopt_param = {}
    hyperopt_param["xgb_max_evals"] = 1
    hyperopt_param["rf_max_evals"] = 1
    hyperopt_param["etr_max_evals"] = 1
    hyperopt_param["gbm_max_evals"] = 1
    hyperopt_param["lr_max_evals"] = 1
    hyperopt_param["ridge_max_evals"] = 1
    hyperopt_param["lasso_max_evals"] = 1
    hyperopt_param['svr_max_evals'] = 1
    hyperopt_param['dnn_max_evals'] = 1
    hyperopt_param['libfm_max_evals'] = 1
    hyperopt_param['rgf_max_evals'] = 1
else:
    xgb_min_num_round = 10
    xgb_max_num_round = 500
    xgb_num_round_step = 10
    skl_min_n_estimators = 10
    skl_max_n_estimators = 500
    skl_n_estimators_step = 10
    libfm_min_iter = 10
    libfm_max_iter = 500
    iter_step = 10
    hyperopt_param = {}
    hyperopt_param["xgb_max_evals"] = 200
    hyperopt_param["rf_max_evals"] = 200
    hyperopt_param["etr_max_evals"] = 200
    hyperopt_param["gbm_max_evals"] = 200
    hyperopt_param["lr_max_evals"] = 200
    hyperopt_param["ridge_max_evals"] = 200
    hyperopt_param["lasso_max_evals"] = 200
    hyperopt_param['svr_max_evals'] = 200
    hyperopt_param['dnn_max_evals'] = 200
    hyperopt_param['libfm_max_evals'] = 200
    hyperopt_param['rgf_max_evals'] = 200



########################################
## Parameter Space for XGBoost models ##
########################################
## In the early stage of the competition, I mostly focus on
## raw tfidf features and linear booster.

## regression with linear booster
param_space_reg_xgb_linear = {
    'task': 'regression',
    'booster': 'gblinear',
    'objective': 'reg:linear',
    'eta' : hp.quniform('eta', 0.01, 1, 0.01),
    'lambda' : hp.quniform('lambda', 0, 5, 0.05),
    'alpha' : hp.quniform('alpha', 0, 0.5, 0.005),
    'lambda_bias' : hp.quniform('lambda_bias', 0, 3, 0.1),
    'num_round' : hp.quniform('num_round', xgb_min_num_round, xgb_max_num_round, xgb_num_round_step),
    'nthread': xgb_nthread,
    'silent' : 1,
    'seed': xgb_random_seed,
    "max_evals": hyperopt_param["xgb_max_evals"],
}

## regression with tree booster
param_space_reg_xgb_tree = {
    'task': 'regression',
    'booster': 'gbtree',
    'objective': 'reg:linear',
    'eta': hp.quniform('eta', 0.01, 1, 0.01),
    'gamma': hp.quniform('gamma', 0, 2, 0.1),
    'min_child_weight': hp.quniform('min_child_weight', 0, 10, 1),
    'max_depth': hp.quniform('max_depth', 1, 10, 1),
    'subsample': hp.quniform('subsample', 0.5, 1, 0.1),
    'colsample_bytree': hp.quniform('colsample_bytree', 0.1, 1, 0.1),
    'num_round': hp.quniform('num_round', xgb_min_num_round, xgb_max_num_round, xgb_num_round_step),
    'nthread': xgb_nthread,
    'silent': 1,
    'seed': xgb_random_seed,
    "max_evals": hyperopt_param["xgb_max_evals"],
}

## softmax with linear booster
param_space_clf_xgb_linear = {
    'task': 'softmax',
    'booster': 'gblinear',
    'objective': 'multi:softprob',
    'eta' : hp.quniform('eta', 0.01, 1, 0.01),
    'lambda' : hp.quniform('lambda', 0, 5, 0.05),
    'alpha' : hp.quniform('alpha', 0, 0.5, 0.005),
    'lambda_bias' : hp.quniform('lambda_bias', 0, 3, 0.1),
    'num_round' : hp.quniform('num_round', xgb_min_num_round, xgb_max_num_round, xgb_num_round_step),
    'num_class': 4,
    'nthread': xgb_nthread,
    'silent' : 1,
    'seed': xgb_random_seed,
    "max_evals": hyperopt_param["xgb_max_evals"],
}

## pairwise ranking with linear booster
param_space_rank_xgb_linear = {
    'task': 'ranking',
    'booster': 'gblinear',
    'objective': 'rank:pairwise',
    'eta' : hp.quniform('eta', 0.01, 1, 0.01),
    'lambda' : hp.quniform('lambda', 0, 5, 0.05),
    'alpha' : hp.quniform('alpha', 0, 0.5, 0.005),
    'lambda_bias' : hp.quniform('lambda_bias', 0, 3, 0.1),
    'num_round' : hp.quniform('num_round', xgb_min_num_round, xgb_max_num_round, xgb_num_round_step),
    'nthread': xgb_nthread,
    'silent' : 1,
    'seed': xgb_random_seed,
    "max_evals": hyperopt_param["xgb_max_evals"],
}

## softkappa with linear booster
param_space_kappa_xgb_linear = {
    'task': 'softkappa',
    'booster': 'gblinear',
    'objective': 'reg:linear', # for linear raw predict score
    'hess_scale': hp.quniform('hess_scale', 0.000005, 0.0005, 0.000005),
    'eta' : hp.quniform('eta', 0.01, 1, 0.01),
    'lambda' : hp.quniform('lambda', 0, 0.001, 0.00001),
    'alpha' : hp.quniform('alpha', 0, 0.000005, 0.00000005),
    'lambda_bias' : hp.quniform('lambda_bias', 0, 3, 0.1),
    'num_round' : hp.quniform('num_round', xgb_min_num_round, xgb_max_num_round, xgb_num_round_step),
    'num_class': 4,
    'nthread': xgb_nthread,
    'silent' : 1,
    'seed': xgb_random_seed,
    "max_evals": hyperopt_param["xgb_max_evals"]
}

## extended binary classification (ebc) with linear booster
param_space_ebc_xgb_linear = {
    'task': 'ebc',
    'booster': 'gblinear',
    'objective': 'reg:linear',
    'eta' : hp.quniform('eta', 0.01, 1, 0.01),
    'lambda' : hp.quniform('lambda', 0, 5, 0.05),
    'alpha' : hp.quniform('alpha', 0, 0.5, 0.005),
    'lambda_bias' : hp.quniform('lambda_bias', 0, 3, 0.1),
    'num_round' : hp.quniform('num_round', xgb_min_num_round, xgb_max_num_round, xgb_num_round_step),
    'num_class': 4,
    'nthread': xgb_nthread,
    'silent' : 1,
    'seed': xgb_random_seed,
    "max_evals": hyperopt_param["xgb_max_evals"],
}

## cost-sensitive ordinal classification via regression (cocr) with linear booster
param_space_cocr_xgb_linear = {
    'task': 'cocr',
    'booster': 'gblinear',
    'objective': 'reg:linear',
    'eta' : hp.quniform('eta', 0.01, 1, 0.01),
    'lambda' : hp.quniform('lambda', 0, 5, 0.05),
    'alpha' : hp.quniform('alpha', 0, 0.5, 0.005),
    'lambda_bias' : hp.quniform('lambda_bias', 0, 3, 0.1),
    'num_round' : hp.quniform('num_round', xgb_min_num_round, xgb_max_num_round, xgb_num_round_step),
    'num_class': 3,
    'nthread': xgb_nthread,
    'silent' : 1,
    'seed': xgb_random_seed,
    "max_evals": hyperopt_param["xgb_max_evals"],
}


########################################
## Parameter Space for Sklearn Models ##
########################################

## random forest regressor
param_space_reg_skl_rf = {
    'task': 'reg_skl_rf',
    'n_estimators': hp.quniform("n_estimators", skl_min_n_estimators, skl_max_n_estimators, skl_n_estimators_step),
    'max_features': hp.quniform("max_features", 0.05, 1.0, 0.05),
    'n_jobs': skl_n_jobs,
    'random_state': skl_random_seed,
    "max_evals": hyperopt_param["rf_max_evals"],
}

## extra trees regressor
param_space_reg_skl_etr = {
    'task': 'reg_skl_etr',
    'n_estimators': hp.quniform("n_estimators", skl_min_n_estimators, skl_max_n_estimators, skl_n_estimators_step),
    'max_features': hp.quniform("max_features", 0.05, 1.0, 0.05),
    'n_jobs': skl_n_jobs,
    'random_state': skl_random_seed,
    "max_evals": hyperopt_param["etr_max_evals"],
}

## gradient boosting regressor
param_space_reg_skl_gbm = {
    'task': 'reg_skl_gbm',
    'n_estimators': hp.quniform("n_estimators", skl_min_n_estimators, skl_max_n_estimators, skl_n_estimators_step),
    'learning_rate': hp.quniform("learning_rate", 0.01, 0.5, 0.01),
    'max_features': hp.quniform("max_features", 0.05, 1.0, 0.05),
    'max_depth': hp.quniform('max_depth', 1, 15, 1),
    'subsample': hp.quniform('subsample', 0.5, 1, 0.1),
    'random_state': skl_random_seed,
    "max_evals": hyperopt_param["gbm_max_evals"],
}

## support vector regression
param_space_reg_skl_svr = {
    'task': 'reg_skl_svr',
    'C': hp.loguniform("C", np.log(1), np.log(100)),
    'gamma': hp.loguniform("gamma", np.log(0.001), np.log(0.1)),
    'degree': hp.quniform('degree', 1, 5, 1),
    'epsilon': hp.loguniform("epsilon", np.log(0.001), np.log(0.1)),    
    'kernel': hp.choice('kernel', ['rbf', 'poly']),
    "max_evals": hyperopt_param["svr_max_evals"],
}

## ridge regression
param_space_reg_skl_ridge = {
    'task': 'reg_skl_ridge',
    'alpha': hp.loguniform("alpha", np.log(0.01), np.log(20)),
    'random_state': skl_random_seed,
    "max_evals": hyperopt_param["ridge_max_evals"],
}

## lasso
param_space_reg_skl_lasso = {
    'task': 'reg_skl_lasso',
    'alpha': hp.loguniform("alpha", np.log(0.00001), np.log(0.1)),
    'random_state': skl_random_seed,
    "max_evals": hyperopt_param["lasso_max_evals"],
}

## logistic regression
param_space_clf_skl_lr = {
    'task': 'clf_skl_lr',
    'C': hp.loguniform("C", np.log(0.001), np.log(10)),
    'random_state': skl_random_seed,
    "max_evals": hyperopt_param["lr_max_evals"],
}


######################################################
## Parameter Space for Factorization Machine Models ##
######################################################

## regression with libfm
param_space_reg_libfm = {
    'task': 'reg_libfm',
    'dim': hp.quniform("dim", 1, 20, 1),
    "iter": hp.quniform("iter", libfm_min_iter, libfm_max_iter, iter_step),
    "max_evals": hyperopt_param["libfm_max_evals"],
}


######################################
## Parameter Space for Keras Models ##
######################################

## regression with Keras' deep neural network
param_space_reg_keras_dnn = {
    'task': 'reg_keras_dnn',
    'batch_norm': hp.choice("batch_norm", [True, False]),
    "hidden_units": hp.choice("hidden_units", [64, 128, 256, 512]),
    "hidden_layers": hp.choice("hidden_layers", [1, 2, 3, 4]),
    "input_dropout": hp.quniform("input_dropout", 0, 0.9, 0.1),
    "hidden_dropout": hp.quniform("hidden_dropout", 0, 0.9, 0.1),
    "hidden_activation": hp.choice("hidden_activation", ["relu", "prelu"]),
    "batch_size": hp.choice("batch_size", [16, 32, 64, 128]),
    "nb_epoch": hp.choice("nb_epoch", [10, 20, 30, 40]),
    "max_evals": hyperopt_param["dnn_max_evals"],
}


##########################################################
## Parameter Space for Regularized Greedy Forest Models ##
##########################################################

## regression with rgf
param_space_reg_rgf = {
    'task': 'reg_rgf',
    'reg_L2': hp.loguniform("reg_L2", np.log(0.1), np.log(10)),
    "max_leaf_forest": hp.quniform("max_leaf_forest", 100, 1000, 100),
    "num_iteration_opt": hp.quniform("num_iteration_opt", 5, 20, 1),
    "num_tree_search": hp.quniform("num_tree_search", 1, 5, 1),
    "min_pop": hp.quniform("min_pop", 1, 20, 1),
    "opt_interval": hp.quniform("opt_interval", 50, 200, 50),
    "opt_stepsize": hp.quniform("opt_stepsize", 0.1, 1.0, 0.1),
    "max_evals": hyperopt_param["rgf_max_evals"],
}


## integer features
int_feat = ["num_round", "n_estimators", "max_depth", "degree",
            "hidden_units", "hidden_layers", "batch_size", "nb_epoch",
            "dim", "iter",
            "max_leaf_forest", "num_iteration_opt", "num_tree_search", "min_pop", "opt_interval"]


####################
## All the Models ##
####################
feat_folders = []
feat_names = []
param_spaces = {}


#####################################
## [Feat@LSA_and_stats_feat_Jun09] ##
#####################################
#############
## xgboost ##
#############
## regression with xgboost tree booster
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_xgb_tree]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_tree

## regression with xgboost linear booster
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_linear

## cocr with xgboost linear booster
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@cocr_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_cocr_xgb_linear

## softkappa with xgboost linear booster
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@kappa_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_kappa_xgb_linear

#############
## Sklearn ##
#############
## extra trees regressor
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_etr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_etr

## random forest regressor
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_rf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_rf

## gradient boosting regressor
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_gbm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_gbm

## support vector regression
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_svr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_svr

## ridge regression
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_ridge]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_ridge

## lasso
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_lasso]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_lasso

## logistic regression
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@clf_skl_lr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_clf_skl_lr

###########################
## Factorization Machine ##
###########################
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_libfm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_libfm

###########
## Keras ##
###########
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_keras_dnn]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_keras_dnn

###############################
## Regularized Greedy Forest ##
###############################
feat_folder = "../../Feat/solution/LSA_and_stats_feat_Jun09"
feat_name = "[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_rgf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_rgf


#####################################
## [Feat@LSA_svd150_and_Jaccard_coef_Jun14] ##
#####################################
#############
## xgboost ##
#############
## regression with xgboost tree booster
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_xgb_tree]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_tree

## regression with xgboost linear booster
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_linear

## cocr with xgboost linear booster
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@cocr_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_cocr_xgb_linear

## softkappa with xgboost linear booster
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@kappa_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_kappa_xgb_linear

#############
## Sklearn ##
#############
## extra trees regressor
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_etr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_etr

## random forest regressor
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_rf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_rf

## gradient boosting regressor
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_gbm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_gbm

## support vector regression
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_svr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_svr

## ridge regression
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_ridge]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_ridge

## lasso
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_lasso]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_lasso

## logistic regression
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@clf_skl_lr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_clf_skl_lr

###########################
## Factorization Machine ##
###########################
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_libfm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_libfm

###########
## Keras ##
###########
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_keras_dnn]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_keras_dnn

###############################
## Regularized Greedy Forest ##
###############################
feat_folder = "../../Feat/solution/LSA_svd150_and_Jaccard_coef_Jun14"
feat_name = "[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_rgf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_rgf


#####################################
## [Feat@svd100_and_bow_Jun23] ##
#####################################
#############
## xgboost ##
#############
## regression with xgboost tree booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_xgb_tree]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_tree

## regression with xgboost linear booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_linear

## cocr with xgboost linear booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@cocr_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_cocr_xgb_linear

## softkappa with xgboost linear booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@kappa_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_kappa_xgb_linear

#############
## Sklearn ##
#############
## extra trees regressor
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_etr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_etr

## random forest regressor
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_rf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_rf

## gradient boosting regressor
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_gbm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_gbm

## support vector regression
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_svr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_svr

## ridge regression
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_ridge]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_ridge

## lasso
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_lasso]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_lasso

## logistic regression
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@clf_skl_lr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_clf_skl_lr

###########################
## Factorization Machine ##
###########################
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_libfm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_libfm

###########
## Keras ##
###########
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_keras_dnn]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_keras_dnn

###############################
## Regularized Greedy Forest ##
###############################
feat_folder = "../../Feat/solution/svd100_and_bow_Jun23"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_rgf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_rgf


#################################
## [Feat@svd100_and_bow_Jun27] ##
#################################
#############
## xgboost ##
#############
## regression with xgboost tree booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_tree]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_tree

## regression with xgboost linear booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_xgb_linear

## cocr with xgboost linear booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@cocr_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_cocr_xgb_linear

## softkappa with xgboost linear booster
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@kappa_xgb_linear]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_kappa_xgb_linear

#############
## Sklearn ##
#############
## extra trees regressor
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_etr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_etr

## random forest regressor
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_rf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_rf

## gradient boosting regressor
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_gbm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_gbm

## support vector regression
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_svr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_svr

## ridge regression
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_ridge]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_ridge

## lasso
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_lasso]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_skl_lasso

## logistic regression
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@clf_skl_lr]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_clf_skl_lr

###########################
## Factorization Machine ##
###########################
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_libfm]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_libfm

###########
## Keras ##
###########
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_keras_dnn]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_keras_dnn

###############################
## Regularized Greedy Forest ##
###############################
feat_folder = "../../Feat/solution/svd100_and_bow_Jun27"
feat_name = "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_rgf]"
feat_folders.append( feat_folder )
feat_names.append( feat_name )
param_spaces[feat_name] = param_space_reg_rgf