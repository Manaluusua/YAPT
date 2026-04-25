from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
from pathlib import Path
from pygltflib import GLTF2

class SceneLoader:
    def __init__(self, resources):
        self._resources = resources
        self._objects = {}
        self._res_paths = []

    def add_resource_search_paths(self, paths):
        self._res_paths.append(paths)

    def loadSceneGLTF(self, scene_path, verbose = False):
        path = Path(scene_path)
        path_str = str(path)
        gltf = GLTF2().load(path_str)
       
        meshes = self._resources.load_meshes_from_gltf(gltf, path_str, verbose)

        gltf_scene = gltf.scenes[gltf.scene]

        for node_index in gltf_scene.nodes:
            node = gltf.nodes[node_index]

            