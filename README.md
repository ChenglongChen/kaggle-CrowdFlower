
# Kaggle_CrowdFlower

1st Place Solution for Search Results Relevance Competition on Kaggle (https://www.kaggle.com/c/crowdflower-search-relevance)

See `./Doc/Kaggle_CrowdFlower_ChenglongChen.pdf` for documentation.

## Instruction

* download data from the [competition website](https://www.kaggle.com/c/crowdflower-search-relevance/data) and put all the data into folder `./Data`.
* run `python ./Code/Feat/run_all.py` to generate features. This will take a few hours.
* run `python ./Code/Model/generate_best_single_model.py` to generate best single model submission. In my experience, it only takes a few trials to generate model of best performance or similar performance. See the training log in `./Output/Log/[Pre@solution]_[Feat@svd100_and_bow_Jun27]_[Model@reg_xgb_linear]_hyperopt.log` for example.
* run `python ./Code/Model/generate_model_library.py` to generate model library. This is quite time consuming. **But you don't have to wait for this script to finish: you can run the next step once you have some models trained.**
* run `python ./Code/Model/generate_ensemble_submission.py` to generate submission via ensemble selection.
* if you don't want to run the code, just submit the file in `./Output/Subm`.