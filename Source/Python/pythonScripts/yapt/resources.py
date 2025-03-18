from py_yapt import Renderer, ResourceUsageBits, ResourceDimension, VertexBufferLayout, MeshAttribute, ResourceFormat, AttributeSemanticName
from yapt.conversions import ConvUtility
from yapt.mesh_utils import MeshUtility
from pathlib import Path

import OpenImageIO as oiio
import imageio.v3 as iio

import trimesh
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

        scene = trimesh.load(path_str, force="scene")

        if not isinstance(scene, trimesh.Scene):
            print("Loaded file is not a scene, treating as a single mesh.")
            scene = trimesh.Scene([scene])

        # Extract individual meshes
        mesh_list = []
        for name, geo in scene.geometry.items():
            mesh = self._create_and_upload_from_trimesh(name, geo, verbose)
            self._meshes[name] = mesh
            mesh_list.append(mesh)

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

    

    def _create_and_upload_from_trimesh(self, mesh_name, mesh, verbose = False):

        vertices = mesh.vertices  # (N, 3) array of vertex positions
    
        # Face indices
        faces = mesh.faces  # (M, 3) array of triangle vertex indices

        # Normals (if available)
        normals = mesh.vertex_normals if hasattr(mesh, 'vertex_normals') else None
    

        # Texture coordinates (if available)
        uvs = mesh.visual.uv if mesh.visual and hasattr(mesh.visual, 'uv') else None

        hasUVs = uvs is not None

        #if not hasUVs:
        #   uvs = np.zeros((len(vertices), 2))
        #   print(f"mesh {mesh_name} does not have uv coordinates. Filling with 0.")

        tangents = None
        if hasattr(mesh.metadata, 'tangents'):
            tangents = mesh.metadata['tangents']
        #else:
        #    if hasUVs and normals is not None:
        #        tangents = MeshUtility.calculate_tangents(vertices, faces, uvs, normals)
        #        print(f"mesh {mesh_name} doesn't have tangents, calculating with UVs")
        #    else:
        #        tangents = MeshUtility.calculate_arbitrary_tangents(normals)
        #        print(f"mesh {mesh_name} doesn't have tangents nor uvs, calculating arbitrary tangents")
        #

        # Colors (if available)
        colors = mesh.visual.vertex_colors if mesh.visual and hasattr(mesh.visual, 'vertex_colors') else None

        layout = []
        buffers = []

        use16BitIndices = len(vertices) < 0xFFFF

        layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGB32_SFLOAT, AttributeSemanticName.POSITION, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
        buffers.append(self._create_and_upload_buffer(f"{mesh_name}_positions", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER, vertices, np.float32))

        if(normals is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGB32_SFLOAT, AttributeSemanticName.NORMAL, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_normals", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER, normals, np.float32))

        if(tangents is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGBA32_SFLOAT, AttributeSemanticName.TANGENT, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_tangents", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER, tangents, np.float32))

        if(uvs is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RG32_SFLOAT, AttributeSemanticName.TEXCOORD, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_texcoord0", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER, uvs, np.float32))

        if(colors is not None):
            layout.append(VertexBufferLayout([MeshAttribute(ResourceFormat.RGBA8_UINT, AttributeSemanticName.COLOR, 0)], VertexBufferLayout.STRIDE_TIGHTLY_PACKED))
            buffers.append(self._create_and_upload_buffer(f"{mesh_name}_colors", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.VERTEX_BUFFER, colors, np.uint8))

        indices = self._create_and_upload_buffer(f"{mesh_name}_indices", ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.INDEX_BUFFER, faces, np.uint16 if use16BitIndices else np.uint32)

        mesh = self._renderer.createMesh(mesh_name, layout, len(vertices), use16BitIndices)

        for i in range(len(buffers)):
            mesh.setVertexBuffer(i, buffers[i], 0)

        mesh.setIndexBuffer(indices, 0, int(faces.size / 3))

        return mesh

    def _create_and_upload_buffer(self, name, usage, data, forceType):

        if forceType != None:
            data = np.array(data, dtype=forceType)

        size = data.size * data.itemsize
        ptr = data.ctypes.data

        buff = self._renderer.createBuffer(name, usage, size)
        buff.upload(0, size, ptr, 0)
        return buff