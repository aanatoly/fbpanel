#!/usr/bin/python

import subprocess as sp
import re, tempfile
import shutil, os, sys

##################################################
# Loging
import logging
def _init_log():
    name = 'app'
    x = logging.getLogger(name)
    x.setLevel(logging.WARNING)
    # x.setLevel(logging.DEBUG)
    h = logging.StreamHandler()
    # f = logging.Formatter("%(name)s (%(funcName)s:%(lineno)d) :: %(message)s")
    f = logging.Formatter("%(message)s")
    h.setFormatter(f)
    x.addHandler(h)
    return x

x = _init_log()

##################################################
# Argument parsing
import argparse
p = argparse.ArgumentParser(
    description='Pack project source to tar archive.',
    epilog = 'Example: \n%(prog)s /tmp/proj-1.2.tar.bz2'
)

p.add_argument('-v', dest = 'verbose', action = 'store_true',
    default = False,
    help = 'Print more messages [%(default)s]')
p.add_argument('-d', '--debug', dest = 'debug', action = 'store_true',
    default = False,
    help = 'Print debug messages [%(default)s]')
p.add_argument('tar', nargs=1,
    help = 'tar file to create')

args = p.parse_args()
if args.verbose:
    x.setLevel(logging.INFO)
if args.debug:
    x.setLevel(logging.DEBUG)

##################################################
# main
def my_check_output(*popenargs, **kwargs):  
    if 'stdout' in kwargs:
        raise ValueError('stdout argument not allowed, it will be overridden.')
    cmd = kwargs.get("args")
    if cmd is None:
        cmd = popenargs[0]
    x.debug("exec: %s", cmd)
    process = sp.Popen(stdout=sp.PIPE, *popenargs, **kwargs)
    output, unused_err = process.communicate()
    retcode = process.poll()
    if retcode:
        raise sp.CalledProcessError(retcode, cmd, output=output)
    return output

def svn_is_used():
    f = open('/dev/null', 'w')
    p = sp.Popen('svn info'.split(), stdout = f, stderr = f)
    p.wait()
    return p.returncode == 0


def svn_get_file_list():
    text = my_check_output('svn info -R'.split())
    files = []
    tre = '(?m)^Path: (?P<path>.*)$(.|\s)*?^Node Kind: (?P<type>.*)$'
    for m in re.finditer(tre, text):
        if m.group('type') == 'file':
            files.append(m.group('path'))
    return files

def git_is_used():
    f = open('/dev/null', 'w')
    p = sp.Popen('git status'.split(), stdout = f, stderr = f)
    p.wait()
    return p.returncode == 0


def git_get_file_list():
    text = my_check_output('git ls-tree --name-only -r HEAD'.split())
    files = text.split('\n')
    return files

def none_is_used():
    f = open('/dev/null', 'w')
    p = sp.Popen('make -n distclean'.split(), stdout = f, stderr = f)
    p.wait()
    return p.returncode == 0
    
def none_get_file_list():
    f = open('/dev/null', 'w')
    p = sp.Popen('make distclean'.split(), stdout = f, stderr = f)
    p.wait()
    if p.returncode:
        return []
    files = []
    for (dirpath, dirnames, filenames) in os.walk('.'):
        if dirpath.startswith('./'):
            dirpath = dirpath[2:]
        elif dirpath == '.':
            dirpath = ''
        filenames = [ os.path.join(dirpath, f) for f in filenames ]
        files.extend(filenames)
    return files

vcs = [
    {
        'name' : 'svn',
        'is_used' : svn_is_used,
        'get_file_list' : svn_get_file_list
    },
    {
        'name' : 'git',
        'is_used' : git_is_used,
        'get_file_list' : git_get_file_list
    },
    
    # must be last entry
    {
        'name' : 'none',
        'is_used' : none_is_used,
        'get_file_list' : none_get_file_list
    }
]

# Get lists of files. If VCS is used, then lists only controlled files
# otherwise lists all files
def get_file_list():
    files = []
    for v in vcs:
        ret = v['is_used']()
        x.debug('trying %s, ret %d', v['name'], ret)
        if ret:
            x.info('VCS: %s', v['name'])
            files = v['get_file_list']()
            # remove duplicates, if any
            files = list(set(files))
            files.remove('')
            files.sort()
            x.debug('File list: %s', files)
            if not files:
                x.error('No files. Aborting')
                exit(2)
            return files
    x.error('No files. Aborting')
    exit(2)

tar = args.tar[0]
path, name = os.path.split(tar)
nv = re.sub('.tar.*', '', name)
x.debug('tar %s, nv %s', tar, nv)
# XXX: get compresion type from extension

files = [ nv + '/' + f for f in get_file_list() ]
files = '\n'.join(files)
x.debug('%s', files)
tdir = tempfile.mkdtemp()
odir = os.path.abspath('.')
os.chdir(tdir)
os.symlink(odir, nv)
open('filelist', 'w').write(files)
cmd = 'tar hjcf %s --files-from filelist' % (tar,)
x.debug('exec: %s', cmd)
p = sp.Popen(cmd.split())
p.wait()
if p.returncode:
    x.error('Aborting')
else:
    x.info('Tar: %s', tar)

shutil.rmtree(tdir)
