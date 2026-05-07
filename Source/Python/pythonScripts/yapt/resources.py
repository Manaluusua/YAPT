from py_yapt import Renderer, ResourceUsageBits, ResourceDimension, VertexBufferLayout, MeshAttribute, ResourceFormat, AttributeSemanticName, Mesh
from yapt.conversions import ConvUtility
from yapt.mesh_utils import MeshUtility
from yapt.texture_loader_ktx import TextureLoader, KtxToolInvocationError
from pathlib import Path

import OpenImageIO as oiio
import imageio.v3 as iio

from pygltflib import GLTF2
import numpy as np

class Resources:
    def __init__(self, renderer):
        self._tex_loader = TextureLoader()
        self._renderer = renderer
        self._textures = {}
        self._meshes = {}

    def clear(self):
        self._textures = None
        self._meshes = None

    def load_texture_2D(self, tex_path, fmt, verbose = False):
        return self._load_texture_internal(tex_path, verbose, self._tex_loader.load_texture_2d, source=tex_path, vk_format=fmt)

    def load_texture_cube(self, tex_path, fmt, faces, verbose = False):
        return self._load_texture_internal(tex_path, verbose, self._tex_loader.load_texture_cube,faces=faces, vk_format=fmt)

    def load_texture_cube(self, tex_path, fmt, slices, verbose = False):
        return self._load_texture_internal(tex_path, verbose, self._tex_loader.load_texture_3d, slices=slices, vk_format=fmt)

    def load_meshes_from_path(self, mesh_path, verbose = False):
        path = Path(mesh_path)
        path_str = str(path)
        file_name = path.stem
        gltf = GLTF2().load(path_str)
        return self.load_meshes_from_gltf(gltf, path_str, verbose)

    def load_meshes_from_gltf(self, gltf, cache_name, verbose = False):

        if cache_name in self._meshes:
            print(f"found mesh {cache_name} from cache, reusing")
            return self._meshes[cache_name]

        # Extract individual meshes
        mesh_list = []

        for gltf_mesh in gltf.meshes:
            name = gltf_mesh.name #f"{file_name}_mesh_{mesh_index}"
            mesh = self._create_and_upload_from_gltf(gltf, name, gltf_mesh, verbose)
            mesh_list.append(mesh)

        self._meshes[cache_name] = mesh_list
        return mesh_list


    ###TEXTURES INTERNAL###
    def _load_texture_internal(self, tex_path, verbose, load_ktx_fn, **load_ktx_params):
        path = Path(tex_path)
        path_str = str(path)

        if path_str in self._textures:
            print(f"found texture {path_str} from cache, reusing")
            return self._textures[path_str]

        img_name = path.stem
        if(verbose):
            print(f"loading {path_str} as {img_name}")

        ktx_tex = None

        try:
            ktx_tex = load_ktx_fn(**load_ktx_params)
        except KtxToolInvocationError as e:
                print(f"retCode: {e.returncode}\nstderr: {e.stderr}cmd: {e.command}, ")

        if(ktx_tex == None):
            return None

        tex = self._create_and_upload_texture_ktx(path.name, ktx_tex, verbose)

        if(tex):
            self._textures[path_str] = tex

        return tex


    def _create_and_upload_texture_ktx(self, name, ktx_tex, verbose):
        width, height = ktx_tex.base_width, ktx_tex.base_height
        faces_slices = ktx_tex.num_faces * ktx_tex.base_depth
        layers = 1
        if hasattr(ktx_tex, "num_layers"):
            layers = ktx_tex.num_layers
        levels = ktx_tex.num_levels
        
        if verbose:
            print(f"texture dimension: w: {width}, h: {height}, faces_slices: {faces_slices}, layers: {layers}, levels: {levels}")

        dim = ConvUtility.ktx_to_yapt_dimension(ktx_tex, verbose)
        form = ConvUtility.vk_to_yapt_format(ktx_tex.vk_format, verbose)

        tex =  self._renderer.createTexture(name, dim, form, ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, width, height, levels, layers * faces_slices)
        data = ktx_tex.data()
        for level in range(0, levels):
            for layer in range(0, layers):
                for face_slice in range(0, faces_slices):
                    row_pitch = ktx_tex.row_pitch(level)
                    offset = ktx_tex.image_offset(level, layer, face_slice)
                    tex.upload(level, layer * face_slice, 1, 1, row_pitch, data, int(offset))

        return tex

    ###MESHES INTERNAL###

    def _get_component_count(self, accessor):
        if(accessor.type == 'SCALAR'):
            return 1;
        elif(accessor.type == 'VEC2'):
            return 2;
        elif(accessor.type == 'VEC3'):
            return 3;
        elif(accessor.type == 'VEC4'):
            return 4;
        elif(accessor.type == 'MAT2'):
            return 4;
        elif(accessor.type == 'MAT3'):
            return 9;
        elif(accessor.type == 'MAT4'):
            return 16;

    def _get_component_size_in_bytes(self, accessor):
        if(accessor.componentType == 5120):
            return 1;
        elif(accessor.componentType == 5121):
            return 1;
        elif(accessor.componentType == 5122):
            return 2;
        elif(accessor.componentType == 5123):
            return 2;
        elif(accessor.componentType == 5125):
            return 4;
        elif(accessor.componentType == 5126):
            return 4;

    def _get_component_numpy_dtype(self, accessor):
        if(accessor.componentType == 5120):
            return np.int8
        elif(accessor.componentType == 5121):
            return np.uint8
        elif(accessor.componentType == 5122):
            return np.int16
        elif(accessor.componentType == 5123):
            return np.uint16
        elif(accessor.componentType == 5125):
            return np.uint32;
        elif(accessor.componentType == 5126):
            return np.float32

    def _extract_data_from_accessor(self, gltf, accessor_index):
        accessor = gltf.accessors[accessor_index]

        buffer_view = gltf.bufferViews[accessor.bufferView]
        buff = gltf.buffers[buffer_view.buffer]

        data = gltf.get_data_from_buffer_uri(buff.uri)

        comp_size_bytes = self._get_component_size_in_bytes(accessor)
        component_count = self._get_component_count(accessor)
        entry_count = accessor.count

        start = buffer_view.byteOffset + accessor.byteOffset
        end = start + buffer_view.byteLength

        stride = comp_size_bytes

        if hasattr(buffer_view, 'byteStride') and buffer_view.byteStride != None:
            stride = buffer_view.byteStride

        data_array = np.ndarray(shape=(entry_count * component_count), dtype=self._get_component_numpy_dtype(accessor), buffer=data[start:end],  strides=(stride))
        
        if component_count > 1:
            data_array = data_array.reshape((-1, component_count))

        return (entry_count, component_count, comp_size_bytes, data_array)

    def _try_prim_array_merge(self, prim_data_arr, checkValidity = True):

        if prim_data_arr == None or len(prim_data_arr) == 0:
            return None

        if len(prim_data_arr) == 1:
            return prim_data_arr[0][3]

        if checkValidity:
            first = prim_data_arr[0]
            for data in prim_data_arr:

                if first[1] != data[1] or first[2] != data[2]:
                    print(f"ignoring primitive, unexpected data layout")
                    print(f"{first[1]}, {first[2]}")
                    print(f"{data[1]}, {data[2]}")
                    return None

        return np.concatenate([t[3] for t in prim_data_arr])
        
    def _create_and_upload_from_gltf(self, gltf, mesh_name, gltf_mesh, verbose = False):
        print(f"loading {mesh_name}")

        #go through each primitive and append the data (need to offset indices to get submesh ranges)
        #TODO: do this properly. Currently we extract all attributes to separate buffers, should accept the layout that is in the file or explicitly define it.

        faces_array =  []
        vertices_array = []
        normals_array = []
        tangents_array = []
        uvs_array = []
        colors_array = []

        for primitive in gltf_mesh.primitives:
            if primitive.indices is not None:
                faces = self._extract_data_from_accessor(gltf, primitive.indices)
                faces_array.append(faces)
        
            if primitive.attributes.POSITION is not None:
                vertices = self._extract_data_from_accessor(gltf, primitive.attributes.POSITION)
                vertices_array.append(vertices)
        
            if primitive.attributes.NORMAL is not None:
                normals = self._extract_data_from_accessor(gltf, primitive.attributes.NORMAL)
                normals_array.append(normals)
            
            if primitive.attributes.TANGENT is not None:
                tangents = self._extract_data_from_accessor(gltf, primitive.attributes.TANGENT)
                tangents_array.append(tangents)
            
            if primitive.attributes.TEXCOORD_0 is not None:
                uvs = self._extract_data_from_accessor(gltf, primitive.attributes.TEXCOORD_0)
                uvs_array.append(uvs)
                
            if primitive.attributes.COLOR_0 is not None:
                colors = self._extract_data_from_accessor(gltf, primitive.attributes.COLOR_0)
                colors_array.append(colors)

        faces = self._try_prim_array_merge(faces_array)
        vertices = self._try_prim_array_merge(vertices_array)
        normals = self._try_prim_array_merge(normals_array)
        tangents = self._try_prim_array_merge(tangents_array)
        uvs = self._try_prim_array_merge(uvs_array)
        colors = self._try_prim_array_merge(colors_array)
        
        if vertices is None:
            print(f"could not load mesh {mesh_name}, missing vertex positions")
            return None

        if normals is None:
            print(f"could not load mesh {mesh_name}, missing vertex normals")
            return None

        if tangents is None and uvs is not None:
            print("Warning: mesh {mesh_name} does not contain tangents. Recalculating them (this might be slow)")
            #tangents = MeshUtility.calculate_tangents(vertices, faces, uvs, normals) #TODO: need to handle submeshes, uncomment when done

        if tangents is None:
            print(f"could not load mesh {mesh_name}, missing vertex tangents (and no UVs to recalculate them)")
            return None

        layout = []
        buffers = []

        use16BitIndices = len(vertices) < 0xFFFF

        layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGB32_SFLOAT, AttributeSemanticName.POSITION, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
        buffers.append(self._create_and_upload_buffer(f"{mesh_name}_positions", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT, vertices, np.float32))

        if(normals is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGB32_SFLOAT, AttributeSemanticName.NORMAL, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_normals", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT, normals, np.float32))

        if(tangents is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGBA32_SFLOAT, AttributeSemanticName.TANGENT, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_tangents", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT, tangents, np.float32))

        if(uvs is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RG32_SFLOAT, AttributeSemanticName.TEXCOORD, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_texcoord0", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT, uvs, np.float32))

        if(colors is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGBA8_UINT, AttributeSemanticName.COLOR, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_colors", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT, colors, np.uint8))

        indices = self._create_and_upload_buffer(f"{mesh_name}_indices", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.INDEX_BUFFER | ResourceUsageBits.ACCELERATION_STRUCTURE_BUILD_INPUT, faces, np.uint16 if use16BitIndices else np.uint32)

        mesh = self._renderer.createMesh(mesh_name, layout, len(vertices), len(faces_array), use16BitIndices) #for now just use one submesh

        for i in range(len(buffers)):
            mesh.setVertexBuffer(i, buffers[i], 0)

        mesh.setIndexBuffer(indices, 0)

        vertex_offset = 0
        index_offset = 0
        submesh_index = 0
        for submesh in faces_array:
            index_count = submesh[0]

            boundsMin, boundsMax = Mesh.calculateBoundsFromVertexBuffer(vertices.ctypes.data, faces.ctypes.data, index_offset, index_count, vertex_offset, 3 * 4, use16BitIndices)

            mesh.setSubmesh(submesh_index, index_offset, index_count, vertex_offset, boundsMin, boundsMax)
            
            vertex_offset += vertices_array[submesh_index][0]
            index_offset += index_count
            submesh_index += 1

        return mesh

    def _create_and_upload_buffer(self, name, usage, data, forceType):

        if forceType != None:
            data = np.array(data, dtype=forceType)

        size = data.size * data.itemsize
        ptr = data.ctypes.data

        buff = self._renderer.createBuffer(name, usage, size)
        buff.upload(0, size, ptr, 0)
        return buff