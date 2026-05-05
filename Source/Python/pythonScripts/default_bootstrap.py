import os
import sys

def append_from_path_to_syspath():
    
    path_set = set(sys.path)
    for path in os.environ['PATH'].split(os.pathsep):
        path_set.add(path)
    path_set.add(os.getcwd())
    path_set.add(os.getcwd() + "/pythonScripts")
    sys.path = list(path_set)

def print_dll_search_paths():
    print("\nDirectories in sys.path:")
    for path in sys.path:
        print(path)


append_from_path_to_syspath()
#print_dll_search_paths()
print("Python version:", sys.version)
from yapt.app import Application


def main():
    
    app = Application()
    app.execute()
    app.shutdown()
    print(f"all done")
     
if __name__=="__main__":
    main()