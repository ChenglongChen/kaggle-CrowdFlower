

"""
__file__

    ngram.py

__description__

    This file provides functions to compute n-gram & n-term.

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

def getUnigram(words):
    """
        Input: a list of words, e.g., ['I', 'am', 'Denny']
        Output: a list of unigram
    """
    assert type(words) == list
    return words
    
def getBigram(words, join_string, skip=0):
	"""
	   Input: a list of words, e.g., ['I', 'am', 'Denny']
	   Output: a list of bigram, e.g., ['I_am', 'am_Denny']
	   I use _ as join_string for this example.
	"""
	assert type(words) == list
	L = len(words)
	if L > 1:
		lst = []
		for i in range(L-1):
			for k in range(1,skip+2):
				if i+k < L:
					lst.append( join_string.join([words[i], words[i+k]]) )
	else:
		# set it as unigram
		lst = getUnigram(words)
	return lst
    
def getTrigram(words, join_string, skip=0):
	"""
	   Input: a list of words, e.g., ['I', 'am', 'Denny']
	   Output: a list of trigram, e.g., ['I_am_Denny']
	   I use _ as join_string for this example.
	"""
	assert type(words) == list
	L = len(words)
	if L > 2:
		lst = []
		for i in range(L-2):
			for k1 in range(1,skip+2):
				for k2 in range(1,skip+2):
					if i+k1 < L and i+k1+k2 < L:
						lst.append( join_string.join([words[i], words[i+k1], words[i+k1+k2]]) )
	else:
		# set it as bigram
		lst = getBigram(words, join_string, skip)
	return lst
    
def getFourgram(words, join_string):
    """
        Input: a list of words, e.g., ['I', 'am', 'Denny', 'boy']
        Output: a list of trigram, e.g., ['I_am_Denny_boy']
        I use _ as join_string for this example.
    """
    assert type(words) == list
    L = len(words)
    if L > 3:
        lst = []
        for i in xrange(L-3):
            lst.append( join_string.join([words[i], words[i+1], words[i+2], words[i+3]]) )
    else:
        # set it as bigram
        lst = getTrigram(words, join_string)
    return lst

def getBiterm(words, join_string):
    """
        Input: a list of words, e.g., ['I', 'am', 'Denny', 'boy']
        Output: a list of biterm, e.g., ['I_am', 'I_Denny', 'I_boy', 'am_Denny', 'am_boy', 'Denny_boy']
        I use _ as join_string for this example.
    """
    assert type(words) == list
    L = len(words)
    if L > 1:
        lst = []
        for i in range(L-1):
            for j in range(i+1,L):
                lst.append( join_string.join([words[i], words[j]]) )
    else:
        # set it as unigram
        lst = getUnigram(words)
    return lst
    
def getTriterm(words, join_string):
    """
        Input: a list of words, e.g., ['I', 'am', 'Denny']
        Output: a list of triterm, e.g., ['I_am_Denny', 'I_Denny_am', 'am_I_Denny',
        'am_Denny_I', 'Denny_I_am', 'Denny_am_I']
        I use _ as join_string for this example.
    """
    assert type(words) == list
    L = len(words)
    if L > 2:
        lst = []
        for i in xrange(L-2):
            for j in xrange(i+1,L-1):
                for k in xrange(j+1,L):
                    lst.append( join_string.join([words[i], words[j], words[k]]) )
    else:
        # set it as biterm
        lst = getBiterm(words, join_string)
    return lst