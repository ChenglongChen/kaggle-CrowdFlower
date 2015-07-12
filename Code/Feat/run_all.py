
"""
__file__

	run_all.py

__description___
	
	This file generates all the features in one shot.

__author__

	Chenglong Chen < c.chenglong@gmail.com >

"""

import os

#################
## Preprocesss ##
#################
#### preprocess data
cmd = "python ./preprocess.py"
os.system(cmd)

# #### generate kfold
# cmd = "python ./gen_kfold.py"
# os.system(cmd)

#######################
## Generate features ##
#######################
#### query id
cmd = "python ./genFeat_id_feat.py"
os.system(cmd)

#### counting feat
cmd = "python ./genFeat_counting_feat.py"
os.system(cmd)

#### distance feat
cmd = "python ./genFeat_distance_feat.py"
os.system(cmd)

#### basic tfidf
cmd = "python ./genFeat_basic_tfidf_feat.py"
os.system(cmd)

#### cooccurrence tfidf
cmd = "python ./genFeat_cooccurrence_tfidf_feat.py"
os.system(cmd)


#####################
## Combine Feature ##
#####################
#### combine feat
cmd = "python ./combine_feat_[LSA_and_stats_feat_Jun09]_[Low].py"
os.system(cmd)

#### combine feat
cmd = "python ./combine_feat_[LSA_svd150_and_Jaccard_coef_Jun14]_[Low].py"
os.system(cmd)

#### combine feat
cmd = "python ./combine_feat_[svd100_and_bow_Jun23]_[Low].py"
os.system(cmd)

#### combine feat
cmd = "python ./combine_feat_[svd100_and_bow_Jun27]_[High].py"
os.system(cmd)