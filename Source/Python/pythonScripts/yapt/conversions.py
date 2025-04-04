from py_yapt import ResourceDimension, ResourceFormat, ResourceUsageBits
import OpenImageIO as oiio
import imageio.v3 as iio

class ConvUtility:

    def iio_image_props_to_dimension(props):
        raise NotImplementedError

    def iio_image_props_to_format(props):
        raise NotImplementedError

    def oiio_spec_to_format(spec, print_debug = False):
        channels = spec.nchannels
        channel_format = spec.format
        bytes_per_channel = spec.channel_bytes
        channel_names = spec.channelnames
        
        components_str = ""
        for v in channel_names:
            components_str += v

        #components_str = "R"
        #if(channels > 1): components_str += "G"
        #if(channels > 2): components_str += "B"
        #if(channels > 3): components_str += "A"
        
        assume_srgb = True
        unorm_type = "SRGB" if assume_srgb is True else "UNORM"

        use_norm = True
        uint_types = [unorm_type, "UINT"]
        int_types = ["SNORM", "INT"]
        type_index = 0 if use_norm is True else 1

        depth_and_type_str = None

        if(channel_format == oiio.UINT8):
            depth_and_type_str = f"8_{uint_types[type_index]}"
        elif(channel_format == oiio.INT8):
            depth_and_type_str = f"8_{int_types[type_index]}"

        elif(channel_format == oiio.UINT16):
            depth_and_type_str = f"16_{uint_types[type_index]}"
        elif(channel_format == oiio.INT16):
            depth_and_type_str = f"16_{int_types[type_index]}"
        elif(channel_format == oiio.HALF):
            depth_and_type_str = "16_SFLOAT"

        elif(channel_format == oiio.UINT32):
            depth_and_type_str = "32_UINT"
        elif(channel_format == oiio.INT32):
            depth_and_type_str = "32_SINT"
        elif(channel_format == oiio.FLOAT):
            depth_and_type_str = "32_SFLOAT"

        if depth_and_type_str != None:
            format_string = components_str + depth_and_type_str
            if print_debug:
                print(format_string)

            return getattr(ResourceFormat, format_string, None)
            
        else:
            raise Exception('unrecognized format')
        
         
        

    def oiio_spec_to_dimension(spec, print_debug = False):
        width, height, depth = spec.width, spec.height, spec.depth
        dim = None
        if depth > 1:
            dim = ResourceDimension.TEXTURE_3D
        else:
            if height == width*6:
                dim = ResourceDimension.TEXTURE_CUBEMAP
            elif height == 1:
                dim = ResourceDimension.TEXTURE_1D
            else:
                dim = ResourceDimension.TEXTURE_2D

        if print_debug:
            print(f"inferred texture dimension to {dim}, w: {width}, h: {height}, d:{depth}")

        return dim