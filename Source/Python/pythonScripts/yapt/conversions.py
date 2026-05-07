from py_yapt import ResourceDimension, ResourceFormat, ResourceUsageBits
from pyktx.vk_format import VkFormat
import re
class ConvUtility:

    def vk_to_yapt_format(vk_format, print_debug = False):

        if vk_format == VkFormat.VK_FORMAT_D16_UNORM:
            return ResourceFormat.D16_UNORM
        elif vk_format == VkFormat.VK_FORMAT_D32_SFLOAT:
            return ResourceFormat.D32_SFLOAT
        elif vk_format == VkFormat.VK_FORMAT_D24_UNORM_S8_UINT:
            return ResourceFormat.D24_UNORM_S8_UINT

        elif vk_format == VkFormat.VK_FORMAT_BC4_UNORM_BLOCK:
            return ResourceFormat.BC4_UNORM
        elif vk_format == VkFormat.VK_FORMAT_BC4_SNORM_BLOCK:
            return ResourceFormat.BC4_SNORM

        elif vk_format == VkFormat.VK_FORMAT_BC6H_SFLOAT_BLOCK:
            return ResourceFormat.BC6H_SFLOAT
        elif vk_format == VkFormat.VK_FORMAT_BC6H_UFLOAT_BLOCK:
            return ResourceFormat.BC6H_UFLOAT

        elif vk_format == VkFormat.VK_FORMAT_BC7_UNORM_BLOCK:
            return ResourceFormat.BC7_UNORM
        elif vk_format == VkFormat.VK_FORMAT_BC7_SRGB_BLOCK:
            return ResourceFormat.BC7_UNORM_SRGB
        else:
            
            split_str = str(vk_format.name).removeprefix("VK_FORMAT_").split("_")
            yapt_format = None
            if(len(split_str) == 2):
                match = re.search(r"\d+", split_str[0])
                precision = None
                components = None

                if match:
                    precision = match.group()
                if precision:
                    components = split_str[0].replace(precision, "")

                if precision and components:
                    format_str = components + precision + split_str[-1]
                    yapt_format = getattr(ResourceFormat, format_str, None)
                    if yapt_format == None:
                        if print_debug:
                            print(f"Tried to convert format str {format_str} from {vk_format.name} but failed")

            if yapt_format == None:
                raise Exception(f"failed to convert vk format: {vk_format.name}")

            return yapt_format
            
         
        

    def ktx_to_yapt_dimension(ktx_tex, print_debug = False):
        if ktx_tex.is_cubemap:
            dim = ResourceDimension.TEXTURE_CUBEMAP
        else:
            if ktx_tex.num_dimensions == 1:
                dim = ResourceDimension.TEXTURE_1D
            elif ktx_tex.num_dimensions == 2:
                dim = ResourceDimension.TEXTURE_2D
            elif ktx_tex.num_dimensions == 3:
                dim = ResourceDimension.TEXTURE_2D
            else:
                raise Exception('unrecognized dimensions')

        return dim