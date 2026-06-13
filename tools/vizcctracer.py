import os
import sys
import shutil
from multiprocessing import Process
import argparse
from pathlib import Path
import fileserver
import uiserver

def getargs():
    ap = argparse.ArgumentParser(
        description='''VizCCtracer''',
        epilog='''run vizcctracer at the root of your project folder 
        so that vizcctracer can find your source code'''
    )
    ap.add_argument(
        '-tf', '--trace_file',
        default=None,
        type=Path,
        help='path to the trace file (e.g. result.json or result.pftrace)'
    )
    ap.add_argument(
        '-p', '--port',
        default=None,
        type=int,
        help='preferred port for visualization, (use when you provide no trace file)'
    )
    args = ap.parse_args()
    checkargs(args)
    return args

def checkargs(args):
    if args.trace_file and args.port:
        print(f'Do not provide trace_file and port at the same time')
        print(f'If you want to use a custom port, provide the port and open the trace file inside ui')
        print(f'Otherwise provide the trace_file and the ui will open on http://localhost:10000')
        sys.exit(1)

    if not args.port and not args.trace_file:
        args.port = uiserver.find_free_port()
    if args.trace_file and args.trace_file.exists():
        args.trace_file = os.path.realpath(args.trace_file)

def symlink_or_copy_trace_to_ui_dir(trace_file):
    TRACE_DEST = os.path.join(uiserver.UI_PATH, uiserver.TRACE_SYMFILE)
    print(uiserver.UI_PATH)
    print(TRACE_DEST)
    try:
        if os.path.exists(TRACE_DEST):
            os.unlink(TRACE_DEST)
        os.symlink(trace_file, TRACE_DEST)
        print(f'Created symlink: {TRACE_DEST} -> {trace_file}')
    except OSError:
        shutil.copy2(trace_file, TRACE_DEST)
        print(f'Copied {trace_file} -> {TRACE_DEST}')

def main():
    args = getargs()
    if args.trace_file:
        symlink_or_copy_trace_to_ui_dir(args.trace_file)
    filesource_process = Process(target=fileserver.start_filesource_server)
    filesource_process.start()
    try:
        uiserver.start_ui(args.port)
    except KeyboardInterrupt:
        filesource_process.kill()
        print('\nShutting Down VizCCTracer..')
        sys.exit(0)

if __name__ == '__main__':
    main()
