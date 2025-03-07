from py_yapt import Renderer, ResourceUsageBits
from yapt.conversions import ConvUtility
from pathlib import Path
import sys

import OpenImageIO as oiio
import imageio.v3 as iio

class Resources:
    def __init__(self, renderer):
        self._renderer = renderer
        self._textures = {}
        self._meshes = {}

    def load_texture_from_path(self, tex_path):
        path = Path(tex_path)
        path_str = str(path)
        img_name = path.stem


        #self._textures[img_name] = self.__load_tex_with_iio(path_str, img_name)
        self._textures[img_name] = self.__load_tex_with_oiio(path_str, img_name)

    def __load_tex_with_oiio(self, path_str, img_name):
        image = oiio.ImageInput.open(path_str)
        spec = image.spec()

        width, height, depth = spec.width, spec.height, spec.depth
        dim = ConvUtility.oiio_spec_to_dimension(spec)
        form = ConvUtility.oiio_spec_to_format(spec)
        print(f"dim {dim}, format {form}")
        #return self._renderer.createTexture(img_name, dim, form, ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, width, height, 1, 1)

    def __load_tex_with_iio(self, path_str, img_name):
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

        
        


    
