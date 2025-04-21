from py_yapt import Renderer, ResourceUsageBits, ResourceDimension, VertexBufferLayout, MeshAttribute, ResourceFormat, AttributeSemanticName
from yapt.conversions import ConvUtility
from yapt.mesh_utils import MeshUtility
from pathlib import Path

import OpenImageIO as oiio
import imageio.v3 as iio

from pygltflib import GLTF2
import numpy as np

class Resources:
    def __init__(self, renderer):
        self._renderer = renderer
        self._textures = {}
        self._meshes = {}

    def clear(self):
        self._textures = None
        self._meshes = None

    def load_texture_from_path(self, tex_path, verbose = False):
        path = Path(tex_path)
        path_str = str(path)
        img_name = path.stem
        if(verbose):
            print(f"loading {path_str} as {img_name}")

        tex = self.__load_tex_with_oiio(path_str, img_name, verbose)
        #tex = self.__load_tex_with_iio(path_str, img_name)
        if(tex):
            self._textures[img_name] = self.__load_tex_with_oiio(path_str, img_name, verbose)
        return tex

    def load_meshes_from_path(self, mesh_path, verbose = False):
        path = Path(mesh_path)
        path_str = str(path)
        file_name = path.stem

        gltf = GLTF2().load(path_str)

        # Extract individual meshes
        mesh_list = []
        mesh_index = 0
        for gltf_mesh in gltf.meshes:
            for primitive in gltf_mesh.primitives:
                name = f"{file_name}_mesh_{mesh_index}"
                mesh = self._create_and_upload_from_gltf(gltf, name, primitive, verbose)
                if mesh is not None:
                    self._meshes[name] = mesh
                    mesh_list.append(mesh)
                    ++mesh_index

        return mesh_list

    ###TEXTURES INTERNAL###
    def __load_tex_with_oiio(self, path_str, img_name, verbose = False):
        image = oiio.ImageInput.open(path_str)

        if image == None:
            print(f"Failed to open image {path_str}")
            return;

        spec = image.spec()
        tex = None
        try:
            width, height, depth = spec.width, spec.height, spec.depth
            mips = 1 #TODO support
            dim = ConvUtility.oiio_spec_to_dimension(spec, verbose)
            form = ConvUtility.oiio_spec_to_format(spec, verbose)
            row_pitch = spec.scanline_bytes() 
            
            if(dim == ResourceDimension.TEXTURE_CUBEMAP):
                side_w, side_h = width, int(height/6)
                tex =  self._renderer.createTexture(img_name, dim, form, ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, side_w, side_h, mips, 6)
                data = image.read_image(format=oiio.UNKNOWN)
                bpp = spec.pixel_bytes()
                slice_size = side_w * side_h * bpp
                for i in range(6):
                    ptr = data.ctypes.data
                    tex.upload(0, i, mips, 1, row_pitch, ptr, int(slice_size * i))
            else:
                
                tex =  self._renderer.createTexture(img_name, dim, form, ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, width, height, mips, depth)
                data = image.read_image(format=oiio.UNKNOWN)
                ptr = data.ctypes.data
                tex.upload(0, 0, mips, depth, row_pitch, ptr)

        except Exception as e:
            print(f"failed to load {path_str}, {e}")
            return None
        finally:
            image.close()
        return tex

    def __load_tex_with_iio(self, path_str, img_name, verbose = False):
        image = iio.imread(path_str)
        props = iio.improps(path_str)
        meta = iio.immeta(path_str)
        print(props)
        print(meta)
        print(props.shape)
        print(props.dtype)

        width, height = image.shape[:2]
        dim = ConvUtility.iio_image_props_to_dimension(props)
        form = ConvUtility.iio_image_props_to_format(props)
        

        return self._renderer.createTexture(img_name, dim, form, ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, width, height, 1, 1)

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

        return data_array

    def _create_and_upload_from_gltf(self, gltf, mesh_name, primitive, verbose = False):
        print(f"loading {mesh_name}")
        # Extract indices
        faces = None
        if primitive.indices is not None:
            faces = self._extract_data_from_accessor(gltf, primitive.indices)
        
        # Extract vertices, normals, tangents, and uvs
        vertices = None
        normals = None
        tangents = None
        uvs = None
        colors = None
        
        #TODO: do this properly. Currently we extract all attributes to separate buffers, should accept the layout that is in the file or explicitly define it.

        if primitive.attributes.POSITION is not None:
            vertices = self._extract_data_from_accessor(gltf, primitive.attributes.POSITION)
        
        if primitive.attributes.NORMAL is not None:
            normals = self._extract_data_from_accessor(gltf, primitive.attributes.NORMAL)
        
        if primitive.attributes.TANGENT is not None:
            tangents = self._extract_data_from_accessor(gltf, primitive.attributes.TANGENT)
        
        if primitive.attributes.TEXCOORD_0 is not None:
            uvs = self._extract_data_from_accessor(gltf, primitive.attributes.TEXCOORD_0)
            
        if primitive.attributes.COLOR_0 is not None:
            colors = self._extract_data_from_accessor(gltf, primitive.attributes.COLOR_0)

        if vertices is None:
            print(f"could not load mesh {mesh_name}, missing vertex positions")
            return None

        if normals is None:
            print(f"could not load mesh {mesh_name}, missing vertex normals")
            return None

        if tangents is None and uvs is not None:
            print("Warning: mesh {mesh_name} does not contain tangents. Recalculating them (this might be slow)")
            tangents = MeshUtility.calculate_tangents(vertices, faces, uvs, normals)

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

        mesh = self._renderer.createMesh(mesh_name, layout, len(vertices), 1, use16BitIndices) #for now just use one submesh

        for i in range(len(buffers)):
            mesh.setVertexBuffer(i, buffers[i], 0)

        mesh.setIndexBuffer(indices, 0)
        mesh.setSubmesh(0, 0, faces.size)

        return mesh

    def _create_and_upload_buffer(self, name, usage, data, forceType):

        if forceType != None:
            data = np.array(data, dtype=forceType)

        size = data.size * data.itemsize
        ptr = data.ctypes.data

        buff = self._renderer.createBuffer(name, usage, size)
        buff.upload(0, size, ptr, 0)
        return buff