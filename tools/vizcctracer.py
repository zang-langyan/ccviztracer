import os
import sys
import shutil
import argparse
from pathlib import Path
import uiserver

def getargs():
    ap = argparse.ArgumentParser(
        description='''VizCCtracer''',
        epilog='''IMPORTANT: run vizcctracer at the root of your project folder 
        so that vizcctracer can find your source code'''
    )
    ap.add_argument(
        '-tf', '--trace_file',
        default=None,
        type=Path,
        help='''path to the trace file (e.g. result.json or result.pftrace)
        , you can also skip this and open the trace file within the ui'''
    )
    ap.add_argument(
        '-p', '--port',
        default=None,
        type=int,
        help='port preferred, a random one will be selected if not provided'
    )
    ap.add_argument(
        '-d', '--debug',
        default=False,
        action="store_true",
        help=argparse.SUPPRESS 
    )
    args = ap.parse_args()
    checkargs(args)
    return args

def checkargs(args):
    if args.trace_file:
        if not args.trace_file.exists() or not args.trace_file.is_file():
            print(f'Trace File: \'{args.trace_file}\' Not Found')
            sys.exit(-1)
        args.trace_file = os.path.realpath(args.trace_file)

def main():
    args = getargs()
    try:
        uiserver.start_ui(args.port, args.trace_file, args.debug)
    except KeyboardInterrupt:
        print('\nShutting Down VizCCTracer..')
        sys.exit(0)


if __name__ == '__main__':
    main()
