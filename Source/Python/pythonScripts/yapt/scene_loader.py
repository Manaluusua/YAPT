from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
from pathlib import Path
from pygltflib import GLTF2

class SceneLoader:
    def __init__(self, resources, scene, renderer):
        self._resources = resources
        self._scene = scene
        self._renderer = renderer
        self._res_paths = []
        self_objects = {}
        

    def add_resource_search_paths(self, paths):
        self._res_paths.append(paths)

    def load_scene_gltf(self, scene_path, verbose = False):
        path = Path(scene_path)
        path_str = str(path)
        gltf = GLTF2().load(path_str)
       
        self._meshes = self._resources.load_meshes_from_gltf(gltf, path_str, verbose)

        gltf_scene = gltf.scenes[gltf.scene]
        loaded_objects = [None] * len(gltf.nodes)

        #create nodes
        for node_index in gltf_scene.nodes:
            node = gltf.nodes[node_index]
            obj = self._create_object(node)
            if(obj != None):
                loaded_objects[node_index] = obj

        #parenting
        for node_index in gltf_scene.nodes:
            node = gltf.nodes[node_index]
            if(node.children != None):
                parent = loaded_objects[node_index]
                if(parent == None):
                    print(f"parent node {node.name} is not loaded")
                    continue
                for child in node.children:
                    obj = loaded_objects[child]
                    if(obj == None):
                        print(f"child node {child} is not loaded for parent {node.name}")
                        continue
                    loaded_objects[child].setParent(parent)
            
    def _create_render_object(self, node):
        obj = self._scene.create_render_object(node.name)
        obj.setMesh(self._meshes[node.mesh])
        ##placeholder mat
        mat = self._renderer.createMaterial(f"{node.name}_mat") 
        mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
        mat.setRoughness(0.2)

        obj.setMaterial(mat, 0)
        return obj

    def _create_scene_object(self, node):
        obj = self._scene.create_scene_object(node.name)
        return obj

    def _create_object(self, node):
        obj = None
        if self._has_transform(node):
            #if node has mesh and its succesfully loaded, create renderobject
            if(node.mesh):
                if self._meshes[node.mesh] != None:
                    obj = self._create_render_object(node)
                else:
                    print(f"no mesh for node {node.name}, likely mesh failed to load")
            #otherwise just a scene node for transform parenting
            if(obj == None):
                obj = self._create_scene_object(node)
            self._setTransform(obj, node)
            return obj
        else:
            return self._create_scene_object(node)

    def _setTransform(self, obj, node):
        if(node.matrix):
            raise Exception("TODO")
        else:
            if(node.translation):
                obj.getTransform().setTranslation(vec3(node.translation))
            if(node.scale):
                obj.getTransform().setScale(vec3(node.scale))
            if(node.rotation):
                obj.getTransform().setOrientation(vec4(node.rotation))

    def _has_transform(self, node):
        return True