import os

def reformatPath(path):
    if path.startswith('/'):
        return path

    cwd = os.getcwd()
    return os.path.realpath(os.path.join(cwd, path))


def getsource(path):
    if path == '': 
        return f'Empty Path.'
    
    path = reformatPath(path)
    if not os.path.isfile(path):
        return f'{path} file not found.'
    with open(path, 'r', encoding='utf-8') as fd:
        src = fd.read()
    return src
