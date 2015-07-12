
# Kaggle_CrowdFlower

Winning Solution for Search Results Relevance Competition on Kaggle (https://www.kaggle.com/c/crowdflower-search-relevance)

See `./Doc/Kaggle_CrowdFlower_ChenglongChen.pdf` for documentation.

## Instruction

* download data from the [competition website](https://www.kaggle.com/c/crowdflower-search-relevance/data) and put all the data into folder `./Data`.
* run `python ./Code/Feat/run_all.py` to generate features.
* run `python ./Code/Model/generate_best_single_model.py` to generate best single model submission.
* run `python ./Code/Model/generate_model_library.py` to generate model library.
* run `python ./Code/Model/generate_ensemble_submission.py` to generate submission via ensemble selection.
* if you don't want to run the code, just submit the file in `./Output/Subm`.