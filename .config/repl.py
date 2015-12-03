#!/usr/bin/python

import re, sys

repl_dict = {
#repl_dict#
}

def repl_func(matchobj):
    key = matchobj.group(0)[1:-1]
    if key in repl_dict:
        return repl_dict[key]
    else:
        return matchobj.group(0)


print re.sub('@\w+@', repl_func, sys.stdin.read())

