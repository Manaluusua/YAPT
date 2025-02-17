import PyYAPT

def main():
    rend = PyYAPT.createRenderer()
    PyYAPT.destroyRenderer(rend)
    print("all done")
     
if __name__=="__main__":
    main()