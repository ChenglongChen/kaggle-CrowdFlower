
"""
__file__

	generate_model_library.py

__description__

	This file generates model library for ensemble selection.

__author__

	Chenglong Chen < c.chenglong@gmail.com >

"""

import os

feat_names_all = [
	## LSA_and_stats_feat_Jun09 (Low)
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_xgb_tree]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@cocr_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@kappa_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_etr]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_rf]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_gbm]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_svr]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_ridge]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_lasso]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@clf_skl_lr]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_libfm]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_keras_dnn]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_rgf]",

	## LSA_svd150_and_Jaccard_coef_Jun14 (Low)
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_xgb_tree]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@cocr_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@kappa_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_etr]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_rf]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_gbm]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_svr]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_ridge]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_lasso]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@clf_skl_lr]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_libfm]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_keras_dnn]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_rgf]",

	## svd100_and_bow_Jun23 (Low)
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_xgb_tree]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@cocr_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@kappa_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_etr]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_rf]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_gbm]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_svr]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_ridge]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_lasso]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@clf_skl_lr]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_libfm]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_keras_dnn]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_rgf]",
	
	## svd100_and_bow_Jun27 (High)
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@cocr_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@kappa_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_ridge]",
]

feat_names = [
	## LSA_and_stats_feat_Jun09 (Low)
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_xgb_tree]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@cocr_xgb_linear]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_ridge]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_lasso]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_etr]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_rf]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_gbm]",
	"[Pre@solution]_[Feat@LSA_and_stats_feat_Jun09]_[Model@reg_skl_svr]",

	## LSA_svd150_and_Jaccard_coef_Jun14 (Low)
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_etr]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_gbm]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_skl_svr]",
	"[Pre@solution]_[Feat@LSA_svd150_and_Jaccard_coef_Jun14]_[Model@reg_keras_dnn]",

	## svd100_and_bow_Jun23 (Low)
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_etr]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_gbm]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_skl_svr]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun23]_[Model@reg_keras_dnn]",

	## svd100_and_bow_Jun27 (High)
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@cocr_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@kappa_xgb_linear]",
	"[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_skl_ridge]",
]

for feat_name in feat_names:
	cmd = "python ./train_model.py %s" % feat_name
	os.system( cmd )