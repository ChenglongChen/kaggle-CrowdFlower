
# Kaggle_CrowdFlower

1st Place Solution for [Search Results Relevance Competition on Kaggle](https://www.kaggle.com/c/crowdflower-search-relevance)

The best single model we have obtained during the competition was an [XGBoost](https://github.com/dmlc/xgboost) model with linear booster of Public LB score **0.69322** and Private LB score **0.70768**. Our final winning submission was a median ensemble of 35 best Public LB submissions. This submission scored **0.70807** on Public LB and **0.72189** on Private LB.

## What's New
* 2016/05/14: For a more clean and modularized version of this code and framework, you may want to check [Kaggle_HomeDepot](https://github.com/ChenglongChen/Kaggle_HomeDepot), which holds the code of Turing Test's solution for the recently [Home Depot Product Search Relevance Competition on Kaggle](https://www.kaggle.com/c/home-depot-product-search-relevance).

## FlowChart

<img src="./Doc/FlowChart.jpg" alt="FlowChart" align="center" width="700px"/>


## Documentation

See `./Doc/Kaggle_CrowdFlower_ChenglongChen.pdf` for documentation.

## Instruction

* download data from the [competition website](https://www.kaggle.com/c/crowdflower-search-relevance/data) and put all the data into folder `./Data`.
* run `python ./Code/Feat/run_all.py` to generate features. This will take a few hours.
* run `python ./Code/Model/generate_best_single_model.py` to generate best single model submission. In our experience, it only takes a few trials to generate model of best performance or similar performance. See the training log in `./Output/Log/[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_linear]_hyperopt.log` for example.
* run `python ./Code/Model/generate_model_library.py` to generate model library. This is quite time consuming. **But you don't have to wait for this script to finish: you can run the next step once you have some models trained.**
* run `python ./Code/Model/generate_ensemble_submission.py` to generate submission via ensemble selection.
* if you don't want to run the code, just submit the file in `./Output/Subm`.