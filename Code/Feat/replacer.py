
"""
__file__

    replacer.py

__description__

    This file provides functions to perform synonym & antonym replacement.
    Such functions are adopted from "Python Text Processing with NLTK 2.0 Cookbook," Chapter 2, Page 39-43.

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import re
import csv
import cPickle
import numpy as np
import pandas as pd
from nltk.corpus import wordnet


##########################
## Synonym Replacement ##
##########################
class WordReplacer(object):
    def __init__(self, word_map):
        self.word_map = word_map
    def replace(self, word):
        return [self.word_map.get(w, w) for w in word]
    
    
class CsvWordReplacer(WordReplacer):
    def __init__(self, fname):
        word_map = {}
        for line in csv.reader(open(fname)):
            word, syn = line
            if word.startswith("#"):
                continue
            word_map[word] = syn
        super(CsvWordReplacer, self).__init__(word_map)


##########################
## Antonym Replacement ##
##########################
class AntonymReplacer(object):
    def replace(self, word, pos=None):
        antonyms = set()
        for syn in wordnet.synsets(word, pos=pos):
            for lemma in syn.lemmas:
                for antonym in lemma.antonyms():
                    antonyms.add(antonym.name)
        if len(antonyms) == 1:
            return antonyms.pop()
        else:
            return None
            
    def replace_negations(self, sent):
        i, l = 0, len(sent)
        words = []
        while i < l:
            word = sent[i]
            if word == 'not' and i+1 < l:
                ant = self.replace(sent[i+1])
                if ant:
                    words.append(ant)
                    i += 2
                    continue
            words.append(word)
            i += 1
        return words