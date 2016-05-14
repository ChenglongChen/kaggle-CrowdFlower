
"""
__file__

    ensemble_selection.py

__description__

    This file contains ensemble selection module.

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""


import csv
import sys
import numpy as np
import pandas as pd
from utils import getScore, getTestScore
from ml_metrics import quadratic_weighted_kappa
from hyperopt import fmin, tpe, hp, STATUS_OK, STATUS_FAIL, Trials
sys.path.append("../")
from param_config import config

########################
## Ensemble Selection ##
########################
def ensembleSelectionPrediction(model_folder, best_bagged_model_list, best_bagged_model_weight, cdf, cutoff=None):
    bagging_size = len(best_bagged_model_list)
    for bagging_iter in range(bagging_size):
        w_ens = 0
        iter = 0
        for model,w in zip(best_bagged_model_list[bagging_iter], best_bagged_model_weight[bagging_iter]):
            iter += 1
            pred_file = "%s/All/test.pred.%s.csv" % (model_folder, model)
            this_p_valid = pd.read_csv(pred_file, dtype=float)["prediction"].values
            this_w = w
            if iter == 1:
                p_ens_valid = np.zeros((this_p_valid.shape[0]),dtype=float)
                id_test = pd.read_csv(pred_file, dtype=float)["id"].values
                id_test = np.asarray(id_test, dtype=int)
            p_ens_valid = (w_ens * p_ens_valid + this_w * this_p_valid) / (w_ens + this_w)
            w_ens += this_w
        if bagging_iter == 0:
            p_ens_valid_bag = p_ens_valid
        else:
            p_ens_valid_bag = (bagging_iter * p_ens_valid_bag + p_ens_valid) / (bagging_iter+1.)
    if cutoff is None:
        p_ens_score = getScore(p_ens_valid_bag, cdf)
    else:
        p_ens_score = getTestScore(p_ens_valid_bag, cutoff)
    ##
    output = pd.DataFrame({"id": id_test, "prediction": p_ens_score})
    return output


def ensembleSelectionObj(param, p1_list, weight1, p2_list, true_label_list, cdf_list, numValidMatrix):

    weight2 = param['weight2']
    kappa_cv = np.zeros((config.n_runs, config.n_folds), dtype=float)
    for run in range(config.n_runs):
        for fold in range(config.n_folds):
            numValid = numValidMatrix[run][fold]
            p1 = p1_list[run,fold,:numValid]
            p2 = p2_list[run,fold,:numValid]
            true_label = true_label_list[run,fold,:numValid]
            cdf = cdf_list[run,fold,:]
            p_ens = (weight1 * p1 + weight2 * p2) / (weight1 + weight2)
            p_ens_score = getScore(p_ens, cdf)
            kappa_cv[run][fold] = quadratic_weighted_kappa(p_ens_score, true_label)
    kappa_cv_mean = np.mean(kappa_cv)
    return {'loss': -kappa_cv_mean, 'status': STATUS_OK}


def ensembleSelection(feat_folder, model_folder, model_list, cdf, cdf_test, subm_prefix,
                 hypteropt_max_evals=10, w_min=-1., w_max=1.,
                  bagging_replacement=False, bagging_fraction=0.5, bagging_size=10, init_top_k=5, prunning_fraction=0.2):
    ## load all the prediction
    maxNumValid = 12000
    pred_list_valid = np.zeros((len(model_list), config.n_runs, config.n_folds, maxNumValid), dtype=float)
    Y_list_valid = np.zeros((config.n_runs, config.n_folds, maxNumValid), dtype=float)
    cdf_list_valid = np.zeros((config.n_runs, config.n_folds, config.n_classes), dtype=float)
    numValidMatrix = np.zeros((config.n_runs, config.n_folds), dtype=int)
    p_ens_list_valid = np.zeros((config.n_runs, config.n_folds, maxNumValid), dtype=float)

    numTest = 22513
    
    ## model to idx
    model2idx = dict()
    kappa_list = dict()
    for i,model in enumerate(model_list): 
        model2idx[model] = i
        kappa_list[model] = 0
    print("============================================================")
    print("Load model...")
    for model in model_list:
        model_id = model2idx[model]
        print("model: %s" % model)
        kappa_cv = np.zeros((config.n_runs, config.n_folds), dtype=float)
        ## load cvf
        for run in range(config.n_runs):
            for fold in range(config.n_folds):
                path = "%s/Run%d/Fold%d" % (model_folder, run+1, fold+1)
                pred_file = "%s/valid.pred.%s.csv" % (path, model)
                cdf_file = "%s/Run%d/Fold%d/valid.cdf" % (feat_folder, run+1, fold+1)
                this_p_valid = pd.read_csv(pred_file, dtype=float)
                numValidMatrix[run][fold] = this_p_valid.shape[0]           
                pred_list_valid[model_id,run,fold,:numValidMatrix[run][fold]] = this_p_valid["prediction"].values
                Y_list_valid[run,fold,:numValidMatrix[run][fold]] = this_p_valid["target"].values
                ## load cdf
                if cdf == None:
                    cdf_list_valid[run,fold, :] = np.loadtxt(cdf_file, dtype=float)
                else:
                    cdf_list_valid[run,fold, :] = cdf
                ##
                score = getScore(pred_list_valid[model_id,run,fold,:numValidMatrix[run][fold]], cdf_list_valid[run,fold, :])
                kappa_cv[run][fold] = quadratic_weighted_kappa(score, Y_list_valid[run,fold,:numValidMatrix[run][fold]])     

        print("kappa: %.6f" % np.mean(kappa_cv))
        kappa_list[model] = np.mean(kappa_cv)

    cdf_mean_init = np.mean(np.mean(cdf_list_valid, axis=0), axis=0)
    cdf_mean_init = cdf_mean_init.tolist()
    cdf_mean_init.insert(0, 0)
    pdf_mean_init = np.diff(np.asarray(cdf_mean_init))

    sorted_models = sorted(kappa_list.items(), key=lambda x: x[1])[::-1]
        
    # greedy ensemble
    print("============================================================")
    print("Perform ensemble selection...")
    best_bagged_model_list = [[]]*bagging_size
    best_bagged_model_weight = [[]]*bagging_size
    num_model = len(model_list)
    #print bagging_size
    for bagging_iter in range(bagging_size):
        rng = np.random.RandomState(2015 + 100 * bagging_iter)
        if bagging_replacement:
            sampleSize = int(num_model*bagging_fraction)
            index_base = rng.randint(num_model, size=sampleSize)
        else:
            randnum = rng.uniform(size=num_model)
            index_base = [i for i in range(num_model) if randnum[i] < bagging_fraction]
        this_sorted_models = [sorted_models[i] for i in sorted(index_base)]

        #print this_model_list
        best_model_list = []
        best_model_weight = []
        best_kappa = 0
        best_model = None
        p_ens_list_valid_tmp = np.zeros((config.n_runs, config.n_folds, maxNumValid), dtype=float)
        #### initialization
        w_ens, this_w = 0, 1.0
        if init_top_k > 0:
            cnt = 0
            kappa_cv = np.zeros((config.n_runs, config.n_folds), dtype=float)
            for model,kappa in this_sorted_models:
                if cnt >= init_top_k:
                    continue
                print("add to the ensembles the following model")
                print("model: %s" % model)
                print("kappa: %.6f" % kappa)
                this_p_list_valid = pred_list_valid[model2idx[model]]
                for run in range(config.n_runs):
                    for fold in range(config.n_folds):
                        numValid = numValidMatrix[run][fold]
                        if cnt == 0:
                            this_w = 1.0
                        else:
                            pass
                        p_ens_list_valid_tmp[run,fold,:numValid] = (w_ens * p_ens_list_valid_tmp[run,fold,:numValid] + this_w * this_p_list_valid[run,fold,:numValid]) / (w_ens + this_w)
                        #p_ens_list_valid_tmp[run,fold,:numValid] = p_ens_list_valid_tmp[run,fold,:numValid].argsort().argsort()
                        if cnt == init_top_k - 1:
                            cdf = cdf_list_valid[run,fold,:]
                            true_label = Y_list_valid[run,fold,:numValid]
                            score = getScore(p_ens_list_valid_tmp[run,fold,:numValid], cdf)
                            kappa_cv[run][fold] = quadratic_weighted_kappa(score, true_label)
                best_model_list.append(model)
                best_model_weight.append(this_w)
                w_ens += this_w
                cnt += 1
            print("Init kappa: %.6f (%.6f)" % (np.mean(kappa_cv), np.std(kappa_cv)))
        #### ensemble selection with replacement
        iter = 0 
        while True:
            iter += 1
            for model,_ in this_sorted_models:
                this_p_list_valid = pred_list_valid[model2idx[model]]

                ## hyperopt for the best weight
                trials = Trials()
                param_space = {
                    'weight2': hp.uniform('weight2', w_min, w_max)
                }
                obj = lambda param: ensembleSelectionObj(param, p_ens_list_valid_tmp, 1., this_p_list_valid, Y_list_valid, cdf_list_valid, numValidMatrix)
                best_params = fmin(obj,
                                   param_space, algo=tpe.suggest,
                                   trials=trials, max_evals=hypteropt_max_evals)
                this_w = best_params['weight2']
                this_w *= w_ens
                # all the current prediction to the ensemble
                kappa_cv = np.zeros((config.n_runs, config.n_folds), dtype=float)
                for run in range(config.n_runs):
                    for fold in range(config.n_folds):
                        numValid = numValidMatrix[run][fold]
                        p1 = p_ens_list_valid_tmp[run,fold,:numValid]
                        p2 = this_p_list_valid[run,fold,:numValid]
                        true_label = Y_list_valid[run,fold,:numValid]
                        cdf = cdf_list_valid[run,fold,:]
                        p_ens = (w_ens * p1 + this_w * p2) / (w_ens + this_w)
                        score = getScore(p_ens, cdf)
                        kappa_cv[run][fold] = quadratic_weighted_kappa(score, true_label)
                if np.mean(kappa_cv) > best_kappa:
                    best_kappa, best_model, best_weight = np.mean(kappa_cv), model, this_w
            if best_model == None:
                break
            print("Iter: %d" % iter)
            print("    model: %s" % best_model)
            print("    weight: %s" % best_weight)
            print("    kappa: %.6f" % best_kappa)

            best_model_list.append(best_model)
            best_model_weight.append(best_weight)
            # valid
            this_p_list_valid = pred_list_valid[model2idx[best_model]]
            for run in range(config.n_runs):
                for fold in range(config.n_folds):
                    numValid = numValidMatrix[run][fold]
                    p_ens_list_valid_tmp[run,fold,:numValid] = (w_ens * p_ens_list_valid_tmp[run,fold,:numValid] + best_weight * this_p_list_valid[run,fold,:numValid]) / (w_ens + best_weight)
            best_model = None
            w_ens += best_weight
            
        kappa_cv = np.zeros((config.n_runs, config.n_folds), dtype=float)
        cutoff = np.zeros((3), dtype=float)
        for run in range(config.n_runs):
            for fold in range(config.n_folds):
                numValid = numValidMatrix[run][fold]
                true_label = Y_list_valid[run,fold,:numValid]
                cdf = cdf_list_valid[run,fold,:]
                p_ens_list_valid[run,fold,:numValid] = (bagging_iter * p_ens_list_valid[run,fold,:numValid] + p_ens_list_valid_tmp[run,fold,:numValid]) / (bagging_iter+1.)
                score, cutoff_tmp = getScore(p_ens_list_valid[run,fold,:numValid], cdf, "valid")
                kappa_cv[run][fold] = quadratic_weighted_kappa(score, true_label)
                cutoff += cutoff_tmp
        cutoff /= float(config.n_runs*config.n_folds)
        cutoff *= (22513/((2./3)*10158))
        print( "Bag %d, kappa: %.6f (%.6f)" % (bagging_iter+1, np.mean(kappa_cv), np.std(kappa_cv)) )
        best_kappa_mean = np.mean(kappa_cv)
        best_kappa_std = np.std(kappa_cv)
        best_bagged_model_list[bagging_iter] = best_model_list
        best_bagged_model_weight[bagging_iter] = best_model_weight

        ## save the current prediction
        # use cdf
        output = ensembleSelectionPrediction(model_folder, best_bagged_model_list[:(bagging_iter+1)], best_bagged_model_weight[:(bagging_iter+1)], cdf_test)
        sub_file = "%s_[InitTopK%d]_[BaggingSize%d]_[BaggingFraction%s]_[Mean%.6f]_[Std%.6f]_cdf.csv" % (subm_prefix, init_top_k, bagging_iter+1, bagging_fraction, best_kappa_mean, best_kappa_std)
        output.to_csv(sub_file, index=False)
        # use cutoff
        output = ensembleSelectionPrediction(model_folder, best_bagged_model_list[:(bagging_iter+1)], best_bagged_model_weight[:(bagging_iter+1)], cdf_test, cutoff)
        sub_file = "%s_[InitTopK%d]_[BaggingSize%d]_[BaggingFraction%s]_[Mean%.6f]_[Std%.6f]_cutoff.csv" % (subm_prefix, init_top_k, bagging_iter+1, bagging_fraction, best_kappa_mean, best_kappa_std)
        output.to_csv(sub_file, index=False)
    return best_kappa_mean, best_kappa_std, best_bagged_model_list, best_bagged_model_weight