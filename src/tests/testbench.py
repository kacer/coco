#!/usr/bin/env python

from __future__ import print_function


import subprocess
import argparse
import tempfile
import operator
import difflib
import sys
import os
import re


RE_COMMENT = r'\s*/\*\*(.+?)\*/'
RE_FLAG = r'Compile with'


def green(s):
    return '\033[42m' + s + '\033[0m'


def yellow(s):
    return '\033[43m' + s + '\033[0m'


def red(s):
    return '\033[41m\033[37m' + s + '\033[0m'


def parse_makefile(mkfile):
    flags = []

    with open(mkfile, 'rt') as fp:
        for line in fp:
            line = line.strip()

            if not line:
                continue

            elif line.startswith('CFLAGS='):
                flags.extend(re.split('[\s*,]', line[7:]))

            elif line.startswith('SOURCES='):
                func = lambda s: '../' + s
                sources = (
                    '../' + f
                    for f in re.split('[\s*,]', line[8:])
                    if f != 'main.c'
                )
                flags.extend(sources)

            elif line.startswith('LIBS='):
                flags.extend(re.split('[\s*,]', line[5:]))

    return flags


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('testfiles', metavar='TEST', type=str, nargs='+',
                        help='Test source (*.c) file')
    parser.add_argument('--novalgrind', action='store_false',
                        help='Do not use valgrind')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Do not use valgrind')
    parser.add_argument('--stop', '-s', action='store_true',
                        help='Stop on first failure')
    args = parser.parse_args()
    verbose = args.verbose
    stop_on_failure = args.stop

    gcc_flags = parse_makefile('../Makefile')

    for testfile in args.testfiles:

        if verbose:
            print(yellow('Test %s' % (testfile)))

        if not os.path.isfile(testfile):
            print(red("Skipping non-existing file %s" % (testfile,)))
            continue

        with open(testfile, 'rt') as fp:
            content = fp.read()

            # grab first comment
            m = re.match(RE_COMMENT, content, re.I + re.U + re.S)
            if not m:
                print(red("File %s does not contain header" % (testfile,)))
                if stop_on_failure:
                    return
                else:
                    continue

            init_comment = m.group(1)
            lines = map(str.strip, re.split(r'\s*\n\s*\*\s', init_comment))

            description = []
            flags = []
            use_diff = True

            for line in lines:
                if line.startswith('Compile with'):
                    flags.extend(re.split('[\s*,]', line[13:]))

                elif line.startswith('"Stand-alone" test executable'):
                    use_diff = False

                else:
                    description.append(line)

        # compile

        try:
            binary = tempfile.NamedTemporaryFile(prefix='cocotest_',
                                                 delete=False)
            binary.close()

            args = [testfile] + gcc_flags + flags + ['-o', binary.name]

            if verbose:
                print('Compiling %s to %s' % (testfile, binary.name))
                print('GCC command:\ngcc', ' '.join(args))

            p = subprocess.Popen(["gcc"] + args,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            out, err = p.communicate()
            if err or p.returncode != 0:
                print(red('Test %s failed to compile' % (testfile)))
                print(err, file=sys.stderr)
                if stop_on_failure:
                    return
                else:
                    continue

            # run

            if verbose:
                print('Running %s' % (binary.name,))
            p = subprocess.Popen([binary.name],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            out, err = p.communicate()

            if err or p.returncode != 0:
                print(red('Test %s failed (stderr non-empty or retval != 0)' % (testfile)))
                print(err, file=sys.stderr)
                if stop_on_failure:
                    return
                else:
                    continue

            if use_diff:
                with open(re.sub('.c$', '.out', testfile), 'rt') as fp:
                    reference_out = fp.read()

                result = list(difflib.unified_diff(
                    reference_out.splitlines(1), out.splitlines(1)
                ))
                if len(result):
                    print(red('Test %s failed (diff mismatch)' % (testfile)))
                    #print(''.join(result), file=sys.stderr)
                    if stop_on_failure:
                        return
                    else:
                        continue

            print(green('Test %s OK' % (testfile)))

        finally:
            os.unlink(binary.name)


if __name__ == '__main__':
    main()
