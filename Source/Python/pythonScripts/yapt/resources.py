from py_yapt import Renderer, ResourceUsageBits, ResourceDimension
from yapt.conversions import ConvUtility
from pathlib import Path
import sys

import OpenImageIO as oiio
import imageio.v3 as iio
import ctypes

class Resources:
    def __init__(self, renderer):
        self._renderer = renderer
        self._textures = {}
        self._meshes = {}

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

    def __load_tex_with_oiio(self, path_str, img_name, verbose = False):
        image = oiio.ImageInput.open(path_str)

        if image == None:
            print(f"Failed to open image {path_str}")
            return;

        spec = image.spec()
        tex = None
        try:
            width, height, depth = spec.width, spec.height, spec.depth
            mips = 1
            dim = ConvUtility.oiio_spec_to_dimension(spec, verbose)
            form = ConvUtility.oiio_spec_to_format(spec, verbose)
            row_pitch = spec.scanline_bytes() 
            
            data = image.read_image(format=oiio.UNKNOWN)

            adjusted_height = height
            if(dim == ResourceDimension.TEXTURE_CUBEMAP and width == int(height / 6)):
                adjusted_height = int(height / 6)
                depth = 6
                
            tex =  self._renderer.createTexture(img_name, dim, form, ResourceUsageBits.COPY_DESTINATION | ResourceUsageBits.SAMPLED_TEXTURE, width, adjusted_height, mips, depth)
            ptr = data.ctypes.data
            tex.upload(0, 0, mips, 1, row_pitch, ptr)
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

        
        


    
