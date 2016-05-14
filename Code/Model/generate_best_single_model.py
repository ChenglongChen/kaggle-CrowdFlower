
"""
__file__

	generate_best_single_model.py

__description__

	This file generates the best single model.

__author__

	Chenglong Chen < c.chenglong@gmail.com >

"""

import os

feat_names = [
	## svd100_and_bow_Jun27 (High)
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_linear]",
	
	# ## you can also try the following models
	# "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@cocr_xgb_linear]",
	# "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@kappa_xgb_linear]",
	# "[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_ridge]",
]

for feat_name in feat_names:
	cmd = "python ./train_model.py %s" % feat_name
	os.system( cmd )