from py_yapt import Renderer, ResourceUsageBits, ResourceDimension, VertexBufferLayout, MeshAttribute, ResourceFormat, AttributeSemanticName, Mesh
from yapt.conversions import ConvUtility
from yapt.mesh_utils import MeshUtility, MeshLoader
from pathlib import Path
from yapt.texture_loader_ktx import TextureLoader, KtxToolInvocationError
from yapt.settings import resolve_asset_path
from cffi import FFI
ffi = FFI()


from pygltflib import GLTF2
import numpy as np

class Resources:
    def __init__(self, renderer):
        self._tex_loader = TextureLoader()
        self._mesh_loader = MeshLoader(renderer)
        self._renderer = renderer
        self._textures = {}
        self._texture_min_alpha = {}
        self._meshes = {}

    def clear(self):
        self._textures = None
        self._meshes = None

    #relative paths are resolved against ASSET_PATH (see yapt/settings.py), absolute ones are used as is
    def load_texture_2D(self, tex_path, fmt, verbose = False):
        tex_path = resolve_asset_path(tex_path)
        return self._load_texture_internal(tex_path, verbose, self._tex_loader.load_texture_2d, source=tex_path, vk_format=fmt)

    #smallest alpha (0..1) in the top mip of a loaded RGBA8 2D texture, None if unknown (other formats / not loaded)
    def get_texture_min_alpha(self, tex_path):
        return self._texture_min_alpha.get(str(Path(resolve_asset_path(tex_path))))
 
    def load_texture_cube(self, tex_path, fmt, faces, verbose = False):
        tex_path = resolve_asset_path(tex_path)
        return self._load_texture_internal(tex_path, verbose, self._tex_loader.load_texture_cube,faces=faces, vk_format=fmt)
 
    def load_texture_cube(self, tex_path, fmt, slices, verbose = False):
        tex_path = resolve_asset_path(tex_path)
        return self._load_texture_internal(tex_path, verbose, self._tex_loader.load_texture_3d, slices=slices, vk_format=fmt)

    def load_meshes_from_path(self, mesh_path, verbose = False):
        path = Path(resolve_asset_path(mesh_path))
        path_str = str(path)
        file_name = path.stem
        gltf = GLTF2().load(path_str)
        return self.load_meshes_from_gltf(gltf, path_str, verbose)

    def load_meshes_from_gltf(self, gltf, cache_name, verbose = False):

        if cache_name in self._meshes:
            print(f"found mesh {cache_name} from cache, reusing")
            return self._meshes[cache_name]

        # One entry per gltf mesh: (meshes, primitive_map) where primitive_map[i]
        # is (mesh_index, submesh_index) for gltf_mesh.primitives[i] or None.
        results = []
        cache = {}
        for gltf_mesh in gltf.meshes:
            name = gltf_mesh.name
            meshes, prim_map = self._mesh_loader.create_and_upload_from_gltf(gltf, name, gltf_mesh, cache, verbose, optimize_layout = True)
            results.append((meshes, prim_map))

        self._meshes[cache_name] = results

        return results

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
            self._store_min_alpha(path_str, ktx_tex)
        
        return tex
 
 
    def _store_min_alpha(self, path_str, ktx_tex):
        #lets alpha testing be skipped for textures that are opaque anyway
        if ktx_tex.vk_format.name not in ("VK_FORMAT_R8G8B8A8_SRGB", "VK_FORMAT_R8G8B8A8_UNORM") or ktx_tex.num_faces * ktx_tex.base_depth != 1:
            return
        texel_count = ktx_tex.base_width * ktx_tex.base_height
        data = np.frombuffer(ktx_tex.data(), dtype=np.uint8, count=texel_count * 4, offset=ktx_tex.image_offset(0, 0, 0))
        self._texture_min_alpha[path_str] = float(data[3::4].min()) / 255.0

    def _create_and_upload_texture_ktx(self, name, ktx_tex, verbose):
        width, height = ktx_tex.base_width, ktx_tex.base_height
        faces_slices = ktx_tex.num_faces * ktx_tex.base_depth
        layers = 1
        if hasattr(ktx_tex, "num_layers"):
            layers = ktx_tex.num_layers
        levels = ktx_tex.num_levels

        dim = ConvUtility.ktx_to_yapt_dimension(ktx_tex, verbose)
        form = ConvUtility.vk_to_yapt_format(ktx_tex.vk_format, verbose)
        
        if verbose:
            print(f"texture dimension: w: {width}, h: {height}, faces_slices: {faces_slices}, layers: {layers}, levels: {levels}, format: {ktx_tex.vk_format.name}, dim: {dim.name}")
        
        
        
        tex =  self._renderer.createTexture(name, dim, form, ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, width, height, levels, layers * faces_slices)
        uint_ptr = int(ffi.cast("uintptr_t", ffi.from_buffer(ktx_tex.data()))) #should maybe just accept the cffi buffer?
        
        for level in range(0, levels):
            for layer in range(0, layers):
                for face_slice in range(0, faces_slices):
                    row_pitch = ktx_tex.row_pitch(level)
                    offset = ktx_tex.image_offset(level, layer, face_slice)
                    tex.upload(level, layer * face_slice, 1, 1, row_pitch, uint_ptr, int(offset))
        
        return tex

