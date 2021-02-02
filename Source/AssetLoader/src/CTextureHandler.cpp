#include <AssetLoader/Impl/CTextureHandler.h>
#include <Common/Logger.h>
#include <Common/CommonUtilities.h>
#include <Renderer/RendererCommonTypesUtility.h>
#include <assert.h>


//required openGL defines
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C
#define GL_R8_SNORM                       0x8F94
#define GL_RG8_SNORM                      0x8F95
#define GL_RGB8_SNORM                     0x8F96
#define GL_RGBA8_SNORM                    0x8F97
#define GL_R16_SNORM                      0x8F98
#define GL_RG16_SNORM                     0x8F99
#define GL_RGB16_SNORM                    0x8F9A
#define GL_RGBA16_SNORM                   0x8F9B
#define GL_RGBA32UI                       0x8D70
#define GL_RGB32UI                        0x8D71
#define GL_RGBA16UI                       0x8D76
#define GL_RGB16UI                        0x8D77
#define GL_RGBA8UI                        0x8D7C
#define GL_RGB8UI                         0x8D7D
#define GL_RGBA32I                        0x8D82
#define GL_RGB32I                         0x8D83
#define GL_RGBA16I                        0x8D88
#define GL_RGB16I                         0x8D89
#define GL_RGBA8I                         0x8D8E
#define GL_RGB8I                          0x8D8F
#define GL_RGBA32F                        0x8814
#define GL_RGB32F                         0x8815
#define GL_RGBA16F                        0x881A
#define GL_RGB16F                         0x881B
#define GL_RGBA8                          0x8058
#define GL_RGBA16                         0x805B

#define GL_COMPRESSED_RGBA_BPTC_UNORM     0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F

#define YAPT_TO_VK_FORMAT_CASE(vkFormat, resFormat) case ResourceFormat::resFormat: format = VK_FORMAT_COPY_##vkFormat; break;
#define VK_TO_YAPT_FORMAT_CASE(vkFormat, resFormat) case VK_FORMAT_COPY_##vkFormat:  format = ResourceFormat::resFormat; break;
#define GL_TO_YAPT_FORMAT_UNCOMPRESSED(internalFormat, yaptFormat) if(tex->glInternalformat == GL_##internalFormat) format = yaptFormat; 
#define GL_TO_YAPT_FORMAT_COMPRESSED(internalFormat, yaptFormat) if(tex->glInternalformat == GL_##internalFormat) format = yaptFormat; 

namespace YAPT
{

    enum VkFormat_COPY {
        VK_FORMAT_COPY_UNDEFINED = 0,
        VK_FORMAT_COPY_R4G4_UNORM_PACK8 = 1,
        VK_FORMAT_COPY_R4G4B4A4_UNORM_PACK16 = 2,
        VK_FORMAT_COPY_B4G4R4A4_UNORM_PACK16 = 3,
        VK_FORMAT_COPY_R5G6B5_UNORM_PACK16 = 4,
        VK_FORMAT_COPY_B5G6R5_UNORM_PACK16 = 5,
        VK_FORMAT_COPY_R5G5B5A1_UNORM_PACK16 = 6,
        VK_FORMAT_COPY_B5G5R5A1_UNORM_PACK16 = 7,
        VK_FORMAT_COPY_A1R5G5B5_UNORM_PACK16 = 8,
        VK_FORMAT_COPY_R8_UNORM = 9,
        VK_FORMAT_COPY_R8_SNORM = 10,
        VK_FORMAT_COPY_R8_USCALED = 11,
        VK_FORMAT_COPY_R8_SSCALED = 12,
        VK_FORMAT_COPY_R8_UINT = 13,
        VK_FORMAT_COPY_R8_SINT = 14,
        VK_FORMAT_COPY_R8_SRGB = 15,
        VK_FORMAT_COPY_R8G8_UNORM = 16,
        VK_FORMAT_COPY_R8G8_SNORM = 17,
        VK_FORMAT_COPY_R8G8_USCALED = 18,
        VK_FORMAT_COPY_R8G8_SSCALED = 19,
        VK_FORMAT_COPY_R8G8_UINT = 20,
        VK_FORMAT_COPY_R8G8_SINT = 21,
        VK_FORMAT_COPY_R8G8_SRGB = 22,
        VK_FORMAT_COPY_R8G8B8_UNORM = 23,
        VK_FORMAT_COPY_R8G8B8_SNORM = 24,
        VK_FORMAT_COPY_R8G8B8_USCALED = 25,
        VK_FORMAT_COPY_R8G8B8_SSCALED = 26,
        VK_FORMAT_COPY_R8G8B8_UINT = 27,
        VK_FORMAT_COPY_R8G8B8_SINT = 28,
        VK_FORMAT_COPY_R8G8B8_SRGB = 29,
        VK_FORMAT_COPY_B8G8R8_UNORM = 30,
        VK_FORMAT_COPY_B8G8R8_SNORM = 31,
        VK_FORMAT_COPY_B8G8R8_USCALED = 32,
        VK_FORMAT_COPY_B8G8R8_SSCALED = 33,
        VK_FORMAT_COPY_B8G8R8_UINT = 34,
        VK_FORMAT_COPY_B8G8R8_SINT = 35,
        VK_FORMAT_COPY_B8G8R8_SRGB = 36,
        VK_FORMAT_COPY_R8G8B8A8_UNORM = 37,
        VK_FORMAT_COPY_R8G8B8A8_SNORM = 38,
        VK_FORMAT_COPY_R8G8B8A8_USCALED = 39,
        VK_FORMAT_COPY_R8G8B8A8_SSCALED = 40,
        VK_FORMAT_COPY_R8G8B8A8_UINT = 41,
        VK_FORMAT_COPY_R8G8B8A8_SINT = 42,
        VK_FORMAT_COPY_R8G8B8A8_SRGB = 43,
        VK_FORMAT_COPY_B8G8R8A8_UNORM = 44,
        VK_FORMAT_COPY_B8G8R8A8_SNORM = 45,
        VK_FORMAT_COPY_B8G8R8A8_USCALED = 46,
        VK_FORMAT_COPY_B8G8R8A8_SSCALED = 47,
        VK_FORMAT_COPY_B8G8R8A8_UINT = 48,
        VK_FORMAT_COPY_B8G8R8A8_SINT = 49,
        VK_FORMAT_COPY_B8G8R8A8_SRGB = 50,
        VK_FORMAT_COPY_A8B8G8R8_UNORM_PACK32 = 51,
        VK_FORMAT_COPY_A8B8G8R8_SNORM_PACK32 = 52,
        VK_FORMAT_COPY_A8B8G8R8_USCALED_PACK32 = 53,
        VK_FORMAT_COPY_A8B8G8R8_SSCALED_PACK32 = 54,
        VK_FORMAT_COPY_A8B8G8R8_UINT_PACK32 = 55,
        VK_FORMAT_COPY_A8B8G8R8_SINT_PACK32 = 56,
        VK_FORMAT_COPY_A8B8G8R8_SRGB_PACK32 = 57,
        VK_FORMAT_COPY_A2R10G10B10_UNORM_PACK32 = 58,
        VK_FORMAT_COPY_A2R10G10B10_SNORM_PACK32 = 59,
        VK_FORMAT_COPY_A2R10G10B10_USCALED_PACK32 = 60,
        VK_FORMAT_COPY_A2R10G10B10_SSCALED_PACK32 = 61,
        VK_FORMAT_COPY_A2R10G10B10_UINT_PACK32 = 62,
        VK_FORMAT_COPY_A2R10G10B10_SINT_PACK32 = 63,
        VK_FORMAT_COPY_A2B10G10R10_UNORM_PACK32 = 64,
        VK_FORMAT_COPY_A2B10G10R10_SNORM_PACK32 = 65,
        VK_FORMAT_COPY_A2B10G10R10_USCALED_PACK32 = 66,
        VK_FORMAT_COPY_A2B10G10R10_SSCALED_PACK32 = 67,
        VK_FORMAT_COPY_A2B10G10R10_UINT_PACK32 = 68,
        VK_FORMAT_COPY_A2B10G10R10_SINT_PACK32 = 69,
        VK_FORMAT_COPY_R16_UNORM = 70,
        VK_FORMAT_COPY_R16_SNORM = 71,
        VK_FORMAT_COPY_R16_USCALED = 72,
        VK_FORMAT_COPY_R16_SSCALED = 73,
        VK_FORMAT_COPY_R16_UINT = 74,
        VK_FORMAT_COPY_R16_SINT = 75,
        VK_FORMAT_COPY_R16_SFLOAT = 76,
        VK_FORMAT_COPY_R16G16_UNORM = 77,
        VK_FORMAT_COPY_R16G16_SNORM = 78,
        VK_FORMAT_COPY_R16G16_USCALED = 79,
        VK_FORMAT_COPY_R16G16_SSCALED = 80,
        VK_FORMAT_COPY_R16G16_UINT = 81,
        VK_FORMAT_COPY_R16G16_SINT = 82,
        VK_FORMAT_COPY_R16G16_SFLOAT = 83,
        VK_FORMAT_COPY_R16G16B16_UNORM = 84,
        VK_FORMAT_COPY_R16G16B16_SNORM = 85,
        VK_FORMAT_COPY_R16G16B16_USCALED = 86,
        VK_FORMAT_COPY_R16G16B16_SSCALED = 87,
        VK_FORMAT_COPY_R16G16B16_UINT = 88,
        VK_FORMAT_COPY_R16G16B16_SINT = 89,
        VK_FORMAT_COPY_R16G16B16_SFLOAT = 90,
        VK_FORMAT_COPY_R16G16B16A16_UNORM = 91,
        VK_FORMAT_COPY_R16G16B16A16_SNORM = 92,
        VK_FORMAT_COPY_R16G16B16A16_USCALED = 93,
        VK_FORMAT_COPY_R16G16B16A16_SSCALED = 94,
        VK_FORMAT_COPY_R16G16B16A16_UINT = 95,
        VK_FORMAT_COPY_R16G16B16A16_SINT = 96,
        VK_FORMAT_COPY_R16G16B16A16_SFLOAT = 97,
        VK_FORMAT_COPY_R32_UINT = 98,
        VK_FORMAT_COPY_R32_SINT = 99,
        VK_FORMAT_COPY_R32_SFLOAT = 100,
        VK_FORMAT_COPY_R32G32_UINT = 101,
        VK_FORMAT_COPY_R32G32_SINT = 102,
        VK_FORMAT_COPY_R32G32_SFLOAT = 103,
        VK_FORMAT_COPY_R32G32B32_UINT = 104,
        VK_FORMAT_COPY_R32G32B32_SINT = 105,
        VK_FORMAT_COPY_R32G32B32_SFLOAT = 106,
        VK_FORMAT_COPY_R32G32B32A32_UINT = 107,
        VK_FORMAT_COPY_R32G32B32A32_SINT = 108,
        VK_FORMAT_COPY_R32G32B32A32_SFLOAT = 109,
        VK_FORMAT_COPY_R64_UINT = 110,
        VK_FORMAT_COPY_R64_SINT = 111,
        VK_FORMAT_COPY_R64_SFLOAT = 112,
        VK_FORMAT_COPY_R64G64_UINT = 113,
        VK_FORMAT_COPY_R64G64_SINT = 114,
        VK_FORMAT_COPY_R64G64_SFLOAT = 115,
        VK_FORMAT_COPY_R64G64B64_UINT = 116,
        VK_FORMAT_COPY_R64G64B64_SINT = 117,
        VK_FORMAT_COPY_R64G64B64_SFLOAT = 118,
        VK_FORMAT_COPY_R64G64B64A64_UINT = 119,
        VK_FORMAT_COPY_R64G64B64A64_SINT = 120,
        VK_FORMAT_COPY_R64G64B64A64_SFLOAT = 121,
        VK_FORMAT_COPY_B10G11R11_UFLOAT_PACK32 = 122,
        VK_FORMAT_COPY_E5B9G9R9_UFLOAT_PACK32 = 123,
        VK_FORMAT_COPY_D16_UNORM = 124,
        VK_FORMAT_COPY_X8_D24_UNORM_PACK32 = 125,
        VK_FORMAT_COPY_D32_SFLOAT = 126,
        VK_FORMAT_COPY_S8_UINT = 127,
        VK_FORMAT_COPY_D16_UNORM_S8_UINT = 128,
        VK_FORMAT_COPY_D24_UNORM_S8_UINT = 129,
        VK_FORMAT_COPY_D32_SFLOAT_S8_UINT = 130,
        VK_FORMAT_COPY_BC1_RGB_UNORM_BLOCK = 131,
        VK_FORMAT_COPY_BC1_RGB_SRGB_BLOCK = 132,
        VK_FORMAT_COPY_BC1_RGBA_UNORM_BLOCK = 133,
        VK_FORMAT_COPY_BC1_RGBA_SRGB_BLOCK = 134,
        VK_FORMAT_COPY_BC2_UNORM_BLOCK = 135,
        VK_FORMAT_COPY_BC2_SRGB_BLOCK = 136,
        VK_FORMAT_COPY_BC3_UNORM_BLOCK = 137,
        VK_FORMAT_COPY_BC3_SRGB_BLOCK = 138,
        VK_FORMAT_COPY_BC4_UNORM_BLOCK = 139,
        VK_FORMAT_COPY_BC4_SNORM_BLOCK = 140,
        VK_FORMAT_COPY_BC5_UNORM_BLOCK = 141,
        VK_FORMAT_COPY_BC5_SNORM_BLOCK = 142,
        VK_FORMAT_COPY_BC6H_UFLOAT_BLOCK = 143,
        VK_FORMAT_COPY_BC6H_SFLOAT_BLOCK = 144,
        VK_FORMAT_COPY_BC7_UNORM_BLOCK = 145,
        VK_FORMAT_COPY_BC7_SRGB_BLOCK = 146,
        VK_FORMAT_COPY_ETC2_R8G8B8_UNORM_BLOCK = 147,
        VK_FORMAT_COPY_ETC2_R8G8B8_SRGB_BLOCK = 148,
        VK_FORMAT_COPY_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
        VK_FORMAT_COPY_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
        VK_FORMAT_COPY_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
        VK_FORMAT_COPY_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
        VK_FORMAT_COPY_EAC_R11_UNORM_BLOCK = 153,
        VK_FORMAT_COPY_EAC_R11_SNORM_BLOCK = 154,
        VK_FORMAT_COPY_EAC_R11G11_UNORM_BLOCK = 155,
        VK_FORMAT_COPY_EAC_R11G11_SNORM_BLOCK = 156,
        VK_FORMAT_COPY_ASTC_4x4_UNORM_BLOCK = 157,
        VK_FORMAT_COPY_ASTC_4x4_SRGB_BLOCK = 158,
        VK_FORMAT_COPY_ASTC_5x4_UNORM_BLOCK = 159,
        VK_FORMAT_COPY_ASTC_5x4_SRGB_BLOCK = 160,
        VK_FORMAT_COPY_ASTC_5x5_UNORM_BLOCK = 161,
        VK_FORMAT_COPY_ASTC_5x5_SRGB_BLOCK = 162,
        VK_FORMAT_COPY_ASTC_6x5_UNORM_BLOCK = 163,
        VK_FORMAT_COPY_ASTC_6x5_SRGB_BLOCK = 164,
        VK_FORMAT_COPY_ASTC_6x6_UNORM_BLOCK = 165,
        VK_FORMAT_COPY_ASTC_6x6_SRGB_BLOCK = 166,
        VK_FORMAT_COPY_ASTC_8x5_UNORM_BLOCK = 167,
        VK_FORMAT_COPY_ASTC_8x5_SRGB_BLOCK = 168,
        VK_FORMAT_COPY_ASTC_8x6_UNORM_BLOCK = 169,
        VK_FORMAT_COPY_ASTC_8x6_SRGB_BLOCK = 170,
        VK_FORMAT_COPY_ASTC_8x8_UNORM_BLOCK = 171,
        VK_FORMAT_COPY_ASTC_8x8_SRGB_BLOCK = 172,
        VK_FORMAT_COPY_ASTC_10x5_UNORM_BLOCK = 173,
        VK_FORMAT_COPY_ASTC_10x5_SRGB_BLOCK = 174,
        VK_FORMAT_COPY_ASTC_10x6_UNORM_BLOCK = 175,
        VK_FORMAT_COPY_ASTC_10x6_SRGB_BLOCK = 176,
        VK_FORMAT_COPY_ASTC_10x8_UNORM_BLOCK = 177,
        VK_FORMAT_COPY_ASTC_10x8_SRGB_BLOCK = 178,
        VK_FORMAT_COPY_ASTC_10x10_UNORM_BLOCK = 179,
        VK_FORMAT_COPY_ASTC_10x10_SRGB_BLOCK = 180,
        VK_FORMAT_COPY_ASTC_12x10_UNORM_BLOCK = 181,
        VK_FORMAT_COPY_ASTC_12x10_SRGB_BLOCK = 182,
        VK_FORMAT_COPY_ASTC_12x12_UNORM_BLOCK = 183,
        VK_FORMAT_COPY_ASTC_12x12_SRGB_BLOCK = 184,
        VK_FORMAT_COPY_G8B8G8R8_422_UNORM = 1000156000,
        VK_FORMAT_COPY_B8G8R8G8_422_UNORM = 1000156001,
        VK_FORMAT_COPY_G8_B8_R8_3PLANE_420_UNORM = 1000156002,
        VK_FORMAT_COPY_G8_B8R8_2PLANE_420_UNORM = 1000156003,
        VK_FORMAT_COPY_G8_B8_R8_3PLANE_422_UNORM = 1000156004,
        VK_FORMAT_COPY_G8_B8R8_2PLANE_422_UNORM = 1000156005,
        VK_FORMAT_COPY_G8_B8_R8_3PLANE_444_UNORM = 1000156006,
        VK_FORMAT_COPY_R10X6_UNORM_PACK16 = 1000156007,
        VK_FORMAT_COPY_R10X6G10X6_UNORM_2PACK16 = 1000156008,
        VK_FORMAT_COPY_R10X6G10X6B10X6A10X6_UNORM_4PACK16 = 1000156009,
        VK_FORMAT_COPY_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010,
        VK_FORMAT_COPY_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011,
        VK_FORMAT_COPY_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
        VK_FORMAT_COPY_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013,
        VK_FORMAT_COPY_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
        VK_FORMAT_COPY_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015,
        VK_FORMAT_COPY_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
        VK_FORMAT_COPY_R12X4_UNORM_PACK16 = 1000156017,
        VK_FORMAT_COPY_R12X4G12X4_UNORM_2PACK16 = 1000156018,
        VK_FORMAT_COPY_R12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019,
        VK_FORMAT_COPY_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020,
        VK_FORMAT_COPY_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021,
        VK_FORMAT_COPY_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
        VK_FORMAT_COPY_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023,
        VK_FORMAT_COPY_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
        VK_FORMAT_COPY_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025,
        VK_FORMAT_COPY_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
        VK_FORMAT_COPY_G16B16G16R16_422_UNORM = 1000156027,
        VK_FORMAT_COPY_B16G16R16G16_422_UNORM = 1000156028,
        VK_FORMAT_COPY_G16_B16_R16_3PLANE_420_UNORM = 1000156029,
        VK_FORMAT_COPY_G16_B16R16_2PLANE_420_UNORM = 1000156030,
        VK_FORMAT_COPY_G16_B16_R16_3PLANE_422_UNORM = 1000156031,
        VK_FORMAT_COPY_G16_B16R16_2PLANE_422_UNORM = 1000156032,
        VK_FORMAT_COPY_G16_B16_R16_3PLANE_444_UNORM = 1000156033,
        VK_FORMAT_COPY_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,
        VK_FORMAT_COPY_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,
        VK_FORMAT_COPY_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,
        VK_FORMAT_COPY_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,
        VK_FORMAT_COPY_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,
        VK_FORMAT_COPY_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,
        VK_FORMAT_COPY_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,
        VK_FORMAT_COPY_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,
        VK_FORMAT_COPY_ASTC_4x4_SFLOAT_BLOCK_EXT = 1000066000,
        VK_FORMAT_COPY_ASTC_5x4_SFLOAT_BLOCK_EXT = 1000066001,
        VK_FORMAT_COPY_ASTC_5x5_SFLOAT_BLOCK_EXT = 1000066002,
        VK_FORMAT_COPY_ASTC_6x5_SFLOAT_BLOCK_EXT = 1000066003,
        VK_FORMAT_COPY_ASTC_6x6_SFLOAT_BLOCK_EXT = 1000066004,
        VK_FORMAT_COPY_ASTC_8x5_SFLOAT_BLOCK_EXT = 1000066005,
        VK_FORMAT_COPY_ASTC_8x6_SFLOAT_BLOCK_EXT = 1000066006,
        VK_FORMAT_COPY_ASTC_8x8_SFLOAT_BLOCK_EXT = 1000066007,
        VK_FORMAT_COPY_ASTC_10x5_SFLOAT_BLOCK_EXT = 1000066008,
        VK_FORMAT_COPY_ASTC_10x6_SFLOAT_BLOCK_EXT = 1000066009,
        VK_FORMAT_COPY_ASTC_10x8_SFLOAT_BLOCK_EXT = 1000066010,
        VK_FORMAT_COPY_ASTC_10x10_SFLOAT_BLOCK_EXT = 1000066011,
        VK_FORMAT_COPY_ASTC_12x10_SFLOAT_BLOCK_EXT = 1000066012,
        VK_FORMAT_COPY_ASTC_12x12_SFLOAT_BLOCK_EXT = 1000066013,

    } ;


	ResourceFormat getFormat(ktxTexture1* tex)
	{

        ResourceFormat format = ResourceFormat::UNKNOWN;

        if (tex->isCompressed)
        {
            GL_TO_YAPT_FORMAT_COMPRESSED(COMPRESSED_RGB_BPTC_SIGNED_FLOAT, ResourceFormat::BC4_SNORM);
            GL_TO_YAPT_FORMAT_COMPRESSED(COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, ResourceFormat::BC4_UNORM);
            GL_TO_YAPT_FORMAT_COMPRESSED(COMPRESSED_RGBA_BPTC_UNORM, ResourceFormat::BC7_UNORM);
            GL_TO_YAPT_FORMAT_COMPRESSED(COMPRESSED_SRGB_ALPHA_BPTC_UNORM, ResourceFormat::BC7_UNORM_SRGB);
        }
        else
        {
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R8, ResourceFormat::R8_UNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R8_SNORM, ResourceFormat::R8_SNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R8I, ResourceFormat::R8_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R8UI, ResourceFormat::R8_UINT);
            //GL_TO_YAPT_FORMAT_UNCOMPRESSED(SRGB8, ResourceFormat::R8_SRGB);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG8, ResourceFormat::RG8_UNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG8_SNORM, ResourceFormat::RG8_SNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG8I, ResourceFormat::RG8_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG8UI, ResourceFormat::RG8_UINT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA8, ResourceFormat::RGBA8_UNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA8_SNORM, ResourceFormat::RGBA8_SNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA8I, ResourceFormat::RGBA8_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA8UI, ResourceFormat::RGBA8_UINT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R16, ResourceFormat::R16_UNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R16_SNORM, ResourceFormat::R16_SNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R16I, ResourceFormat::R16_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R16UI, ResourceFormat::R16_UINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R16F, ResourceFormat::R16_SFLOAT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG16, ResourceFormat::RG16_UNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG16_SNORM, ResourceFormat::RG16_SNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG16I, ResourceFormat::RG16_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG16UI, ResourceFormat::RG16_UINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG16F, ResourceFormat::RG16_SFLOAT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA16, ResourceFormat::RGBA16_UNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA16_SNORM, ResourceFormat::RGBA16_SNORM);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA16I, ResourceFormat::RGBA16_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA16UI, ResourceFormat::RGBA16_UINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA16F, ResourceFormat::RGBA16_SFLOAT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R32UI, ResourceFormat::R32_UINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R32I, ResourceFormat::R32_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(R32F, ResourceFormat::R32_SFLOAT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG32UI, ResourceFormat::RG32_UINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG32I, ResourceFormat::RG32_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RG32F, ResourceFormat::RG32_SFLOAT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGB32UI, ResourceFormat::RGB32_UINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGB32I, ResourceFormat::RGB32_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGB32F, ResourceFormat::RGB32_SFLOAT);

            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA32UI, ResourceFormat::RGBA32_UINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA32I, ResourceFormat::RGBA32_SINT);
            GL_TO_YAPT_FORMAT_UNCOMPRESSED(RGBA32F, ResourceFormat::RGBA32_SFLOAT);

        }
		
        if (format == ResourceFormat::UNKNOWN)
        {
            YAPT_LOG_FATAL_ERROR("Unknown/unsupported fileformat");
        }
		return format;
	}

	ResourceFormat getFormat(ktxTexture2* tex)
	{
        ResourceFormat format;
        switch (tex->vkFormat)
        {
            VK_TO_YAPT_FORMAT_CASE(R8_UNORM, R8_UNORM);
            VK_TO_YAPT_FORMAT_CASE(R8_SNORM, R8_SNORM);
            VK_TO_YAPT_FORMAT_CASE(R8_UINT, R8_UINT);
            VK_TO_YAPT_FORMAT_CASE(R8_SINT, R8_SINT);
            VK_TO_YAPT_FORMAT_CASE(R8_SRGB, R8_SRGB);

            VK_TO_YAPT_FORMAT_CASE(R16_UNORM, R16_UNORM);
            VK_TO_YAPT_FORMAT_CASE(R16_SNORM, R16_SNORM);
            VK_TO_YAPT_FORMAT_CASE(R16_UINT, R16_UINT);
            VK_TO_YAPT_FORMAT_CASE(R16_SINT, R16_SINT);
            VK_TO_YAPT_FORMAT_CASE(R16_SFLOAT, R16_SFLOAT);

            VK_TO_YAPT_FORMAT_CASE(R32_UINT, R32_UINT);
            VK_TO_YAPT_FORMAT_CASE(R32_SINT, R32_SINT);
            VK_TO_YAPT_FORMAT_CASE(R32_SFLOAT, R32_SFLOAT);

            VK_TO_YAPT_FORMAT_CASE(R8G8_UNORM, RG8_UNORM);
            VK_TO_YAPT_FORMAT_CASE(R8G8_SNORM, RG8_SNORM);
            VK_TO_YAPT_FORMAT_CASE(R8G8_UINT, RG8_UINT);
            VK_TO_YAPT_FORMAT_CASE(R8G8_SINT, RG8_SINT);
            VK_TO_YAPT_FORMAT_CASE(R8G8_SRGB, RG8_SRGB);
        
            VK_TO_YAPT_FORMAT_CASE(R16G16_UNORM, RG16_UNORM);
            VK_TO_YAPT_FORMAT_CASE(R16G16_SNORM, RG16_SNORM);
            VK_TO_YAPT_FORMAT_CASE(R16G16_UINT, RG16_UINT);
            VK_TO_YAPT_FORMAT_CASE(R16G16_SINT, RG16_SINT);
            VK_TO_YAPT_FORMAT_CASE(R16G16_SFLOAT, RG16_SFLOAT);

            VK_TO_YAPT_FORMAT_CASE(R32G32_UINT, RG32_UINT);
            VK_TO_YAPT_FORMAT_CASE(R32G32_SINT, RG32_SINT);
            VK_TO_YAPT_FORMAT_CASE(R32G32_SFLOAT, RG32_SFLOAT);

            VK_TO_YAPT_FORMAT_CASE(R32G32B32_UINT, RGB32_UINT);
            VK_TO_YAPT_FORMAT_CASE(R32G32B32_SINT, RGB32_SINT);
            VK_TO_YAPT_FORMAT_CASE(R32G32B32_SFLOAT, RGB32_SFLOAT);

            VK_TO_YAPT_FORMAT_CASE(R8G8B8A8_UNORM, RGBA8_UNORM);
            VK_TO_YAPT_FORMAT_CASE(R8G8B8A8_SNORM, RGBA8_SNORM);
            VK_TO_YAPT_FORMAT_CASE(R8G8B8A8_UINT, RGBA8_UINT);
            VK_TO_YAPT_FORMAT_CASE(R8G8B8A8_SINT, RGBA8_SINT);
            VK_TO_YAPT_FORMAT_CASE(R8G8B8A8_SRGB, RGBA8_SRGB);

            VK_TO_YAPT_FORMAT_CASE(R16G16B16A16_UNORM, RGBA16_UNORM);
            VK_TO_YAPT_FORMAT_CASE(R16G16B16A16_SNORM, RGBA16_SNORM);
            VK_TO_YAPT_FORMAT_CASE(R16G16B16A16_UINT, RGBA16_UINT);
            VK_TO_YAPT_FORMAT_CASE(R16G16B16A16_SINT, RGBA16_SINT);
            VK_TO_YAPT_FORMAT_CASE(R16G16B16A16_SFLOAT, RGBA16_SFLOAT);

            VK_TO_YAPT_FORMAT_CASE(R32G32B32A32_UINT, RGBA32_UINT);
            VK_TO_YAPT_FORMAT_CASE(R32G32B32A32_SINT, RGBA32_SINT);
            VK_TO_YAPT_FORMAT_CASE(R32G32B32A32_SFLOAT, RGBA32_SFLOAT);

            VK_TO_YAPT_FORMAT_CASE(BC4_UNORM_BLOCK, BC4_UNORM);
            VK_TO_YAPT_FORMAT_CASE(BC4_SNORM_BLOCK, BC4_SNORM);
            VK_TO_YAPT_FORMAT_CASE(BC7_UNORM_BLOCK, BC7_UNORM);
            VK_TO_YAPT_FORMAT_CASE(BC7_SRGB_BLOCK, BC7_UNORM_SRGB);
        default:
            YAPT_LOG_ERROR("UNSUPPORTED TEXTURE FORMAT");
            format = ResourceFormat::UNKNOWN;
            break;
        }

		return format;
	}

    VkFormat_COPY getVKFormat(ResourceFormat originalFormat)
    {
        VkFormat_COPY format;
        switch (originalFormat)
        {
            YAPT_TO_VK_FORMAT_CASE(R8_UNORM, R8_UNORM);
            YAPT_TO_VK_FORMAT_CASE(R8_SNORM, R8_SNORM);
            YAPT_TO_VK_FORMAT_CASE(R8_UINT, R8_UINT);
            YAPT_TO_VK_FORMAT_CASE(R8_SINT, R8_SINT);
            YAPT_TO_VK_FORMAT_CASE(R8_SRGB, R8_SRGB);
  
            YAPT_TO_VK_FORMAT_CASE(R16_UNORM, R16_UNORM);
            YAPT_TO_VK_FORMAT_CASE(R16_SNORM, R16_SNORM);
            YAPT_TO_VK_FORMAT_CASE(R16_UINT, R16_UINT);
            YAPT_TO_VK_FORMAT_CASE(R16_SINT, R16_SINT);
            YAPT_TO_VK_FORMAT_CASE(R16_SFLOAT, R16_SFLOAT);

            YAPT_TO_VK_FORMAT_CASE(R32_UINT, R32_UINT);
            YAPT_TO_VK_FORMAT_CASE(R32_SINT, R32_SINT);
            YAPT_TO_VK_FORMAT_CASE(R32_SFLOAT, R32_SFLOAT);

            YAPT_TO_VK_FORMAT_CASE(R8G8_UNORM, RG8_UNORM);
            YAPT_TO_VK_FORMAT_CASE(R8G8_SNORM, RG8_SNORM);
            YAPT_TO_VK_FORMAT_CASE(R8G8_UINT, RG8_UINT);
            YAPT_TO_VK_FORMAT_CASE(R8G8_SINT, RG8_SINT);
            YAPT_TO_VK_FORMAT_CASE(R8G8_SRGB, RG8_SRGB);

            YAPT_TO_VK_FORMAT_CASE(R16G16_UNORM, RG16_UNORM);
            YAPT_TO_VK_FORMAT_CASE(R16G16_SNORM, RG16_SNORM);
            YAPT_TO_VK_FORMAT_CASE(R16G16_UINT, RG16_UINT);
            YAPT_TO_VK_FORMAT_CASE(R16G16_SINT, RG16_SINT);
            YAPT_TO_VK_FORMAT_CASE(R16G16_SFLOAT, RG16_SFLOAT);

            YAPT_TO_VK_FORMAT_CASE(R32G32_UINT, RG32_UINT);
            YAPT_TO_VK_FORMAT_CASE(R32G32_SINT, RG32_SINT);
            YAPT_TO_VK_FORMAT_CASE(R32G32_SFLOAT, RG32_SFLOAT);

            YAPT_TO_VK_FORMAT_CASE(R32G32B32_UINT, RGB32_UINT);
            YAPT_TO_VK_FORMAT_CASE(R32G32B32_SINT, RGB32_SINT);
            YAPT_TO_VK_FORMAT_CASE(R32G32B32_SFLOAT, RGB32_SFLOAT);
   
            YAPT_TO_VK_FORMAT_CASE(R8G8B8A8_UNORM, RGBA8_UNORM);
            YAPT_TO_VK_FORMAT_CASE(R8G8B8A8_SNORM, RGBA8_SNORM);
            YAPT_TO_VK_FORMAT_CASE(R8G8B8A8_UINT, RGBA8_UINT);
            YAPT_TO_VK_FORMAT_CASE(R8G8B8A8_SINT, RGBA8_SINT);
            YAPT_TO_VK_FORMAT_CASE(R8G8B8A8_SRGB, RGBA8_SRGB);

            YAPT_TO_VK_FORMAT_CASE(R16G16B16A16_UNORM, RGBA16_UNORM);
            YAPT_TO_VK_FORMAT_CASE(R16G16B16A16_SNORM, RGBA16_SNORM);
            YAPT_TO_VK_FORMAT_CASE(R16G16B16A16_UINT, RGBA16_UINT);
            YAPT_TO_VK_FORMAT_CASE(R16G16B16A16_SINT, RGBA16_SINT);
            YAPT_TO_VK_FORMAT_CASE(R16G16B16A16_SFLOAT, RGBA16_SFLOAT);

            YAPT_TO_VK_FORMAT_CASE(R32G32B32A32_UINT, RGBA32_UINT);
            YAPT_TO_VK_FORMAT_CASE(R32G32B32A32_SINT, RGBA32_SINT);
            YAPT_TO_VK_FORMAT_CASE(R32G32B32A32_SFLOAT, RGBA32_SFLOAT);

            YAPT_TO_VK_FORMAT_CASE(BC4_UNORM_BLOCK, BC4_UNORM);
            YAPT_TO_VK_FORMAT_CASE(BC4_SNORM_BLOCK, BC4_SNORM);
            YAPT_TO_VK_FORMAT_CASE(BC7_UNORM_BLOCK, BC7_UNORM);
            YAPT_TO_VK_FORMAT_CASE(BC7_SRGB_BLOCK, BC7_UNORM_SRGB);
        default:
            YAPT_LOG_ERROR("UNSUPPORTED TEXTURE FORMAT");
            format = VK_FORMAT_COPY_UNDEFINED;
            break;
        }

        return format;
    }

	CKtxTexture::CKtxTexture()
		:m_tex(nullptr)
	{


		

	}

    bool CKtxTexture::init(const char* path)
    {
        ktxTexture* tex;
        KTX_error_code code = ktxTexture_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_NO_FLAGS, &tex);
        if (code != KTX_SUCCESS)
        {
            return false;
        }

        m_tex = tex;

        if (m_tex->isCubemap)
        {
            if (m_tex->isArray)
            {
                m_texInfo.dimension = ResourceDimension::TEXTURE_CUBEMAP_ARRAY;
            }
            else
            {
                m_texInfo.dimension = ResourceDimension::TEXTURE_CUBEMAP;
            }
        }
        else
        {
            if (m_tex->isArray)
            {
                switch (m_tex->numDimensions)
                {
                case 1:
                    m_texInfo.dimension = ResourceDimension::TEXTURE_1D_ARRAY;
                    break;
                case 2:
                    m_texInfo.dimension = ResourceDimension::TEXTURE_2D_ARRAY;
                    break;
                default:
                    YAPT_LOG_FATAL_ERROR("unable to load texture texture");
                    m_texInfo.dimension = ResourceDimension::UNDEFINED;
                    break;
                }
            }
            else
            {
                switch (m_tex->numDimensions)
                {
                case 1:
                    m_texInfo.dimension = ResourceDimension::TEXTURE_1D;
                    break;
                case 2:
                    m_texInfo.dimension = ResourceDimension::TEXTURE_2D;
                    break;
                case 3:
                    m_texInfo.dimension = ResourceDimension::TEXTURE_3D;
                    break;
                default:
                    m_texInfo.dimension = ResourceDimension::UNDEFINED;
                    break;
                }
            }

        }

        m_texInfo.numberOfSlices = size_t(m_tex->numLayers) * m_tex->numFaces;
        m_texInfo.numberOfMips = m_tex->numLevels;


        if (m_tex->classId == ktxTexture1_c)
        {
            ktxTexture1* t = (ktxTexture1*)(m_tex);
            m_texInfo.format = getFormat(t);
        }
        else if (m_tex->classId == ktxTexture2_c)
        {
            ktxTexture2* t = (ktxTexture2*)(m_tex);
            m_texInfo.format = getFormat(t);
        }
        else
        {
            YAPT_LOG_FATAL_ERROR("Unknown ktx texture class id")
        }

        m_subTextureInfo.resize(m_texInfo.numberOfSlices * m_texInfo.numberOfMips);


        size_t subTextureIndex = 0;
        for (uint32_t arraySlice = 0; arraySlice < m_tex->numLayers; ++arraySlice)
        {
            for (uint32_t face = 0; face < m_tex->numFaces; ++face)
            {
                uint32_t width = m_tex->baseWidth;
                uint32_t height = m_tex->baseHeight;
                uint32_t depth = m_tex->baseDepth;

                for (uint32_t mip = 0; mip < m_tex->numLevels; ++mip)
                {
                    SubTextureLoadInfo& info = m_subTextureInfo[subTextureIndex];
                    info.width = max(width, uint32_t(1));
                    info.height = max(height, uint32_t(1));
                    info.depth = max(depth, uint32_t(1));
                    info.rowPitch = ktxTexture_GetRowPitch(m_tex, (ktx_uint32_t)mip);

                    width /= 2;
                    height /= 2;
                    depth /= 2;

                    ++subTextureIndex;
                }
            }
        }
        return true;
    }

	CKtxTexture::~CKtxTexture()
	{
        if (m_tex)
        {
            ktxTexture_Destroy(m_tex);
        }
		
	}
	const TextureLoadInfo& CKtxTexture::getTextureInfo()
	{
		return m_texInfo;
	}
	const SubTextureLoadInfo& CKtxTexture::getSubTextureInfo(uint32_t mip, uint32_t arraySlice)
	{
        uint32_t index = m_tex->numLevels * arraySlice + mip;
		return m_subTextureInfo[index];
	}
	const void* CKtxTexture::getData(uint32_t mip, uint32_t arraySliceOrDepth)
	{
		ktx_uint8_t* data = ktxTexture_GetData(m_tex);
        if (data == nullptr)
        {
            ktxTexture_LoadImageData(m_tex, nullptr, 0);
        }
        data = ktxTexture_GetData(m_tex);
        assert(data != nullptr);

        ktx_uint32_t face;
        ktx_uint32_t layer;

        if (m_texInfo.dimension == ResourceDimension::TEXTURE_3D)
        {
            face = arraySliceOrDepth;
            layer = 0;
        }
        else
        {
            face = (ktx_uint32_t)arraySliceOrDepth % m_tex->numFaces;
            layer = (ktx_uint32_t)arraySliceOrDepth / m_tex->numFaces;
        }
        
		

		ktx_size_t offset;
		KTX_error_code status = ktxTexture_GetImageOffset(m_tex, (ktx_uint32_t)mip, layer, face, &offset);

		if (status != KTX_SUCCESS)
		{
			return nullptr;
		}

		return data + offset;
	}
	void CKtxTexture::release()
	{
		delete this;
	}


    void CKtxTexture::storeTexture(const char* path, const TextureStoreInfo& info)
    {
        ktxTextureCreateInfo createInfo;
        KTX_error_code result;


        bool isCube = (info.dimension == ResourceDimension::TEXTURE_CUBEMAP || info.dimension == ResourceDimension::TEXTURE_CUBEMAP_ARRAY);

        createInfo.vkFormat = getVKFormat(info.format);
        createInfo.baseWidth = info.width;
        createInfo.baseHeight = info.height;
        createInfo.baseDepth = info.depth;
        createInfo.numDimensions = getNumberOfDimensions(info.dimension);
        // Note: it is not necessary to provide a full mipmap pyramid.
        createInfo.numLevels = info.numberOfMips;
        createInfo.numLayers = isCube ? info.numberOfSlices / 6 :  info.numberOfSlices;

        createInfo.pDfd = nullptr;

        createInfo.numFaces = isCube ? 6 : 1;
        createInfo.isArray = isArrayDimension(info.dimension);
        createInfo.generateMipmaps = KTX_FALSE;
        ktxTexture2* texture2 = nullptr;

        result = ktxTexture2_Create(&createInfo,
            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
            &texture2);

        ktxTexture* texture = ktxTexture(texture2);
        uint32_t subTextureIndex = 0;
        for (uint32_t arraySlice = 0; arraySlice < createInfo.numLayers; ++arraySlice)
        {
            for (uint32_t face = 0; face < createInfo.numFaces; ++face)
            {
                uint32_t width = createInfo.baseWidth;
                uint32_t height = createInfo.baseHeight;
                uint32_t depth = createInfo.baseDepth;

                for (uint32_t mip = 0; mip < createInfo.numLevels; ++mip)
                {
                    const ktx_uint8_t* dataPtr = static_cast<const ktx_uint8_t*>(info.dataPtrs[subTextureIndex]);
                    for (uint32_t depthSlice = 0; depthSlice < depth; ++depthSlice)
                    {
                        size_t w = max(width, uint32_t(1));
                        size_t h = max(height, uint32_t(1));

                        ktx_size_t srcSize = getFormatSizeInBytes(info.format) * w * h;

                        result = ktxTexture_SetImageFromMemory(texture, mip, arraySlice, face + depthSlice,
                            dataPtr, srcSize);

                        assert(result == KTX_SUCCESS);

                        dataPtr += srcSize;
                    }

                    width /= 2;
                    height /= 2;
                    depth /= 2;

                    ++subTextureIndex;
                }
            }
        }
        

        ktxTexture_WriteToNamedFile(texture, path);
        ktxTexture_Destroy(texture);
    }
}