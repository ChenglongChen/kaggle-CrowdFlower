
"""
__file__

    genFeat_distance_feat.py

__description__

    This file generates the following features for each run and fold, and for the entire training and testing set.

        1. jaccard coefficient/dice distance between query & title, query & description, title & description pairs
            - just plain jaccard coefficient/dice distance
            - compute for unigram/bigram/trigram

        2. jaccard coefficient/dice distance stats features for title/description
            - computation is carried out with regard to a pool of samples grouped by:
                - median_relevance (#4)
                - query (qid) & median_relevance (#4)
            - jaccard coefficient/dice distance for the following pairs are computed for each sample
                - sample title        vs.  pooled sample titles
                - sample description  vs.  pooled sample descriptions
                Note that in the pool samples, we exclude the current sample being considered.
            - stats features include quantiles of cosine similarity and others defined in the variable "stats_func", e.g.,
                - mean value
                - standard deviation (std)
                - more can be added, e.g., moment features etc

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import re
import sys
import ngram
import cPickle
import numpy as np
import pandas as pd
from copy import copy
from nlp_utils import stopwords, english_stemmer, stem_tokens
from feat_utils import try_divide, get_sample_indices_by_relevance, dump_feat_name
sys.path.append("../")
from param_config import config

## stats feat is quite time-consuming to generate
## (about 2 days to generate on my computer)
stats_feat_flag = False


#####################
## Distance metric ##
#####################
def JaccardCoef(A, B):
    A, B = set(A), set(B)
    intersect = len(A.intersection(B))
    union = len(A.union(B))
    coef = try_divide(intersect, union)
    return coef

def DiceDist(A, B):
    A, B = set(A), set(B)
    intersect = len(A.intersection(B))
    union = len(A) + len(B)
    d = try_divide(2*intersect, union)
    return d

def compute_dist(A, B, dist="jaccard_coef"):
    if dist == "jaccard_coef":
        d = JaccardCoef(A, B)
    elif dist == "dice_dist":
        d = DiceDist(A, B)
    return d

#### pairwise distance
def pairwise_jaccard_coef(A, B):
    coef = np.zeros((A.shape[0], B.shape[0]), dtype=float)
    for i in range(A.shape[0]):
        for j in range(B.shape[0]):
            coef[i,j] = JaccardCoef(A[i], B[j])
    return coef
    
def pairwise_dice_dist(A, B):
    d = np.zeros((A.shape[0], B.shape[0]), dtype=float)
    for i in range(A.shape[0]):
        for j in range(B.shape[0]):
            d[i,j] = DiceDist(A[i], B[j])
    return d

def pairwise_dist(A, B, dist="jaccard_coef"):
    if dist == "jaccard_coef":
        d = pairwise_jaccard_coef(A, B)
    elif dist == "dice_dist":
        d = pairwise_dice_dist(A, B)
    return d


######################
## Pre-process data ##
######################
token_pattern = r"(?u)\b\w\w+\b"
#token_pattern = r'\w{1,}'
#token_pattern = r"\w+"
#token_pattern = r"[\w']+"
transform = config.count_feat_transform
def preprocess_data(line, token_pattern=token_pattern,
                     exclude_stopword=config.cooccurrence_word_exclude_stopword,
                     encode_digit=False):
    token_pattern = re.compile(token_pattern, flags = re.UNICODE | re.LOCALE)
    ## tokenize
    tokens = [x.lower() for x in token_pattern.findall(line)]
    ## stem
    tokens_stemmed = stem_tokens(tokens, english_stemmer)
    if exclude_stopword:
        tokens_stemmed = [x for x in tokens_stemmed if x not in stopwords]
    return tokens_stemmed


#####################################
## Extract basic distance features ##
#####################################
def extract_basic_distance_feat(df):
    ## unigram
    print "generate unigram"
    df["query_unigram"] = list(df.apply(lambda x: preprocess_data(x["query"]), axis=1))
    df["title_unigram"] = list(df.apply(lambda x: preprocess_data(x["product_title"]), axis=1))
    df["description_unigram"] = list(df.apply(lambda x: preprocess_data(x["product_description"]), axis=1))
    ## bigram
    print "generate bigram"
    join_str = "_"
    df["query_bigram"] = list(df.apply(lambda x: ngram.getBigram(x["query_unigram"], join_str), axis=1))
    df["title_bigram"] = list(df.apply(lambda x: ngram.getBigram(x["title_unigram"], join_str), axis=1))
    df["description_bigram"] = list(df.apply(lambda x: ngram.getBigram(x["description_unigram"], join_str), axis=1))
    ## trigram
    print "generate trigram"
    join_str = "_"
    df["query_trigram"] = list(df.apply(lambda x: ngram.getTrigram(x["query_unigram"], join_str), axis=1))
    df["title_trigram"] = list(df.apply(lambda x: ngram.getTrigram(x["title_unigram"], join_str), axis=1))
    df["description_trigram"] = list(df.apply(lambda x: ngram.getTrigram(x["description_unigram"], join_str), axis=1))

    ## jaccard coef/dice dist of n-gram
    print "generate jaccard coef and dice dist for n-gram"
    dists = ["jaccard_coef", "dice_dist"]
    grams = ["unigram", "bigram", "trigram"]
    feat_names = ["query", "title", "description"]
    for dist in dists:
        for gram in grams:
            for i in range(len(feat_names)-1):
                for j in range(i+1,len(feat_names)):
                    target_name = feat_names[i]
                    obs_name = feat_names[j]
                    df["%s_of_%s_between_%s_%s"%(dist,gram,target_name,obs_name)] = \
                            list(df.apply(lambda x: compute_dist(x[target_name+"_"+gram], x[obs_name+"_"+gram], dist), axis=1))


###########################################
## Extract statistical distance features ##
###########################################
## generate dist stats feat
def generate_dist_stats_feat(dist, X_train, ids_train, X_test, ids_test, indices_dict, qids_test=None):

    stats_feat = 0 * np.ones((len(ids_test), stats_feat_num*config.n_classes), dtype=float)
    ## pairwise dist
    distance = pairwise_dist(X_test, X_train, dist)
    for i in range(len(ids_test)):
        id = ids_test[i]
        if qids_test is not None:
            qid = qids_test[i]
        for j in range(config.n_classes):
            key = (qid, j+1) if qids_test is not None else j+1
            if indices_dict.has_key(key):
                inds = indices_dict[key]
                # exclude this sample itself from the list of indices
                inds = [ ind for ind in inds if id != ids_train[ind] ]
                distance_tmp = distance[i][inds]
                if len(distance_tmp) != 0:
                    feat = [ func(distance_tmp) for func in stats_func ]
                    ## quantile
                    distance_tmp = pd.Series(distance_tmp)
                    quantiles = distance_tmp.quantile(quantiles_range)
                    feat = np.hstack((feat, quantiles))
                    stats_feat[i,j*stats_feat_num:(j+1)*stats_feat_num] = feat
    return stats_feat


def extract_statistical_distance_feat(path, dfTrain, dfTest, mode, feat_names):

    new_feat_names = copy(feat_names)
    ## get the indices of pooled samples
    relevance_indices_dict = get_sample_indices_by_relevance(dfTrain)
    query_relevance_indices_dict = get_sample_indices_by_relevance(dfTrain, "qid")
    ## very time consuming
    for dist in ["jaccard_coef", "dice_dist"]:
        for name in ["title", "description"]:
            for gram in ["unigram", "bigram", "trigram"]:
                ## train
                dist_stats_feat_by_relevance_train = generate_dist_stats_feat(dist, dfTrain[name+"_"+gram].values, dfTrain["id"].values,
                                                            dfTrain[name+"_"+gram].values, dfTrain["id"].values,
                                                            relevance_indices_dict)
                dist_stats_feat_by_query_relevance_train = generate_dist_stats_feat(dist, dfTrain[name+"_"+gram].values, dfTrain["id"].values,
                                                                dfTrain[name+"_"+gram].values, dfTrain["id"].values,
                                                                query_relevance_indices_dict, dfTrain["qid"].values)
                with open("%s/train.%s_%s_%s_stats_feat_by_relevance.feat.pkl" % (path, name, gram, dist), "wb") as f:
                    cPickle.dump(dist_stats_feat_by_relevance_train, f, -1)
                with open("%s/train.%s_%s_%s_stats_feat_by_query_relevance.feat.pkl" % (path, name, gram, dist), "wb") as f:
                    cPickle.dump(dist_stats_feat_by_query_relevance_train, f, -1)
                ## test
                dist_stats_feat_by_relevance_test = generate_dist_stats_feat(dist, dfTrain[name+"_"+gram].values, dfTrain["id"].values,
                                                            dfTest[name+"_"+gram].values, dfTest["id"].values,
                                                            relevance_indices_dict)
                dist_stats_feat_by_query_relevance_test = generate_dist_stats_feat(dist, dfTrain[name+"_"+gram].values, dfTrain["id"].values,
                                                                dfTest[name+"_"+gram].values, dfTest["id"].values,
                                                                query_relevance_indices_dict, dfTest["qid"].values)
                with open("%s/%s.%s_%s_%s_stats_feat_by_relevance.feat.pkl" % (path, mode, name, gram, dist), "wb") as f:
                    cPickle.dump(dist_stats_feat_by_relevance_test, f, -1)
                with open("%s/%s.%s_%s_%s_stats_feat_by_query_relevance.feat.pkl" % (path, mode, name, gram, dist), "wb") as f:
                    cPickle.dump(dist_stats_feat_by_query_relevance_test, f, -1)

                ## update feat names
                new_feat_names.append( "%s_%s_%s_stats_feat_by_relevance" % (name, gram, dist) )
                new_feat_names.append( "%s_%s_%s_stats_feat_by_query_relevance" % (name, gram, dist) )

    return new_feat_names


##########
## Main ##
##########
if __name__ == "__main__":

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


    ## file to save feat names
    feat_name_file = "%s/distance.feat_name" % config.feat_folder


    ## stats to extract
    quantiles_range = np.arange(0, 1.5, 0.5)
    stats_func = [ np.mean, np.std ]
    stats_feat_num = len(quantiles_range) + len(stats_func)

    #######################
    ## Generate Features ##
    #######################
    print("==================================================")
    print("Generate distance features...")

    extract_basic_distance_feat(dfTrain)
    feat_names = [name for name in dfTrain.columns if "jaccard_coef" in name or "dice_dist" in name]

    print("For cross-validation...")
    for run in range(config.n_runs):
        ## use 33% for training and 67 % for validation
        ## so we switch trainInd and validInd
        for fold, (validInd, trainInd) in enumerate(skf[run]):
            print("Run: %d, Fold: %d" % (run+1, fold+1))
            path = "%s/Run%d/Fold%d" % (config.feat_folder, run+1, fold+1)
              
            for feat_name in feat_names:
                X_train = dfTrain[feat_name].values[trainInd]
                X_valid = dfTrain[feat_name].values[validInd]
                with open("%s/train.%s.feat.pkl" % (path, feat_name), "wb") as f:
                    cPickle.dump(X_train, f, -1)
                with open("%s/valid.%s.feat.pkl" % (path, feat_name), "wb") as f:
                    cPickle.dump(X_valid, f, -1)
            ## extract statistical distance features
            if stats_feat_flag:
                dfTrain2 = dfTrain.iloc[trainInd].copy()
                dfValid = dfTrain.iloc[validInd].copy()
                extract_statistical_distance_feat(path, dfTrain2, dfValid, "valid", feat_names)

    print("Done.")


    print("For training and testing...")
    path = "%s/All" % config.feat_folder
    ## use full version for X_train
    extract_basic_distance_feat(dfTest)
    for feat_name in feat_names:
        X_train = dfTrain[feat_name].values
        X_test = dfTest[feat_name].values
        with open("%s/train.%s.feat.pkl" % (path, feat_name), "wb") as f:
            cPickle.dump(X_train, f, -1)
        with open("%s/test.%s.feat.pkl" % (path, feat_name), "wb") as f:
            cPickle.dump(X_test, f, -1)
    ## extract statistical distance features
    if stats_feat_flag:
        feat_names = extract_statistical_distance_feat(path, dfTrain, dfTest, "test", feat_names)

    ## save feat names
    print("Feature names are stored in %s" % feat_name_file)
    ## dump feat name
    dump_feat_name(feat_names, feat_name_file)
            
    print("All Done.")