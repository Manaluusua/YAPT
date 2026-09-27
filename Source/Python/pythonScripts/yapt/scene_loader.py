from py_yapt import Material, vec4, vec3, vec2
from yapt.utility import *
from PySide6.QtGui import QQuaternion, QVector3D
from pathlib import Path
from pygltflib import GLTF2
import math

class SceneLoader:
    def __init__(self, resources, scene, renderer):
        self._resources = resources
        self._scene = scene
        self._renderer = renderer
        self._res_paths = []
        self._objects = {}
        self._default_mat = None
        
    def load_scene_gltf(self, scene_path, verbose = False):
        path = Path(scene_path)
        path_str = str(path)
        gltf = GLTF2().load(path_str)
       
        self._gltf = gltf

        #meshes
        self._mesh_groups = self._resources.load_meshes_from_gltf(gltf, path_str, verbose)

        #textures
        self._textures = []
        for image in gltf.images:
            #TODO: add support for loading textures embedded to glb either with bufferview or data uri
            can_load_tex = False
            if hasattr(image, 'uri') and image.uri != None and not image.uri.startswith("data:"):
                can_load_tex = True

            if not can_load_tex:
                print(f"couldn't load texture image {image.name}, currently only external image files are supported")
                self._textures.append(None)
                continue

            image_path = str(path.parent) + "/" + image.uri
            image_name = image.name
            tex = self._load_texture(image_path, image_name)
            self._textures.append(tex)

        #materials
        self._materials = []
        for mat in gltf.materials:
            self._materials.append(self._load_material_gltf(mat))

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

    def _get_default_material(self):
        if self._default_mat == None:
            yapt_mat = self._renderer.createMaterial(f"default_mat") 
            yapt_mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
            yapt_mat.setSpecularAmount(0)
            self._default_mat = yapt_mat
        return self._default_mat

    def _set_texture(self, tex_def, setter_fn):
        tex_entry = self._gltf.textures[tex_def.index]
        if len(self._textures) <= tex_entry.source:
            print(f"couldn't set texture {tex_def}, texture array doesn't have that index. index {tex_entry.source}, array size {len(self._textures)}")
            return None
        tex = self._textures[tex_entry.source]
        if tex == None:
            print(f"couldn't set texture {tex_def}, texture was not succesfully loaded")
            return None

        setter_fn(tex, v2(1, 1))
        return tex

    def _load_material_gltf(self, mat):
        yapt_mat = self._renderer.createMaterial(f"{mat.name}_mat") 
        
        if getattr(mat, "emissiveFactor"):
            yapt_mat.setEmission(vec3(mat.emissiveFactor))

        if getattr(mat, "normalTexture"):
            self._set_texture(mat.normalTexture, yapt_mat.setNormalTexture)

        orm_tex = None

        if getattr(mat, "occlusionTexture"):
            orm_tex = self._set_texture(mat.occlusionTexture, yapt_mat.setORMTexture)

        if getattr(mat, "doubleSided"):
            yapt_mat.setTwoSided(mat.doubleSided)

        if getattr(mat, "pbrMetallicRoughness"):
            pbr_config = mat.pbrMetallicRoughness
            
            color = [1, 1, 1, 1]
            if getattr(pbr_config, "baseColorFactor"):
                color = pbr_config.baseColorFactor
            yapt_mat.setAlbedo(v3(color[0], color[1], color[2]))
            yapt_mat.setTransparency(color[3])


            if getattr(pbr_config, "baseColorTexture"):
                self._set_texture(pbr_config.baseColorTexture, yapt_mat.setAlbedoTexture)

            metallicFactor = 1
            if pbr_config.metallicFactor is not None:
                metallicFactor = pbr_config.metallicFactor
            yapt_mat.setMetalness(metallicFactor)

            roughnessFactor = 1
            if pbr_config.roughnessFactor is not None:
                roughnessFactor = pbr_config.roughnessFactor
            yapt_mat.setRoughness(roughnessFactor)

            if getattr(pbr_config, "metallicRoughnessTexture"):
                tex = self._set_texture(pbr_config.metallicRoughnessTexture, yapt_mat.setORMTexture)
                if(orm_tex != None and tex != orm_tex):
                    print(f"loading material {mat.name} which has separate occlusion and roughness and metalness textures. Yapt assumes ORM to be in one tex, using the RM texture for everything! ")
        
        #material extensions
        if getattr(mat, "extensions"):
            extensions = mat.extensions

            #volume
            if "KHR_materials_volume" in extensions:
                volume = extensions["KHR_materials_volume"]

                if("thicknessFactor" in volume):
                    if(volume["thicknessFactor"] == 0):
                        yapt_mat.setTwoSided(True)
                att_dist = -1
                att_col = [1, 1, 1]
                
                    
                if("attenuationDistance" in volume):
                    att_dist = volume["attenuationDistance"]
                if("attenuationColor" in volume):
                    att_col = volume["attenuationColor"]

                if(att_dist > 0 and (att_col[0] != 1 or att_col[1] != 1 or att_col[2] != 1)):
                    absorb = v3(-math.log(att_col[0]) / att_dist, -math.log(att_col[1]) / att_dist, -math.log(att_col[2]) / att_dist)
                    yapt_mat.setAbsorption(absorb)

            #iridiscence (assumed coating)
            if "KHR_materials_iridescence" in extensions:
                iridiscence = extensions["KHR_materials_iridescence"]
                coating_str = 0
                coating_ior = 1.3
                thickness_nm = 400
                if "iridescenceIor" in iridiscence:
                    coating_ior = iridiscence["iridescenceIor"]
                if "iridescenceFactor" in iridiscence:
                    coating_str = iridiscence["iridescenceFactor"]
                if "iridescenceThicknessMaximum" in iridiscence:
                    thickness_nm = iridiscence["iridescenceThicknessMaximum"]

                yapt_mat.setClearCoatIOR(coating_ior)
                yapt_mat.setClearCoatAmount(coating_str)
                yapt_mat.setThinFilmThicknessNM(thickness_nm)
            #ior
            if "KHR_materials_ior" in extensions:
                khr_ior = extensions["KHR_materials_ior"]
                ior = 1.5
                if "ior" in khr_ior:
                    ior = khr_ior["ior"]

                yapt_mat.setDielectricIOR(ior)


        return yapt_mat
    
    def _load_texture(self, path, name):
        #TODO: do properly: how to infer dimensions and formats?
        
        assumed_format = "R8G8B8A8_SRGB"
        if "normal" in name.casefold():
            assumed_format = "R8G8B8A8_UNORM"

        return self._resources.load_texture_2D(path, assumed_format)

    def _create_render_object(self, node):

        parent = self._create_scene_object(node)

        created_objects = []
        mesh_array, primitive_mapping = self._mesh_groups[node.mesh]

        for index, mesh in enumerate(mesh_array):
            obj = self._scene.create_render_object(f"{node.name}_{index}")
            obj.setMesh(mesh)
            created_objects.append(obj)
            obj.getTransform().setParent(parent.getTransform())

        obj_materials = []

        gltf_mesh = self._gltf.meshes[node.mesh]
        for primIndex in range(0, len(gltf_mesh.primitives)):
            prim = gltf_mesh.primitives[primIndex]
            if prim.material is not None:
                mat_ind = prim.material
                obj_materials.append(self._materials[mat_ind])
            else:
                obj_materials.append(self._get_default_material())

        for index, mat in enumerate(obj_materials):
            mesh_index, submesh_index = primitive_mapping[index]
            created_objects[mesh_index].setMaterial(mat, submesh_index)
        
        return parent

    def _create_scene_object(self, node):
        obj = self._scene.create_scene_object(node.name)
        return obj

    def _create_object(self, node):
        obj = None
        if self._has_transform(node):
            #if node has mesh and its succesfully loaded, create renderobject
            if(node.mesh is not None):
                mesh_group = self._mesh_groups[node.mesh]
                if mesh_group != None and len(mesh_group[0]) > 0:
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