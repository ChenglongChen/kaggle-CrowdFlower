
"""
__file__

    genFeat_basic_tfidf_feat.py

__description__

    This file generates the following features for each run and fold, and for the entire training and testing set.

        1. basic tfidf features for query/title/description
            - use common vocabulary among query/title/description for further computation of cosine similarity

        2. cosine similarity between query & title, query & description, title & description pairs
            - just plain cosine similarity

        3. cosine similarity stats features for title/description
            - computation is carried out with regard to a pool of samples grouped by:
                - median_relevance (#4)
                - query (qid) & median_relevance (#4)
            - cosine similarity for the following pairs are computed for each sample
                - sample title        vs.  pooled sample titles
                - sample description  vs.  pooled sample descriptions
                Note that in the pool samples, we exclude the current sample being considered.
            - stats features include quantiles of cosine similarity and others defined in the variable "stats_func", e.g.,
                - mean value
                - standard deviation (std)
                - more can be added, e.g., moment features etc

        4. SVD version of the above features

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import sys
import cPickle
import numpy as np
import pandas as pd
from copy import copy
from scipy.sparse import vstack
from nlp_utils import getTFV, getBOW
from feat_utils import get_sample_indices_by_relevance, dump_feat_name
from sklearn.manifold import TSNE
from sklearn.decomposition import TruncatedSVD
from sklearn.metrics.pairwise import cosine_similarity, pairwise_distances
sys.path.append("../")
from param_config import config


stats_feat_flag = True


#####################
## Helper function ##
#####################
## compute cosine similarity
def cosine_sim(x, y):
    try:
        d = cosine_similarity(x, y)
        d = d[0][0]
    except:
        print x
        print y
        d = 0.
    return d

## generate distance stats feat
def generate_dist_stats_feat(metric, X_train, ids_train, X_test, ids_test, indices_dict, qids_test=None):
    if metric == "cosine":
        stats_feat = 0 * np.ones((len(ids_test), stats_feat_num*config.n_classes), dtype=float)
        sim = 1. - pairwise_distances(X_test, X_train, metric=metric, n_jobs=1)
    elif metric == "euclidean":
        stats_feat = -1 * np.ones((len(ids_test), stats_feat_num*config.n_classes), dtype=float)
        sim = pairwise_distances(X_test, X_train, metric=metric, n_jobs=1)

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
                sim_tmp = sim[i][inds]
                if len(sim_tmp) != 0:
                    feat = [ func(sim_tmp) for func in stats_func ]
                    ## quantile
                    sim_tmp = pd.Series(sim_tmp)
                    quantiles = sim_tmp.quantile(quantiles_range)
                    feat = np.hstack((feat, quantiles))
                    stats_feat[i,j*stats_feat_num:(j+1)*stats_feat_num] = feat
    return stats_feat


## extract all features
def extract_feat(path, dfTrain, dfTest, mode, feat_names, column_names):

    new_feat_names = copy(feat_names)
    ## first fit a bow/tfidf on the all_text to get
    ## the common vocabulary to ensure query/title/description
    ## has the same length bow/tfidf for computing the similarity
    if vocabulary_type == "common":
        if vec_type == "tfidf":
            vec = getTFV(ngram_range=ngram_range)
        elif vec_type == "bow":
            vec = getBOW(ngram_range=ngram_range)
        vec.fit(dfTrain["all_text"])
        vocabulary = vec.vocabulary_
    elif vocabulary_type == "individual":
        vocabulary = None
    for feat_name,column_name in zip(feat_names, column_names):

        ##########################
        ## basic bow/tfidf feat ##
        ##########################
        print "generate %s feat for %s" % (vec_type, column_name)
        if vec_type == "tfidf":
            vec = getTFV(ngram_range=ngram_range, vocabulary=vocabulary)
        elif vec_type == "bow":
            vec = getBOW(ngram_range=ngram_range, vocabulary=vocabulary)
        X_train = vec.fit_transform(dfTrain[column_name])
        X_test = vec.transform(dfTest[column_name])
        with open("%s/train.%s.feat.pkl" % (path, feat_name), "wb") as f:
            cPickle.dump(X_train, f, -1)
        with open("%s/%s.%s.feat.pkl" % (path, mode, feat_name), "wb") as f:
            cPickle.dump(X_test, f, -1)
        
        if stats_feat_flag:
            #####################################
            ## bow/tfidf cosine sim stats feat ##
            #####################################
            ## get the indices of pooled samples
            relevance_indices_dict = get_sample_indices_by_relevance(dfTrain)
            query_relevance_indices_dict = get_sample_indices_by_relevance(dfTrain, "qid")
            ## skip query part
            if column_name in ["product_title", "product_description"]:
                print "generate %s stats feat for %s" % (vec_type, column_name)
                ## train
                cosine_sim_stats_feat_by_relevance_train = generate_dist_stats_feat("cosine", X_train, dfTrain["id"].values,
                                                                    X_train, dfTrain["id"].values,
                                                                    relevance_indices_dict)
                cosine_sim_stats_feat_by_query_relevance_train = generate_dist_stats_feat("cosine", X_train, dfTrain["id"].values,
                                                                            X_train, dfTrain["id"].values,
                                                                            query_relevance_indices_dict, dfTrain["qid"].values)
                with open("%s/train.%s_cosine_sim_stats_feat_by_relevance.feat.pkl" % (path, feat_name), "wb") as f:
                    cPickle.dump(cosine_sim_stats_feat_by_relevance_train, f, -1)
                with open("%s/train.%s_cosine_sim_stats_feat_by_query_relevance.feat.pkl" % (path, feat_name), "wb") as f:
                    cPickle.dump(cosine_sim_stats_feat_by_query_relevance_train, f, -1)
                ## test
                cosine_sim_stats_feat_by_relevance_test = generate_dist_stats_feat("cosine", X_train, dfTrain["id"].values,
                                                                    X_test, dfTest["id"].values,
                                                                    relevance_indices_dict)
                cosine_sim_stats_feat_by_query_relevance_test = generate_dist_stats_feat("cosine", X_train, dfTrain["id"].values,
                                                                            X_test, dfTest["id"].values,
                                                                            query_relevance_indices_dict, dfTest["qid"].values)
                with open("%s/%s.%s_cosine_sim_stats_feat_by_relevance.feat.pkl" % (path, mode, feat_name), "wb") as f:
                    cPickle.dump(cosine_sim_stats_feat_by_relevance_test, f, -1)
                with open("%s/%s.%s_cosine_sim_stats_feat_by_query_relevance.feat.pkl" % (path, mode, feat_name), "wb") as f:
                    cPickle.dump(cosine_sim_stats_feat_by_query_relevance_test, f, -1)

                ## update feat names
                new_feat_names.append( "%s_cosine_sim_stats_feat_by_relevance" % feat_name )
                new_feat_names.append( "%s_cosine_sim_stats_feat_by_query_relevance" % feat_name )


    #####################
    ## cosine sim feat ##
    #####################
    for i in range(len(feat_names)-1):
        for j in range(i+1,len(feat_names)):
            print "generate common %s cosine sim feat for %s and %s" % (vec_type, feat_names[i], feat_names[j])
            for mod in ["train", mode]:
                with open("%s/%s.%s.feat.pkl" % (path, mod, feat_names[i]), "rb") as f:
                    target_vec = cPickle.load(f)
                with open("%s/%s.%s.feat.pkl" % (path, mod, feat_names[j]), "rb") as f:
                    obs_vec = cPickle.load(f)
                sim = np.asarray(map(cosine_sim, target_vec, obs_vec))[:,np.newaxis]
                ## dump feat
                with open("%s/%s.%s_%s_%s_cosine_sim.feat.pkl" % (path, mod, feat_names[i], feat_names[j], vec_type), "wb") as f:
                    cPickle.dump(sim, f, -1)
            ## update feat names
            new_feat_names.append( "%s_%s_%s_cosine_sim" % (feat_names[i], feat_names[j], vec_type))


    ##################
    ## SVD features ##
    ##################
    ## we fit svd use stacked query/title/description bow/tfidf for further cosine simalirity computation
    for i,feat_name in enumerate(feat_names):
        with open("%s/train.%s.feat.pkl" % (path, feat_name), "rb") as f:
            X_vec_train = cPickle.load(f)
        if i == 0:
            X_vec_all_train = X_vec_train
        else:
            X_vec_all_train = vstack([X_vec_all_train, X_vec_train])

    for n_components in svd_n_components:
        svd = TruncatedSVD(n_components=n_components, n_iter=15)
        svd.fit(X_vec_all_train)
        ## load bow/tfidf (for less coding...)
        for feat_name,column_name in zip(feat_names, column_names):
            print "generate common %s-svd%d feat for %s" % (vec_type, n_components, column_name)
            with open("%s/train.%s.feat.pkl" % (path, feat_name), "rb") as f:
                X_vec_train = cPickle.load(f)
            with open("%s/%s.%s.feat.pkl" % (path, mode, feat_name), "rb") as f:
                X_vec_test = cPickle.load(f)
            X_svd_train = svd.transform(X_vec_train)
            X_svd_test = svd.transform(X_vec_test)
            with open("%s/train.%s_common_svd%d.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                cPickle.dump(X_svd_train, f, -1)
            with open("%s/%s.%s_common_svd%d.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                cPickle.dump(X_svd_test, f, -1)

            ## update feat names
            new_feat_names.append( "%s_common_svd%d" % (feat_name, n_components) )
            
            if stats_feat_flag:
                #####################################
                ## bow/tfidf-svd cosine sim stats feat ##
                #####################################
                if column_name in ["product_title", "product_description"]:
                    print "generate common %s-svd%d stats feat for %s" % (vec_type, n_components, column_name)
                    ## train
                    cosine_sim_stats_feat_by_relevance_train = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                        X_svd_train, dfTrain["id"].values,
                                                                        relevance_indices_dict)
                    cosine_sim_stats_feat_by_query_relevance_train = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                                X_svd_train, dfTrain["id"].values,
                                                                                query_relevance_indices_dict, dfTrain["qid"].values)
                    with open("%s/train.%s_common_svd%d_cosine_sim_stats_feat_by_relevance.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_relevance_train, f, -1)
                    with open("%s/train.%s_common_svd%d_cosine_sim_stats_feat_by_query_relevance.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_query_relevance_train, f, -1)
                    ## test
                    cosine_sim_stats_feat_by_relevance_test = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                        X_svd_test, dfTest["id"].values,
                                                                        relevance_indices_dict)
                    cosine_sim_stats_feat_by_query_relevance_test = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                                X_svd_test, dfTest["id"].values,
                                                                                query_relevance_indices_dict, dfTest["qid"].values)
                    with open("%s/%s.%s_common_svd%d_cosine_sim_stats_feat_by_relevance.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_relevance_test, f, -1)
                    with open("%s/%s.%s_common_svd%d_cosine_sim_stats_feat_by_query_relevance.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_query_relevance_test, f, -1)

                    ## update feat names
                    new_feat_names.append( "%s_common_svd%d_cosine_sim_stats_feat_by_relevance" % (feat_name, n_components) )
                    new_feat_names.append( "%s_common_svd%d_cosine_sim_stats_feat_by_query_relevance" % (feat_name, n_components) )


        #####################
        ## cosine sim feat ##
        #####################
        for i in range(len(feat_names)-1):
            for j in range(i+1,len(feat_names)):
                print "generate common %s-svd%d cosine sim feat for %s and %s" % (vec_type, n_components, feat_names[i], feat_names[j])
                for mod in ["train", mode]:
                    with open("%s/%s.%s_common_svd%d.feat.pkl" % (path, mod, feat_names[i], n_components), "rb") as f:
                        target_vec = cPickle.load(f)
                    with open("%s/%s.%s_common_svd%d.feat.pkl" % (path, mod, feat_names[j], n_components), "rb") as f:
                        obs_vec = cPickle.load(f)
                    sim = np.asarray(map(cosine_sim, target_vec, obs_vec))[:,np.newaxis]
                    ## dump feat
                    with open("%s/%s.%s_%s_%s_common_svd%d_cosine_sim.feat.pkl" % (path, mod, feat_names[i], feat_names[j], vec_type, n_components), "wb") as f:
                        cPickle.dump(sim, f, -1)
                ## update feat names
                new_feat_names.append( "%s_%s_%s_common_svd%d_cosine_sim" % (feat_names[i], feat_names[j], vec_type, n_components))

        #########################
        ## Individual SVD feat ##
        #########################
        ## generate individual svd feat
        for feat_name,column_name in zip(feat_names, column_names):
            print "generate individual %s-svd%d feat for %s" % (vec_type, n_components, column_name)
            with open("%s/train.%s.feat.pkl" % (path, feat_name), "rb") as f:
                X_vec_train = cPickle.load(f)
            with open("%s/%s.%s.feat.pkl" % (path, mode, feat_name), "rb") as f:
                X_vec_test = cPickle.load(f)
            svd = TruncatedSVD(n_components=n_components, n_iter=15)
            X_svd_train = svd.fit_transform(X_vec_train)
            X_svd_test = svd.transform(X_vec_test)
            with open("%s/train.%s_individual_svd%d.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                cPickle.dump(X_svd_train, f, -1)
            with open("%s/%s.%s_individual_svd%d.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                cPickle.dump(X_svd_test, f, -1)
            ## update feat names
            new_feat_names.append( "%s_individual_svd%d" % (feat_name, n_components) )

            if stats_feat_flag:
                #########################################
                ## bow/tfidf-svd cosine sim stats feat ##
                #########################################
                if column_name in ["product_title", "product_description"]:
                    print "generate individual %s-svd%d stats feat for %s" % (vec_type, n_components, column_name)
                    ## train
                    cosine_sim_stats_feat_by_relevance_train = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                        X_svd_train, dfTrain["id"].values,
                                                                        relevance_indices_dict)
                    cosine_sim_stats_feat_by_query_relevance_train = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                                X_svd_train, dfTrain["id"].values,
                                                                                query_relevance_indices_dict, dfTrain["qid"].values)
                    with open("%s/train.%s_individual_svd%d_cosine_sim_stats_feat_by_relevance.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_relevance_train, f, -1)
                    with open("%s/train.%s_individual_svd%d_cosine_sim_stats_feat_by_query_relevance.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_query_relevance_train, f, -1)
                    ## test
                    cosine_sim_stats_feat_by_relevance_test = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                        X_svd_test, dfTest["id"].values,
                                                                        relevance_indices_dict)
                    cosine_sim_stats_feat_by_query_relevance_test = generate_dist_stats_feat("cosine", X_svd_train, dfTrain["id"].values,
                                                                                X_svd_test, dfTest["id"].values,
                                                                                query_relevance_indices_dict, dfTest["qid"].values)
                    with open("%s/%s.%s_individual_svd%d_cosine_sim_stats_feat_by_relevance.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_relevance_test, f, -1)
                    with open("%s/%s.%s_individual_svd%d_cosine_sim_stats_feat_by_query_relevance.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                        cPickle.dump(cosine_sim_stats_feat_by_query_relevance_test, f, -1)

                    ## update feat names
                    new_feat_names.append( "%s_individual_svd%d_cosine_sim_stats_feat_by_relevance" % (feat_name, n_components) )
                    new_feat_names.append( "%s_individual_svd%d_cosine_sim_stats_feat_by_query_relevance" % (feat_name, n_components) )

    """
    #########################
    ## bow/tfidf-tsne feat ##
    #########################
    ## generate t-sne feat
    for n_components in tsne_n_components:
        for feat_name,column_name in zip(feat_names, column_names):
            print "generate individual %s-tsne%d feat for %s" % (vec_type, n_components, column_name)
            with open("%s/train.%s.feat.pkl" % (path, feat_name), "rb") as f:
                X_vec_train = cPickle.load(f)
            with open("%s/%s.%s.feat.pkl" % (path, mode, feat_name), "rb") as f:
                X_vec_test = cPickle.load(f)
            tsne = TSNE(n_components=n_components, init='pca', random_state=2015, metric="cosine")
            X = vstack([X_vec_train, X_vec_test])
            Y = tsne.fit_transform(X)
            num_train = X_vec_train.shape[0]
            X_tsne_train = Y[:num_train]
            X_tsne_test = Y[num_train:]
            with open("%s/train.%s_individual_tsne%d.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                cPickle.dump(X_tsne_train, f, -1)
            with open("%s/%s.%s_individual_tsne%d.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                cPickle.dump(X_tsne_test, f, -1)

            ##################################################
            ## bow/tfidf-tsne euclidean distance stats feat ##
            ##################################################
            if column_name in ["product_title", "product_description"]:
                print "generate individual %s-tsne%d stats feat for %s" % (vec_type, n_components, column_name)
                ## train
                euclidean_dist_stats_feat_by_relevance_train = generate_dist_stats_feat("euclidean", X_tsne_train, dfTrain["id"].values,
                                                                    X_tsne_train, dfTrain["id"].values,
                                                                    relevance_indices_dict)
                euclidean_dist_stats_feat_by_query_relevance_train = generate_dist_stats_feat("euclidean", X_tsne_train, dfTrain["id"].values,
                                                                            X_tsne_train, dfTrain["id"].values,
                                                                            query_relevance_indices_dict, dfTrain["qid"].values)
                with open("%s/train.%s_individual_tsne%d_euclidean_dist_stats_feat_by_relevance.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                    cPickle.dump(euclidean_dist_stats_feat_by_relevance_train, f, -1)
                with open("%s/train.%s_individual_tsne%d_euclidean_dist_stats_feat_by_query_relevance.feat.pkl" % (path, feat_name, n_components), "wb") as f:
                    cPickle.dump(euclidean_dist_stats_feat_by_query_relevance_train, f, -1)
                ## test
                euclidean_dist_stats_feat_by_relevance_test = generate_dist_stats_feat("euclidean", X_tsne_train, dfTrain["id"].values,
                                                                    X_tsne_test, dfTest["id"].values,
                                                                    relevance_indices_dict)
                euclidean_dist_stats_feat_by_query_relevance_test = generate_dist_stats_feat("euclidean", X_tsne_train, dfTrain["id"].values,
                                                                            X_tsne_test, dfTest["id"].values,
                                                                            query_relevance_indices_dict, dfTest["qid"].values)
                with open("%s/%s.%s_individual_tsne%d_euclidean_dist_stats_feat_by_relevance.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                    cPickle.dump(euclidean_dist_stats_feat_by_relevance_test, f, -1)
                with open("%s/%s.%s_individual_tsne%d_euclidean_dist_stats_feat_by_query_relevance.feat.pkl" % (path, mode, feat_name, n_components), "wb") as f:
                    cPickle.dump(euclidean_dist_stats_feat_by_query_relevance_test, f, -1)

                ## update feat names
                new_feat_names.append( "%s_individual_tsne%d_euclidean_dist_stats_feat_by_relevance" % (feat_name, n_components) )
                new_feat_names.append( "%s_individual_tsne%d_euclidean_dist_stats_feat_by_query_relevance" % (feat_name, n_components) )
    """

    return new_feat_names


if __name__ == "__main__":

    ############
    ## Config ##
    ############
    ## stats to extract
    quantiles_range = np.arange(0, 1.5, 0.5)
    stats_func = [ np.mean, np.std ]
    stats_feat_num = len(quantiles_range) + len(stats_func)

    ## tfidf config
    vec_types = [ "tfidf", "bow" ]
    ngram_range = config.basic_tfidf_ngram_range
    vocabulary_type = config.basic_tfidf_vocabulary_type
    svd_n_components = [100, 150]
    tsne_n_components = [2]

    ## feat name config
    column_names = [ "query", "product_title", "product_description" ]

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

    ## for fitting common vocabulary
    def cat_text(x):
        res = '%s %s %s' % (x['query'], x['product_title'], x['product_description'])
        return res
    dfTrain["all_text"] = list(dfTrain.apply(cat_text, axis=1))
    dfTest["all_text"] = list(dfTest.apply(cat_text, axis=1))

    for vec_type in vec_types:
        ## save feat names
        feat_names = [ "query", "title", "description" ]
        feat_names = [ name+"_%s_%s_vocabulary" % (vec_type, vocabulary_type) for name in feat_names ]
        ## file to save feat names
        feat_name_file = "%s/basic_%s_and_cosine_sim.feat_name" % (config.feat_folder, vec_type)

        #######################
        ## Generate Features ##
        #######################
        print("==================================================")
        print("Generate basic %s features..." % vec_type)

        print("For cross-validation...")
        for run in range(config.n_runs):
            ## use 33% for training and 67 % for validation
            ## so we switch trainInd and validInd
            for fold, (validInd, trainInd) in enumerate(skf[run]):
                print("Run: %d, Fold: %d" % (run+1, fold+1))
                path = "%s/Run%d/Fold%d" % (config.feat_folder, run+1, fold+1)
                
                dfTrain2 = dfTrain.iloc[trainInd].copy()
                dfValid = dfTrain.iloc[validInd].copy()
                ## extract feat
                extract_feat(path, dfTrain2, dfValid, "valid", feat_names, column_names)

        print("Done.")

        print("For training and testing...")
        path = "%s/All" % config.feat_folder
        ## extract feat
        feat_names = extract_feat(path, dfTrain, dfTest, "test", feat_names, column_names)
        ## dump feat name
        dump_feat_name(feat_names, feat_name_file)

        print("All Done.")