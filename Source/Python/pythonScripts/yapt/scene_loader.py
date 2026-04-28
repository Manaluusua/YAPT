from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
from pathlib import Path
from pygltflib import GLTF2

class SceneLoader:
    def __init__(self, resources):
        self._resources = resources
        self._res_paths = []
        self_objects = {}
        

    def add_resource_search_paths(self, paths):
        self._res_paths.append(paths)

    def loadSceneGLTF(self, scene_path, verbose = False):
        path = Path(scene_path)
        path_str = str(path)
        gltf = GLTF2().load(path_str)
       
        meshes = self._resources.load_meshes_from_gltf(gltf, path_str, verbose)

        loaded_objects = [None] * len(gltf_scene.nodes)

        gltf_scene = gltf.scenes[gltf.scene]

        #create nodes
        for node_index in gltf_scene.nodes:
            node = gltf.nodes[node_index]
            obj = self._createObject(node)
            if(obj != None):
                loaded_objects[node_index] = obj

        #parenting
        for node_index in gltf_scene.nodes:
            node = gltf.nodes[node_index]
            if(node.children != None):
                parent = loaded_objects[node_index]
                for child in node.children:
                    loaded_objects[child].setParent(parent)
            
    def _createObject(self, node):
        pass