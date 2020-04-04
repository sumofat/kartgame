#pragma once

enum CompareFunc
{
	compare_func_never,
	//A new value never passes the comparison test.
	compare_func_less,
	//A new value passes the comparison test if it is less than the existing value.Otherwise, the new value fails.
	compare_func_equal,
	//A new value passes the comparison test if it is equal to the existing value.Otherwise, the new value fails.
	compare_func_lessEqual,
	//A new value passes the comparison test if it is less than or equal to the existing value.Otherwise, the new value fails.
	compare_func_greater,
	//A new value passes the comparison test if it is greater than the existing value.Otherwise, the new value fails.
	compare_func_notEqual,
	//A new value passes the comparison test if it is not equal to the existing value.Otherwise, the new value fails.
	compare_func_greaterEqual,
	//A new value passes the comparison test if it is greater than or equal to the existing value.Otherwise, the new value fails.
	compare_func_always,
	//A new value always passes the comparison test.
};

#if !IOS
enum SamplerBorderColor
{
    SamplerBorderColorTransparentBlack = 0,  // {0,0,0,0}
    SamplerBorderColorOpaqueBlack = 1,       // {0,0,0,1}
    SamplerBorderColorOpaqueWhite = 2,       // {1,1,1,1}
};// API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios);
#endif

enum SamplerAddressMode
{
    SamplerAddressModeClampToEdge = 0,
    
#if !IOS
    SamplerAddressModeMirrorClampToEdge,// API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 1,
#endif
    SamplerAddressModeRepeat = 2,
    SamplerAddressModeMirrorRepeat = 3,
    SamplerAddressModeClampToZero = 4,
#if !IOS
    SamplerAddressModeClampToBorderColor = 5// API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios) = 5,
#endif
};

enum SamplerMipFilter
{
    SamplerMipFilterNotMipmapped = 0,
    SamplerMipFilterNearest = 1,
    SamplerMipFilterLinear = 2,
};// API_AVAILABLE(macos(10.11), ios(8.0));

enum SamplerMinMagFilter
{
    SamplerMinMagFilterNearest = 0,
    SamplerMinMagFilterLinear = 1,
};// API_AVAILABLE(macos(10.11), ios(8.0));

struct SamplerDescriptor
{
    SamplerAddressMode r_address_mode;
    //The address mode for the texture depth (r) coordinate.
    SamplerAddressMode s_address_mode;
    //The address mode for the texture width (s) coordinate.
    
    SamplerAddressMode t_address_mode;
    //The address mode for the texture height (t) coordinate.
    
    SamplerMinMagFilter min_filter;
    //The filtering option for combining pixels within one mipmap level when the sample footprint is larger than a pixel (minification).
    
    SamplerMinMagFilter mag_filter;
    //The filtering operation for combining pixels within one mipmap level when the sample footprint is smaller than a pixel (magnification).
    
    SamplerMipFilter mip_filter;
    //The filtering option for combining pixels between two mipmap levels.
    
    float lod_min_clamp;
    //The minimum level of detail (LOD) to use when sampling from a texture.
    
    float lod_max_clamp;
    //The maximum level of detail (LOD) to use when sampling from a texture.
    
    bool lod_average;
    //Allows the driver to use an average level of detail (LOD) when sampling from a texture.
    
    uint32_t max_anisotropy;
    //The number of samples that can be taken to improve the quality of sample footprints that are anisotropic.
    
    bool normalized_coordinates;
    //A Boolean value that indicates whether texture coordinates are normalized to the range [0.0, 1.0].
    
    CompareFunc compare_function;
    //The sampler comparison function used when sampling texels from a depth texture.
    
#if !IOS
    SamplerBorderColor border_color;
    //The border color for clamped texture values.
#endif
    
    // #if __IPHONE_OS_VERSION_MAX_ALLOWED >= 80000
    bool support_argument_buffers;
    //#endif
    //Determines whether the sampler can be encoded into an argument buffer.
    
//    Yostr label;
    //A string that identifies this object.
    void* state;
};

struct SamplerState
{
    void* state;
};

//TODO(Ray):Change uint64 to a proper memory size type.
struct RenderRegion
{
    f3 origin;//xyz
    f2   size;//xy//width height
};

struct RenderRange
{
    uint64_t location;
    uint64_t length;
};

enum TextureUsage
{
    TextureUsageUnknown         = 0x0000,
    //An option that specifies that the texture usage is unknown.
    TextureUsageShaderRead      = 0x0001,
    //An option that enables reading or sampling from the texture.
    TextureUsageShaderWrite     = 0x0002,
    //An option that enables writing to the texture.
    TextureUsageRenderTarget    = 0x0004,
    //An option that enables using the texture as a color, depth, or stencil render target in a render pass descriptor.
    TextureUsagePixelFormatView = 0x0010
};

enum WindingOrder : uint64_t
{
	winding_order_clockwise = 0,//default
	//Primitives whose vertices are specified in clockwise order are front - facing.
	winding_order_counter_clockwise = 1
        //Primitives whose vertices are specified in counter - clockwise order are front - facing.
};

enum TessellationPartitionMode
{
	tesselation_partition_pow2,
	//A power of two partitioning mode.
	tesselation_partition_integer,
	//An integer partitioning mode.
	tesselation_partition_fractional_odd,
	//A fractional odd partitioning mode.
	tesselation_partition_fractionalEven
        //A fractional even partitioning mode.    
};

enum IndexType
{
    IndexTypeUInt16 = 0,
    IndexTypeUInt32 = 1,
};

enum PrimitiveType
{
    primitive_type_point = 0,
    primitive_type_line = 1,
    primitive_type_line_strip = 2,
    primitive_type_triangle = 3,
    primitive_type_trianglestrip = 4
};

enum PrimitiveTopologyClass
{
	topology_unspecified,
	//An unspecified primitive.
	topology_point,
	//A point primitive.
	topology_line,
	//A line primitive.
	topology_triangle
        //A triangle primitive.    
};

enum TessellationFactorFormat
{
    
	tesselation_factor_format_half,
	//A 16-bit floating-point format.  
};

enum TessellationControlPointIndexType
{
	tesselation_control_point_index_type_none,
	//No size. This value should only be used when drawing patches without a control point index buffer.
	tesselation_control_point_index_type_uint16,
	//The size of a 16-bit unsigned integer.
	tesselation_control_point_index_type_uint32,
	//The size of a 32-bit unsigned integer.
};

enum TessellationFactorStepFunction
{
	tesselation_factor_step_function_constant,
	//A constant step function. For all instances, the tessellation factor for all patches in a patch draw call is at the offset location in the tessellation factor buffer.
	tesselation_factor_step_function_per_patch,
	//A per-patch step function. For all instances, the tessellation factor for all patches in a patch draw call is at the offset + (drawPatchIndex * tessellationFactorStride) location in the tessellation factor buffer.
	tesselation_factor_step_function_per_instance,
	//A per-instance step function. For a given instance ID, the tessellation factor for a patch in a patch draw call is at the offset + (instanceID * instanceStride) location in the tessellation factor buffer.
	tesselation_factor_step_function_per_patch_and_per_instance
        //A per-patch and per-instance step function. For a given instance ID, the tessellation factor for a patch in a patch draw call is at the offset + (drawPatchIndex * tessellationFactorStride + instanceID * instanceStride) location in the tessellation factor buffer.    
};

enum PixelFormat
{
    PixelFormatInvalid = 0,
    
    /* Normal 8 bit formats */
    PixelFormatA8Unorm      = 1,
    
    PixelFormatR8Unorm                            = 10,
    PixelFormatR8Unorm_sRGB /*API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos)*/ = 11,
    
    PixelFormatR8Snorm      = 12,
    PixelFormatR8Uint       = 13,
    PixelFormatR8Sint       = 14,
    
    /* Normal 16 bit formats */
    
    PixelFormatR16Unorm     = 20,
    PixelFormatR16Snorm     = 22,
    PixelFormatR16Uint      = 23,
    PixelFormatR16Sint      = 24,
    PixelFormatR16Float     = 25,
    
    PixelFormatRG8Unorm                            = 30,
    PixelFormatRG8Unorm_sRGB /*API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos)*/ = 31,
    PixelFormatRG8Snorm                            = 32,
    PixelFormatRG8Uint                             = 33,
    PixelFormatRG8Sint                             = 34,
    
    /* Packed 16 bit formats */
    PixelFormatB5G6R5Unorm = 40,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 40,
    PixelFormatA1BGR5Unorm = 41,// API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 41,
    PixelFormatABGR4Unorm = 42,//  API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 42,
    PixelFormatBGR5A1Unorm = 43,// API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 43,
    
    /* Normal 32 bit formats */
    
    PixelFormatR32Uint  = 53,
    PixelFormatR32Sint  = 54,
    PixelFormatR32Float = 55,
    
    PixelFormatRG16Unorm  = 60,
    PixelFormatRG16Snorm  = 62,
    PixelFormatRG16Uint   = 63,
    PixelFormatRG16Sint   = 64,
    PixelFormatRG16Float  = 65,
    
    PixelFormatRGBA8Unorm      = 70,
    PixelFormatRGBA8Unorm_sRGB = 71,
    PixelFormatRGBA8Snorm      = 72,
    PixelFormatRGBA8Uint       = 73,
    PixelFormatRGBA8Sint       = 74,
    
    PixelFormatBGRA8Unorm      = 80,
    PixelFormatBGRA8Unorm_sRGB = 81,
    
    /* Packed 32 bit formats */
    
    PixelFormatRGB10A2Unorm = 90,
    PixelFormatRGB10A2Uint  = 91,
    
    PixelFormatRG11B10Float = 92,
    PixelFormatRGB9E5Float = 93,
    
    PixelFormatBGR10A2Unorm = 94,//  API_AVAILABLE(macos(10.13), ios(11.0)) = 94,
    
    PixelFormatBGR10_XR = 554,//      API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos) = 554,
    PixelFormatBGR10_XR_sRGB = 555,// API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos) = 555,
    
    /* Normal 64 bit formats */
    
    PixelFormatRG32Uint  = 103,
    PixelFormatRG32Sint  = 104,
    PixelFormatRG32Float = 105,
    
    PixelFormatRGBA16Unorm  = 110,
    PixelFormatRGBA16Snorm  = 112,
    PixelFormatRGBA16Uint   = 113,
    PixelFormatRGBA16Sint   = 114,
    PixelFormatRGBA16Float  = 115,
    
    PixelFormatBGRA10_XR = 552,//      API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos) = 552,
    PixelFormatBGRA10_XR_sRGB = 553,// API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos) = 553,
    
    /* Normal 128 bit formats */
    
    PixelFormatRGBA32Uint  = 123,
    PixelFormatRGBA32Sint  = 124,
    PixelFormatRGBA32Float = 125,
    
    /* Compressed formats. */
#if OSX
    /* S3TC/DXT */
    PixelFormatBC1_RGBA = 130,//              API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 130,
    PixelFormatBC1_RGBA_sRGB = 131,//         API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 131,
    PixelFormatBC2_RGBA = 132,//              API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 132,
    PixelFormatBC2_RGBA_sRGB = 133,//         API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 133,
    PixelFormatBC3_RGBA = 134,//              API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 134,
    PixelFormatBC3_RGBA_sRGB = 135,//         API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 135,
    
    /* RGTC */
    PixelFormatBC4_RUnorm = 140,//            API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 140,
    PixelFormatBC4_RSnorm = 141,//            API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 141,
    PixelFormatBC5_RGUnorm = 142,//            API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 142,
    PixelFormatBC5_RGSnorm = 143,//           API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 143,
    
    /* BPTC */
    PixelFormatBC6H_RGBFloat = 150 ,//         API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 150,
    PixelFormatBC6H_RGBUfloat = 151,//        API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 151,
    PixelFormatBC7_RGBAUnorm = 152,//         API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 152,
    PixelFormatBC7_RGBAUnorm_sRGB = 153,//    API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 153,
    
#endif
    //TODO(Ray):Later fix these for ios
#if IOS
    /* PVRTC */
    PixelFormatPVRTC_RGB_2BPP        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 160,
    PixelFormatPVRTC_RGB_2BPP_sRGB   ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 161,
    PixelFormatPVRTC_RGB_4BPP        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 162,
    PixelFormatPVRTC_RGB_4BPP_sRGB   ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 163,
    PixelFormatPVRTC_RGBA_2BPP       ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 164,
    PixelFormatPVRTC_RGBA_2BPP_sRGB  ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 165,
    PixelFormatPVRTC_RGBA_4BPP       ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 166,
    PixelFormatPVRTC_RGBA_4BPP_sRGB  ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 167,
    
    /* ETC2 */
    PixelFormatEAC_R11Unorm          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 170,
    PixelFormatEAC_R11Snorm          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 172,
    PixelFormatEAC_RG11Unorm         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 174,
    PixelFormatEAC_RG11Snorm         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 176,
    PixelFormatEAC_RGBA8             ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 178,
    PixelFormatEAC_RGBA8_sRGB        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 179,
    
    PixelFormatETC2_RGB8             ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 180,
    PixelFormatETC2_RGB8_sRGB        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 181,
    PixelFormatETC2_RGB8A1           ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 182,
    PixelFormatETC2_RGB8A1_sRGB      ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 183,
    
    /* ASTC */
    PixelFormatASTC_4x4_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 186,
    PixelFormatASTC_5x4_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 187,
    PixelFormatASTC_5x5_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 188,
    PixelFormatASTC_6x5_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 189,
    PixelFormatASTC_6x6_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 190,
    PixelFormatASTC_8x5_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 192,
    PixelFormatASTC_8x6_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 193,
    PixelFormatASTC_8x8_sRGB         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 194,
    PixelFormatASTC_10x5_sRGB        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 195,
    PixelFormatASTC_10x6_sRGB        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 196,
    PixelFormatASTC_10x8_sRGB        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 197,
    PixelFormatASTC_10x10_sRGB       ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 198,
    PixelFormatASTC_12x10_sRGB       ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 199,
    PixelFormatASTC_12x12_sRGB       ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 200,
    
    PixelFormatASTC_4x4_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 204,
    PixelFormatASTC_5x4_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 205,
    PixelFormatASTC_5x5_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 206,
    PixelFormatASTC_6x5_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 207,
    PixelFormatASTC_6x6_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 208,
    PixelFormatASTC_8x5_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 210,
    PixelFormatASTC_8x6_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 211,
    PixelFormatASTC_8x8_LDR          ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 212,
    PixelFormatASTC_10x5_LDR         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 213,
    PixelFormatASTC_10x6_LDR         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 214,
    PixelFormatASTC_10x8_LDR         ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 215,
    PixelFormatASTC_10x10_LDR        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 216,
    PixelFormatASTC_12x10_LDR        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 217,
    PixelFormatASTC_12x12_LDR        ,//API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos) = 218,
#endif    
    
    /*!
      @constant PixelFormatGBGR422
      @abstract A pixel format where the red and green channels are subsampled horizontally.  Two pixels are stored in 32 bits, with shared red and blue values, and unique green values.
      @discussion This format is equivalent to YUY2, YUYV, yuvs, or GL_RGB_422_APPLE/GL_UNSIGNED_SHORT_8_8_REV_APPLE.   The component order, from lowest addressed byte to highest, is Y0, Cb, Y1, Cr.  There is no implicit colorspace conversion from YUV to RGB, the shader will receive (Cr, Y, Cb, 1).  422 textures must have a width that is a multiple of 2, and can only be used for 2D non-mipmap textures.  When sampling, ClampToEdge is the only usable wrap mode.
    */
    PixelFormatGBGR422 = 240,
    /*!
      @constant PixelFormatBGRG422
      @abstract A pixel format where the red and green channels are subsampled horizontally.  Two pixels are stored in 32 bits, with shared red and blue values, and unique green values.
      @discussion This format is equivalent to UYVY, 2vuy, or GL_RGB_422_APPLE/GL_UNSIGNED_SHORT_8_8_APPLE. The component order, from lowest addressed byte to highest, is Cb, Y0, Cr, Y1.  There is no implicit colorspace conversion from YUV to RGB, the shader will receive (Cr, Y, Cb, 1).  422 textures must have a width that is a multiple of 2, and can only be used for 2D non-mipmap textures.  When sampling, ClampToEdge is the only usable wrap mode.
    */
    PixelFormatBGRG422 = 241,
    /* Depth */
#if OSX
    PixelFormatDepth16Unorm = 250,//          API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios) = 250,
#endif
    PixelFormatDepth32Float  = 252,
    /* Stencil */
    PixelFormatStencil8        = 253,
    /* Depth Stencil */
#if OSX
    PixelFormatDepth24Unorm_Stencil8 = 255,//  API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 255,
#endif
    PixelFormatDepth32Float_Stencil8 = 260,  /*API_AVAILABLE(macos(10.11), ios(9.0))*/
    PixelFormatX32_Stencil8  = 261, //API_AVAILABLE(macos(10.12), ios(10.0)) = 261,
    PixelFormatX24_Stencil8  = 262,//API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios) = 262,
};

enum StorageMode
{
    StorageModeShared  = 0,
#if OSX || WINDOWS
    StorageModeManaged = 1,//API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios) = 1,
#endif
    StorageModePrivate = 2,
#if IOS
    StorageModeMemoryless = 3,// API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos) = 3,
#endif
};
//API_AVAILABLE(macos(10.11), ios(9.0));

enum VertexFormat : uint32_t
{
    VertexFormatInvalid = 0,
    
    VertexFormatUChar2 = 1,
    VertexFormatUChar3 = 2,
    VertexFormatUChar4 = 3,
    
    VertexFormatChar2 = 4,
    VertexFormatChar3 = 5,
    VertexFormatChar4 = 6,
    
    VertexFormatUChar2Normalized = 7,
    VertexFormatUChar3Normalized = 8,
    VertexFormatUChar4Normalized = 9,
    
    VertexFormatChar2Normalized = 10,
    VertexFormatChar3Normalized = 11,
    VertexFormatChar4Normalized = 12,
    
    VertexFormatUShort2 = 13,
    VertexFormatUShort3 = 14,
    VertexFormatUShort4 = 15,
    
    VertexFormatShort2 = 16,
    VertexFormatShort3 = 17,
    VertexFormatShort4 = 18,
    
    VertexFormatUShort2Normalized = 19,
    VertexFormatUShort3Normalized = 20,
    VertexFormatUShort4Normalized = 21,
    
    VertexFormatShort2Normalized = 22,
    VertexFormatShort3Normalized = 23,
    VertexFormatShort4Normalized = 24,
    
    VertexFormatHalf2 = 25,
    VertexFormatHalf3 = 26,
    VertexFormatHalf4 = 27,
    
    VertexFormatFloat = 28,
    VertexFormatFloat2 = 29,
    VertexFormatFloat3 = 30,
    VertexFormatFloat4 = 31,
    
    VertexFormatInt = 32,
    VertexFormatInt2 = 33,
    VertexFormatInt3 = 34,
    VertexFormatInt4 = 35,
    
    VertexFormatUInt = 36,
    VertexFormatUInt2 = 37,
    VertexFormatUInt3 = 38,
    VertexFormatUInt4 = 39,
    
    VertexFormatInt1010102Normalized = 40,
    VertexFormatUInt1010102Normalized = 41,
    
    //Try not to use
    /*
     * MTLVertexFormatUChar API_AVAILABLE(macos(10.13), ios(11.0)) = 45,
    MTLVertexFormatChar API_AVAILABLE(macos(10.13), ios(11.0)) = 46,
    MTLVertexFormatUCharNormalized API_AVAILABLE(macos(10.13), ios(11.0)) = 47,
    MTLVertexFormatCharNormalized API_AVAILABLE(macos(10.13), ios(11.0)) = 48,
    
    MTLVertexFormatUShort API_AVAILABLE(macos(10.13), ios(11.0)) = 49,
    MTLVertexFormatShort API_AVAILABLE(macos(10.13), ios(11.0)) = 50,
    MTLVertexFormatUShortNormalized API_AVAILABLE(macos(10.13), ios(11.0)) = 51,
    MTLVertexFormatShortNormalized API_AVAILABLE(macos(10.13), ios(11.0)) = 52,
    
    MTLVertexFormatHalf API_AVAILABLE(macos(10.13), ios(11.0)) = 53,
     */
};

enum VertexStepFunction
{
	step_function_constant,
	//The vertex function fetches attribute data once and uses that data for every vertex.
	step_function_per_vertex,
	//The vertex function fetches and uses new attribute data for every vertex.
	step_function_per_instance,
	//The vertex function regularly fetches new attribute data for a number of instances that is determined by stepRate.
	step_function_per_patch,
	//The post-tessellation vertex function fetches data based on the patch index of the patch.
	step_function_per_patch_control_point,
	//The post-tessellation vertex function fetches data based on the control-point indices associated with the patch.
};

struct VertexAttributeDescriptor
{
	VertexFormat format;
	//The format of the vertex attribute.
	int offset;
	//The location of an attribute in vertex data, determined by the byte offset from the start of the vertex data.
	int buffer_index;
	//The index in the argument table for the associated vertex buffer.    
};

struct VertexBufferLayoutDescriptor
{
	VertexStepFunction step_function;
	//The circumstances under which the vertex and its attributes are presented to the vertex function.
	int step_rate;
	//The interval at which the vertex and its attributes are presented to the vertex function.
	int stride;
	//distance, in bytes, between the attribute data of two vertices in the buffer.    
};

//NOTE(Ray):TODO(Ray):This is wasteful change it to a stretchy vector
#define MAX_VERTEX_DESCRIPTORS 10

struct VertexDescriptor
{
    uint32_t attribute_count;
    uint32_t layout_count;
	VertexAttributeDescriptor attributes[MAX_VERTEX_DESCRIPTORS];
	//An array of state data that describes how vertex attribute data is stored in memory and is mapped to arguments for a vertex shader function.
	VertexBufferLayoutDescriptor layouts[MAX_VERTEX_DESCRIPTORS];
	//An array of state data that describes how data are fetched by a vertex shader function when rendering primitives.
    void* state;
};

enum Mutability
{
	default_,
	//Determines a default mutability option depending on the buffer type.
	mutable_,
	//Determines that a buffer's contents can be modified.
	immutable
        //Determines that a buffer's contents cannot be modified.    
};

//TODO(Ray):Finish filling up mutability and function structs for arugment
//lookup etc... for now this is not supported.
struct PiplineBufferDescriptorArray
{
};

struct ShaderFunction
{
	void* function;   //pointer to byte codes.
};

enum BlendFactor
{
    BlendFactorZero = 0,
    BlendFactorOne = 1,
    BlendFactorSourceColor = 2,
    BlendFactorOneMinusSourceColor = 3,
    BlendFactorSourceAlpha = 4,
    BlendFactorOneMinusSourceAlpha = 5,
    BlendFactorDestinationColor = 6,
    BlendFactorOneMinusDestinationColor = 7,
    BlendFactorDestinationAlpha = 8,
    BlendFactorOneMinusDestinationAlpha = 9,
    BlendFactorSourceAlphaSaturated = 10,
    BlendFactorBlendColor = 11,
    BlendFactorOneMinusBlendColor = 12,
    BlendFactorBlendAlpha = 13,
    BlendFactorOneMinusBlendAlpha = 14,
    BlendFactorSource1Color              /*API_AVAILABLE(macos(10.12), ios(10.11))*/ = 15,
    BlendFactorOneMinusSource1Color      /*API_AVAILABLE(macos(10.12), ios(10.11))*/ = 16,
    BlendFactorSource1Alpha              /*API_AVAILABLE(macos(10.12), ios(10.11))*/ = 17,
    BlendFactorOneMinusSource1Alpha      /*API_AVAILABLE(macos(10.12), ios(10.11))*/ = 18
};

enum BlendOperation
{
    BlendOperationAdd = 0,
    BlendOperationSubtract = 1,
    BlendOperationReverseSubtract = 2,
    BlendOperationMin = 3,
    BlendOperationMax = 4,  
};

enum ColorWriteMask
{
    ColorWriteMaskNone  = 0,
    ColorWriteMaskRed   = 0x1 << 3,
    ColorWriteMaskGreen = 0x1 << 2,
    ColorWriteMaskBlue  = 0x1 << 1,
    ColorWriteMaskAlpha = 0x1 << 0,
    ColorWriteMaskAll   = 0xf
};

struct RenderPipelineColorAttachmentDescriptor
{
    PixelFormat pixelFormat;
    //The pixel format of the color attachment’s texture.
    
    ColorWriteMask writeMask;
    //A bitmask that restricts which color channels are written into the texture.
    
    //Controlling the Blend Operation
    bool blendingEnabled;
    //A Boolean value that determines whether blending is enabled.
    
    BlendOperation alphaBlendOperation;
    //The blend operation assigned for the alpha data.
    
    BlendOperation rgbBlendOperation;
    //The blend operation assigned for the RGB data.
    
    //Specifying Blend Factors
    BlendFactor destinationAlphaBlendFactor;
    //The destination blend factor (DBF) used by the alpha blend operation.
    
    BlendFactor destinationRGBBlendFactor;
    //The destination blend factor (DBF) used by the RGB blend operation.
    
    BlendFactor sourceAlphaBlendFactor;
    //The source blend factor (SBF) used by the alpha blend operation.
    
    BlendFactor sourceRGBBlendFactor;
    //The source blend factor (SBF) used by the RGB blend operation.
    
    /*Constants
MTLBlendOperation
For every pixel, MTLBlendOperation determines how to combine and weight the source fragment values with the destination values. Some blend operations multiply the source values by a source blend factor (SBF), multiply the destination values by a destination blend factor (DBF), and then combine the results using addition or subtraction. Other blend operations use either a minimum or maximum function to determine the result.

MTLBlendFactor
The source and destination blend factors are often needed to complete specification of a blend operation. In most cases, the blend factor for both RGB values (F(rgb)) and alpha values (F(a)) are similar to one another, but in some cases, such as MTLBlendFactorSourceAlphaSaturated, the blend factor is slightly different. Four blend factors (MTLBlendFactorBlendColor, MTLBlendFactorOneMinusBlendColor, MTLBlendFactorBlendAlpha, and MTLBlendFactorOneMinusBlendAlpha) refer to a constant blend color value that is set by the 
setBlendColorRed:green:blue:alpha:
 method of MTLRenderCommandEncoder.
 
MTLColorWriteMask
Values used to specify a mask to permit or restrict writing to color channels of a color value. The values 
MTLColorWriteMaskRed
, 
MTLColorWriteMaskGreen
, 
MTLColorWriteMaskBlue
, and 
MTLColorWriteMaskAlpha
 select one color channel each, and they can be bitwise combined.
    */
};

struct RenderPipelineColorAttachmentDescriptorArray
{
    RenderPipelineColorAttachmentDescriptor i[4];
};

struct RenderPipelineStateDesc
{
	void* state;
	//    func reset()
	//Specifies the default rendering pipeline state values for the descriptor.
    //TODO(Ray):Will propbably get rid of this
    //	YoyoVector color_render_targets;// RenderTargetView ; //option: max configurable render targets for device
	//var colorAttachments: MTLRenderPipelineColorAttachmentDescriptorArray
    //An array of attachments that store color data.
    //	YoyoVector depth_render_targets;// RenderTargetView ; //option: max configurable render targets for device
    
    RenderPipelineColorAttachmentDescriptorArray color_attachments;
    
    PixelFormat depthAttachmentPixelFormat;
    //The pixel format of the attachment that stores depth data.
	//YoyoVector stencil_render_targets;// RenderTargetView ; //option: max configurable render targets for device
    
    PixelFormat stencilAttachmentPixelFormat;
    //The pixel format of the attachment that stores stencil data.
    
    //Might change that later.
	void* vertex_function;
	//A programmable function that processes individual vertices in a rendering pass.
	void* fragment_function;
    
    VertexDescriptor vertex_descriptor;
    
    //TODO(Ray):allow setting the mutability options for buffers but at the moment not used
    //    PiplineBufferDescriptorArray vertex_buffers;
    //    An array that contains the buffer mutability options for a render pipeline's vertex function.
    //    PiplineBufferDescriptorArray fragment_buffers;
    //    An array that contains the buffer mutability options for a render pipeline's fragment function.
    
	int sample_count;//: Int//multi-sample count
    //The number of samples in each fragment.
	bool is_alpha_coverage_enabled;//: Bool
    //Indicates whether the alpha channel fragment output for 
    //colorAttachments
    // is read and used to compute a sample coverage mask.
	bool is_alpha_to_one_enabled;//: Bool
    //Indicates whether the alpha channel values for 
    //colorAttachments
    // are forced to 1.0, which is the largest representable value.
	bool isRasterizationEnabled;//: Bool
    //Determines whether primitives are rasterized.
	PrimitiveTopologyClass inputPrimitiveTopology;//: MTLPrimitiveTopologyClass
    //Indicates the type of primitive topology being rendered.
	int rasterSampleCount;//: Int
    //The number of samples in each fragment.
    
	bool supportIndirectCommandBuffers;//: Bool
    //A Boolean value that determines whether the render pipeline state can be used in an indirect command buffer.
	int maxTessellationFactor;//: Int
    //Specifies the maximum tessellation factor to be used by the tessellator when tessellating a patch (or patches).
	bool isTessellationFactorScaleEnabled;//: Bool
    //Determines whether the tessellation factor is scaled.
	TessellationFactorFormat tessellationFactorFormat;//: MTLTessellationFactorFormat
    //The format of the tessellation factors specified in the tessellation factor buffer.
	TessellationControlPointIndexType tessellationControlPointIndexType;//: MTLTessellationControlPointIndexType
    //The size of the control point indices in a control point index buffer.
	TessellationFactorStepFunction tessellationFactorStepFunction;//: MTLTessellationFactorStepFunction
    //The step function used to determine the tessellation factors for a patch from the tessellation factor buffer.
	WindingOrder tessellationOutputWindingOrder;//: MTLWinding
    //The winding order of triangles output by the tessellator.
	TessellationPartitionMode tessellationPartitionMode;//: MTLTessellationPartitionMode
    //The partitioning mode used by the tessellator to derive the number and spacing of segments used to subdivide a corresponding edge.
    //Identifying Properties
	//Yostr label;
	//A string that identifies this object.
};

struct RenderPipelineState
{
	//RenderDevice device;
	//struct string label;
	RenderPipelineStateDesc desc;
    
	//    Using Tile Functions
	int max_total_threads_per_group;//: Int
    //The maximum total number of threads that can be in a single threadgroup.
    //Required.
    
	bool thread_group_size_matches_tile_size;// threadgroupSizeMatchesTileSize: Bool
    //A Boolean indicating that the pipeline state requires a threadgroup size equal to the tile size.
    //Required.
    //    Using Imageblocks
	int image_block_sample_length;// imageblockSampleLength: Int
    //The size, in bytes, of the render pipeline's imageblock for a single sample.
    //Required.
    
    //func imageblockMemoryLength(forDimensions: MTLSize) -> Int
    //Returns the imageblock memory length for the given imageblock dimensions.
    //Required.
    //Indirect Command Buffers
	bool support_indirect_command_buffers;// supportIndirectCommandBuffers: Bool
    //A boolean you set to indicate your desire to use indirect command buffers.
    //Required.    
	void* shader_signature;
    void* state;
    uint64_t hash;
};

enum StencilOp
{
	stencil_op_keep,
	//Keep the current stencil value.
	stencil_op_zero,
	//Set the stencil value to zero.
	stencil_op_replace,
	//Replace the stencil value with the stencil reference value, which is set by the
	//setStencilReferenceValue(_:)
	//method of
	//MTLRenderCommandEncoder
	stencil_op_incrementClamp,
	//If the current stencil value is not the maximum representable value, increase the stencil value by one.Otherwise, if the current stencil value is the maximum representable value, do not change the stencil value.
	stencil_op_decrementClamp,
	//If the current stencil value is not zero, decrease the stencil value by one.Otherwise, if the current stencil value is zero, do not change the stencil value.
	stencil_op_invert,
	//Perform a logical bitwise invert operation on the current stencil value.
	stencil_op_incrementWrap,
	//If the current stencil value is not the maximum representable value, increase the stencil value by one.Otherwise, if the current stencil value is the maximum representable value, set the stencil value to zero.
	stencil_op_decrementWrap,
	//If the current stencil value is not zero, decrease the stencil value by one.Otherwise, if the current stencil value is zero, set the stencil value to the maximum representable value.
};

enum TriangleFillMode
{
	triangle_fill_mode_fill,
	//Rasterize triangle and triangle strip primitives as filled triangles.
	triange_fill_mode_lines,
	//Rasterize triangle and triangle strip primitives as lines.
};

enum CullMode
{
	cull_mode_none,
	//Does not cull any primitives.
	cull_mode_front,
	//Culls front - facing primitives.
	cull_mode_back
        //Culls back - facing primitives.
};

enum DepthClipMode
{
	depth_clip_mode_clip,
	//Clip fragments outside the near or far planes.
	depth_clip_mode_clamp
        //Clamp fragments outside the near or far planes.
};

struct StencilDesc
{
	StencilOp stencilFailureOperation;
	//The operation that is performed to update the values in the stencil attachment when the stencil test fails.
	StencilOp depthFailureOperation;// : MTLStencilOperation
    //The operation that is performed to update the values in the stencil attachment when the stencil test passes, but the depth test fails.
	StencilOp depthStencilPassOperation;// : MTLStencilOperation
    //The operation that is performed to update the values in the stencil attachment when both the stencil test and the depth test pass.
	CompareFunc stencilCompareFunction;// : MTLCompareFunction
    //The comparison that is performed between the masked reference value and a masked value in the stencil attachment.
    //The readMask bits are used for logical AND operations to both the stored stencil value and the reference value.
    //The least significant bits of the read mask are used. The default value is all ones. A logical AND operation with the default readMask does not change the value.
    uint32_t write_mask = 0xFFFFFFFF;
    uint32_t read_mask  = 0xFFFFFFFF;
    bool enabled;//NOTE(Ray):IMPORTANT(RAY):This value does not exist in the original metal api it is a byproduct of the fact we dont
    //want to make this an object. The original api states when this object isnil that than stencil testing is off.
};

struct StencilReferenceValues
{
	float front;
	//The front stencil reference value.
	float back;
	//The back stencil reference value.
    //The default value is 0.
};

//TODO(Ray):We probably will end up just keeping the description around and removing the properties
//on the actual state object ....
struct DepthStencilDescription
{
    CompareFunc depthCompareFunction;
    //The comparison that is performed between a fragment’s depth value and the depth value in the attachment, which determines whether to discard the fragment.
    bool depthWriteEnabled;
    //A Boolean value that indicates whether depth values can be written to the depth attachment.
    //Specifying Stencil Descriptors for Primitives
    StencilDesc backFaceStencil;
    //The stencil descriptor for back-facing primitives.
    StencilDesc frontFaceStencil;
    //The stencil descriptor for front-facing primitives.
    //Identifying Properties
    //    char* label;
    //A string that identifies this object.
    void* state;
};

enum CPUCacheMode
{
    CPUCacheModeDefaultCache,
    //The default CPU cache mode for the resource. Guarantees that read and write operations are executed in the expected order.
    CPUCacheModeWriteCombined
        //A write-combined CPU cache mode for the resource. Optimized for resources that the CPU will write into, but never read.  
};

#define ResourceCPUCacheModeShift            0
#define ResourceCPUCacheModeMask             (0xfUL << ResourceCPUCacheModeShift)

#define ResourceStorageModeShift             4
#define ResourceStorageModeMask              (0xfUL << ResourceStorageModeShift)

#define ResourceHazardTrackingModeShift      8
#define ResourceHazardTrackingModeMask       (0x1UL << ResourceHazardTrackingModeShift)

enum ResourceOptions
{
    ResourceCPUCacheModeDefaultCache = CPUCacheModeDefaultCache  << ResourceCPUCacheModeShift,
    //The default CPU cache mode for the resource. Guarantees that read and write operations are executed in the expected order.
    ResourceCPUCacheModeWriteCombined = CPUCacheModeWriteCombined << ResourceCPUCacheModeShift,
    //A write-combined CPU cache mode for the resource. Optimized for resources that the CPU will write into, but never read.
    ResourceStorageModeShared = StorageModeShared << ResourceStorageModeShift,
#if OSX
    //The resource is stored in system memory accessible to both the CPU and the GPU.
    ResourceStorageModeManaged = StorageModeManaged << ResourceStorageModeShift,
#endif
    //The resource exists as a synchronized memory pair with one copy stored in system memory accessible to the CPU and another copy stored in video memory accessible to the GPU.
    ResourceStorageModePrivate = StorageModePrivate << ResourceStorageModeShift,
    //The resource is stored in memory only accessible to the GPU. In iOS and tvOS, the resource is stored in system memory. In macOS, the resource is stored in video memory.
#if IOS
    ResourceStorageModeMemoryless = StorageModeMemoryless << ResourceStorageModeShift,
#endif
    //The resource is stored in on-tile memory, without CPU or GPU memory backing. The contents of the on-tile memory are undefined and do not persist; the only way to populate the resource is to render into it. Memoryless resources are limited to temporary render targets (in effect,
    //MTLTexture
    // objects configured with a
    //MTLTextureDescriptor
    // object and used with a
    //MTLRenderPassAttachmentDescriptor
    // object).
    ResourceHazardTrackingModeUntracked = 0x1UL << ResourceHazardTrackingModeShift,
    //Command encoder dependencies for this resource are tracked manually with
    //MTLFence
    // objects. This value is always set for resources sub-allocated from a
    //MTLHeap
    // object and may optionally be specified for non-heap resources.
    //    ResourceOptionCPUCacheModeDefault,
    //This constant was deprecated in iOS 9.0 and OS X 10.11. Use MTLResourceCPUCacheModeDefaultCache instead.
    //    MTLResourceOptionCPUCacheModeWriteCombined,
    //This constant was deprecated in iOS 9.0 and macOS 10.11. Use MTLResourceCPUCacheModeWriteCombined instead.
};

enum TextureType
{
    TextureType1D,
    //A one-dimensional texture image.
    TextureType1DArray,
    //An array of one-dimensional texture images.
    TextureType2D,
    //A two-dimensional texture image.
    TextureType2DArray,
    //An array of two-dimensional texture images.
    TextureType2DMultisample,
    //A two-dimensional texture image that uses more than one sample for each pixel.
    TextureTypeCube,
    //A cube texture with six two-dimensional images.
    TextureTypeCubeArray,
    //An array of cube textures, each with six two-dimensional images.
    TextureType3D,
    //A three-dimensional texture image.
};

struct TextureDescriptor
{
    TextureType textureType;
    //The dimension and arrangement of texture image data.
    PixelFormat pixelFormat;
    //Format that determines how a pixel is written to, stored, and read from the storage allocation of the texture.
    uint32_t width;
    //The width of the texture image for the base level mipmap, in pixels.
    uint32_t height;
    //The height of the texture image for the base level mipmap, in pixels.
    uint32_t depth;
    //The depth of the texture image for the base level mipmap, in pixels.
    uint32_t mipmapLevelCount;
    ///The number of mipmap levels for this texture.
    uint32_t sampleCount;
    //The number of samples in each fragment.
    uint32_t arrayLength;
    //The number of array elements for this texture.
    /*
     * The value of this property must be between 1 and 2048, inclusive. The default value is 1.
This value is 1 if the texture type is not an array.
This value can be between 1 and 2048 if the texture type is one of the following array types:
MTLTextureType1DArray
MTLTextureType2DArray
MTLTextureType2DMultisampleArray
MTLTextureTypeCubeArray
     */
    ResourceOptions resourceOptions;
    //The behavior of a new memory allocation.
    CPUCacheMode cpuCacheMode;
    //The CPU cache mode used for the CPU mapping of the texture.
    StorageMode storageMode;
    //The storage mode used for the location and mapping of the texture.
    bool allowGPUOptimizedContents;
    //A Boolean value indicating whether the GPU is allowed to adjust the contents of this texture to improve GPU performance.
    TextureUsage usage;
    //Describes how the texture will be used in your app.
    void* state;//the native descriptor object.
};

struct Texture
{
    TextureDescriptor descriptor;
    bool framebufferOnly;
    bool shareable;
    void* state;
    //NOTE(Ray):TODO(Ray):Make this a debug only thing we added on for debugging purposes.
    bool is_released;
};

struct DepthStencilState
{
    DepthStencilDescription desc;
	//struct string label;
	//A string that identifies this object.
    //	CompareFunc depth_compare_function;
    //	StencilDesc backFaceStencil;
    //	StencilDesc frontFaceStencil;
    //	bool is_depth_write_enabled;
	void* state;
};

struct DepthBiasState
{
	float depth_bias;
	float slope_scale;
	float clamp;
};

struct Viewport
{
	double origin_x;
	//The x coordinate of the upper - left corner of the viewport.
	double origin_y;
	//The y coordinate of the upper - left corner of the viewport.
	double width;
	//The width of the viewport, in pixels.
	double height;
	//The height of the viewport, in pixels.
	double znear;
	//The z coordinate of the near clipping plane of the viewport.
	double zfar;
	//The z coordinate of the far clipping plane of the viewport.
};

struct ScissorRect
{
	int x;
	//The x window coordinate of the upper - left corner of the scissor rectangle.
	int y;
	//The y window coordinate of the upper - left corner of the scissor rectangle.
    int width;
	//The height of the scissor rectangle, in pixels.
	int height;
    //width of the scissor rectangle, in pixels.
};

enum VisiblityResultMode
{
	visibility_result_mode_disabled,
	//Does not monitor the samples that pass the depth and stencil tests.
	visiblity_result_mode_boolean,
	//	Indicates whether samples pass the depth and stencil tests.
	visiblity_result_mode_counting
        //Counts the samples that pass the depth and stencil tests.
};

enum LoadAction
{
    LoadActionDontCare,
    //Each pixel in the attachment is allowed to take on any value at the start of the rendering pass.
    LoadActionLoad,
    //The existing contents of the texture are preserved.
    LoadActionClear,
    //A value is written to every pixel in the specified attachment.
};

enum StoreAction
{
    StoreActionDontCare,
    //After the rendering pass is complete, the attachment is left in an undefined state. This may improve performance because it enables the implementation to avoid any work necessary to store the rendering results.
    StoreActionStore,
    //The final results of the rendering pass are stored in the attachment.
    StoreActionMultisampleResolve,
    //The multisample values are resolved into single sample values that are stored in the texture specified by the 
    //resolveTexture
    // property. The contents of the attachment are left undefined.
    StoreActionStoreAndMultisampleResolve,
    //The multisample values are stored in the attachment and also resolved into single sample values that are stored in the texture specified by the 
    //resolveTexture
    //property.
    StoreActionUnknown,
    //This is a temporary store action only to be used if you do not know the attachment’s store action up front. Use this value to defer your decision until after the render command encoder is created. You must specify a known store action before you finish encoding commands into the render command encoder. Refer to the 
    //MTLRenderCommandEncoder
    // and MTLParallelRenderCommandEncoder,
    // protocol references for further information.
    StoreActionCustomSampleDepthStore,
    //A render target action that stores depth data in a sample-position-agnostic representation.    
};

enum StoreActionOptions
{
    StoreActionOptionNone,
    //An option that doesn modify the intended behavior of a store action.
    StoreActionOptionCustomSamplePositions,
    //An option that stores data in a sample-position-agnostic representation.
    //Using Programmable Sample Positions
    //    StoreActionOptionCustomSamplePositions,
    //An option that stores data in a sample-position-agnostic representation.
};

struct RenderPassAttachmentDescriptor
{
    Texture texture;
    //The texture object associated with this attachment.
    uint32_t level;
    ///The mipmap level of the texture used for rendering to the attachment.
    uint32_t slice;
    //The slice of the texture used for rendering to the attachment.
    uint32_t depthPlane;
    //The depth plane of the texture used for rendering to the attachment.
    //Specifying Rendering Pass Actions
    LoadAction loadAction;
    //The action performed by this attachment at the start of a rendering pass for a render command encoder.
    StoreAction storeAction;
    //The action performed by this attachment at the end of a rendering pass for a render command encoder.
    StoreActionOptions storeActionOptions;
    //The options that modify the store action performed by this attachment.
    //Specifying the Texture to Resolve Multisample Data
    Texture resolveTexture;
    //The destination texture used when multisampled texture data is resolved into single sample values.
    uint32_t resolveLevel;
    //The mipmap level of the texture used for the multisample resolve action.
    uint32_t resolveSlice;
    //The slice of the texture used for the multisample resolve action.
    uint32_t resolveDepthPlane;
    //The depth plane of the texture used for the multisample resolve action.
};

enum MultisampleDepthResolveFilter
{
    MultisampleDepthResolveFilterSample0,
    //No filter is applied. This is the default case.
    MultisampleDepthResolveFilterMin,
    //A minimum filter is applied. All depth samples within the pixel are compared, and the minimum value is selected.
    MultisampleDepthResolveFilterMax,
    //A maximum filter is applied. All depth samples within the pixel are compared, and the maximum value is selected.
};

struct RenderPassColorAttachmentDescriptor
{
    RenderPassAttachmentDescriptor description;
    f4 clear_color;
    void* state;
};

struct RenderPassDepthAttachmentDescriptor
{
    RenderPassAttachmentDescriptor description;
    float clear_depth;
    MultisampleDepthResolveFilter resolve_filter;
    void* state;
};

struct RenderPassStencilAttachmentDescriptor
{
    RenderPassAttachmentDescriptor description;
    float clearStencil;
    //The stencil value to use when the stencil attachment is cleared.
    //Instance Properties
    float stencilResolveFilter;
    void* state;
};

//NOTE(Ray):To use visibility testing, your render pass descriptor must include a buffer in which to store results.See visibilityResultBuffer.
struct VisbilityResult
{
	VisiblityResultMode mode;
	int offset;
	//The offset within the visibility buffer where the visibility result should be stored. Must be a multiple of 8 bytes.
};

//NOTE(Ray):Encoder job here is to emulate the encoder for metal on all platforms.
struct RenderCommandEncoder//Encoder state
{
	void* state;
    //NOTE(Ray):Most of these dont beling here probably should be on the material.
    //or pipeline state.
	RenderPipelineState pipeline_state;
	TriangleFillMode fill_mode;
	WindingOrder triangle_winding_order;
	CullMode cull_mode;
	DepthStencilState depth_stencil_state;
	DepthBiasState depth_bias_state;
	DepthClipMode clip_mode;
	StencilReferenceValues stencil_reference_values;
	Viewport viewport;
	f4 blend_color;//rgba format
	VisbilityResult visiblity_result;
	//TODO(Ray):Support for below argument buffers when allowed by device
	/*
 func useResource(MTLResource, usage: MTLResourceUsage)
  Adds an untracked resource to the render pass.
  Required.
  
  func useResources([MTLResource], usage: MTLResourceUsage)
  Adds an array of untracked resources to the render pass.
  
  func useHeap(MTLHeap)
  Adds the resources in a heap to the render pass.
  
  Required.
  
  func useHeaps([MTLHeap])
  Adds the resources in an array of heaps to the render pass.
 */
    
	/*
 //NOTE(Ray):Originally the metal command encoder would have api for taking in vertex function resources.
 //but we are not going to do that as we leave that ot the command buffer abstraction fo rnow.
 Specifying Resources for a Vertex Function
 */
    
	/*
 //NOTE(Ray):Originally the metal command encoder would have api for taking in fragment function resources.
 //but we are not going to do that as we leave that ot the command buffer abstraction fo rnow.
 Specifying Resources for a Fragment Function
 */
    
	/*
 //NOTE(Ray):Originally the metal command encoder would have api for taking in tesselation factors.
 //but we are not going to do that as we leave that ot the command buffer abstraction fo rnow.
 Specifying Tessellation Factors
 */
};

struct RenderPassDescriptor
{
    //    Attachments for a Rendering Pass
    FMJStretchBuffer colorAttachments;//of render pass attachments
    //An array of state information for attachments that store color data.
    RenderPassDepthAttachmentDescriptor depth_attachment;
    //State information for an attachment that stores depth data.
    RenderPassStencilAttachmentDescriptor stencil_attachment;
    //State information for an attachment that stores stencil data.
    //Specifying the Visibility Result Buffer
    void* visibility_result_buffer;
    //The destination for the GPU to write visibility information when samples pass the depth and stencil tests.
    //Layered Rendering
    uint32_t renderTargetArrayLength;
    //The number of active layers that all attachments must have for layered rendering.
    uint32_t renderTargetWidth;
    //The width, in pixels, to constain the render target to.
    uint32_t renderTargetHeight;
    //The height, in pixels, to constain the render target to.
    //Using Programmable Sample Positions
    //    SamplePosition
    //A sample position on a subpixel grid.
    //TODO(Ray):Implement these functions
    //   SamplePositionMake
    //Returns a new sample position on a subpixel grid.
    /*
    - setSamplePositions:count:
    Sets the programmable sample positions for a render pass.
    - getSamplePositions:count:
    Retrieves the programmable sample positions set for a render pass.
    */
    //Specifying Tile Shading Parameters
    uint32_t image_block_sample_length;
    //The per-sample size, in bytes, of the largest explicit imageblock layout in the render pass.
    uint32_t threadgroupMemoryLength;
    //The per-tile size, in bytes, of the persistent threadgroup memory allocation.
    uint32_t tileWidth;
    //The tile width, in pixels.
    uint32_t tileHeight;
    //The tile height, in pixels.
    //Specifying Sample Counts
    uint32_t defaultRasterSampleCount;
    //The raster sample count for the render pass when no attachments are given.
    void* state;
};

struct Drawable
{
    Texture texture;
    //    Layer layer;
    void* state;
};

//NOTE(Ray):This is for doing interops with opengl.
//Was added for porting old gl code to metalize api.
struct GLInterOpTexture
{
    Texture texture;
    void* state;
};

#if 0
struct HeapDescriptor
{
    StorageMode storage_mode;
    //The storage mode for the heap.
    CPUCacheMode cpu_cache_mode;
    //The CPU cache mode for the heap.
    int size;
    //The size for the heap, in bytes.
};

struct Heap
{
    RenderDevice device;
    //The device that created this heap.
    //Required.
    Yostr label;
    //A string that identifies this object.
    //Required.
    //Querying Heap Properties
    StorageMode storage_mode;
    //The storage mode of the heap.
    //Required.
    CPUCacheMode cpu_cache_mode;
    //The CPU cache mode of the heap.
    //Required.
    int size;
    //The total size of the heap, in bytes.
    //Required.
    int used_size;
    //The size of all resources sub-allocated from the heap, in bytes.
    //Required.
    int current_allocated_size;//currentAllocatedSize
    //The size, in bytes, of the current heap allocation.
    //Required.
    int max_available_size_with_alignment;//maxAvailableSizeWithAlignment:
    //he maximum size, in bytes, that can be successfully sub-allocated from the heap for a resource with the given alignment.
    //Required.
};

struct SizeAndAlign
{
    int size;
    //The size of the resource, in bytes.
    int align;
    //The alignment of the resource, in bytes.
};

enum PurgeableState
{
    MTLPurgeableStateKeepCurrent = 1,
    MTLPurgeableStateNonVolatile = 2,
    MTLPurgeableStateVolatile = 3,
    MTLPurgeableStateEmpty = 4,
};// API_AVAILABLE(macos(10.11), ios(8.0));
#endif

struct CaptureManager
{
    void* state;
};

struct CaptureScope
{
    void* state;
};

enum FeatureSet
{
#if IOS || OSX
    FeatureSet_iOS_GPUFamily1_v1/* API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos, tvos) */ = 0,
    FeatureSet_iOS_GPUFamily2_v1/* API_AVAILABLE(ios(8.0)) API_UNAVAILABLE(macos, tvos) */ = 1,
    
    FeatureSet_iOS_GPUFamily1_v2/* API_AVAILABLE(ios(9.0)) API_UNAVAILABLE(macos, tvos) */ = 2,
    FeatureSet_iOS_GPUFamily2_v2/* API_AVAILABLE(ios(9.0)) API_UNAVAILABLE(macos, tvos) */ = 3,
    FeatureSet_iOS_GPUFamily3_v1/* API_AVAILABLE(ios(9.0)) API_UNAVAILABLE(macos, tvos) */ = 4,
    
    FeatureSet_iOS_GPUFamily1_v3/* API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos, tvos) */ = 5,
    FeatureSet_iOS_GPUFamily2_v3/* API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos, tvos) */ = 6,
    FeatureSet_iOS_GPUFamily3_v2/* API_AVAILABLE(ios(10.0)) API_UNAVAILABLE(macos, tvos) */ = 7,
    
    FeatureSet_iOS_GPUFamily1_v4/* API_AVAILABLE(ios(11.0)) API_UNAVAILABLE(macos, tvos) */ = 8,
    FeatureSet_iOS_GPUFamily2_v4/* API_AVAILABLE(ios(11.0)) API_UNAVAILABLE(macos, tvos) */ = 9,
    FeatureSet_iOS_GPUFamily3_v3/* API_AVAILABLE(ios(11.0)) API_UNAVAILABLE(macos, tvos) */ = 10,
    FeatureSet_iOS_GPUFamily4_v1/* API_AVAILABLE(ios(11.0)) API_UNAVAILABLE(macos, tvos) */ = 11,
    
    FeatureSet_iOS_GPUFamily1_v5/* API_AVAILABLE(ios(12.0)) API_UNAVAILABLE(macos, tvos) */ = 12,
    FeatureSet_iOS_GPUFamily2_v5/* API_AVAILABLE(ios(12.0)) API_UNAVAILABLE(macos, tvos) */ = 13,
    FeatureSet_iOS_GPUFamily3_v4/* API_AVAILABLE(ios(12.0)) API_UNAVAILABLE(macos, tvos) */ = 14,
    FeatureSet_iOS_GPUFamily4_v2/* API_AVAILABLE(ios(12.0)) API_UNAVAILABLE(macos, tvos) */ = 15,
    FeatureSet_iOS_GPUFamily5_v1/* API_AVAILABLE(ios(12.0)) API_UNAVAILABLE(macos, tvos) */ = 16,
    
    FeatureSet_macOS_GPUFamily1_v1/* API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios)*/ = 10000,
    FeatureSet_OSX_GPUFamily1_v1/* API_AVAILABLE(macos(10.11)) API_UNAVAILABLE(ios)*/ = FeatureSet_macOS_GPUFamily1_v1, // deprecated
    
    FeatureSet_macOS_GPUFamily1_v2/* API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios)*/ = 10001,
    FeatureSet_OSX_GPUFamily1_v2/* API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios)*/ = FeatureSet_macOS_GPUFamily1_v2, // deprecated
    FeatureSet_macOS_ReadWriteTextureTier2 /*API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios)*/ = 10002,
    FeatureSet_OSX_ReadWriteTextureTier2 /*API_AVAILABLE(macos(10.12)) API_UNAVAILABLE(ios)*/ = FeatureSet_macOS_ReadWriteTextureTier2, // deprecated
    
    FeatureSet_macOS_GPUFamily1_v3/* API_AVAILABLE(macos(10.13)) API_UNAVAILABLE(ios)*/ = 10003,
    
    FeatureSet_macOS_GPUFamily1_v4/* API_AVAILABLE(macos(10.14)) API_UNAVAILABLE(ios)*/ = 10004,
    FeatureSet_macOS_GPUFamily2_v1/* API_AVAILABLE(macos(10.14)) API_UNAVAILABLE(ios)*/ = 10005,
    
    
    FeatureSet_tvOS_GPUFamily1_v1/* API_AVAILABLE(tvos(9.0)) API_UNAVAILABLE(ios) API_UNAVAILABLE(macos)*/ = 30000,
    FeatureSet_TVOS_GPUFamily1_v1/* API_AVAILABLE(tvos(9.0)) API_UNAVAILABLE(ios) API_UNAVAILABLE(macos)*/ = FeatureSet_tvOS_GPUFamily1_v1, // deprecated
    
    FeatureSet_tvOS_GPUFamily1_v2/* API_AVAILABLE(tvos(10.0)) API_UNAVAILABLE(ios) API_UNAVAILABLE(macos)*/ = 30001,
    
    FeatureSet_tvOS_GPUFamily1_v3/* API_AVAILABLE(tvos(11.0)) API_UNAVAILABLE(ios) API_UNAVAILABLE(macos)*/ = 30002,
    
    FeatureSet_tvOS_GPUFamily1_v4/* API_AVAILABLE(tvos(12.0)) API_UNAVAILABLE(ios) API_UNAVAILABLE(macos)*/ = 30004,
#elif WINDOWS
    
#endif    
};// API_AVAILABLE(macos(10.11), ios(8.0)) API_AVAILABLE(tvos(9.0));

enum PipelineOption
{
    PipelineOptionNone               = 0,
    PipelineOptionArgumentInfo       = 1 << 0,
    PipelineOptionBufferTypeInfo     = 1 << 1,
};// API_AVAILABLE(macos(10.11), ios(8.0));

enum ReadWriteTextureTier
{
    ReadWriteTextureTierNone = 0,
    ReadWriteTextureTier1 = 1,
    ReadWriteTextureTier2 = 2,
};// API_AVAILABLE(macos(10.13), ios(11.0));


//For both tiers, the maximum number of argument buffer entries in each function argument table is 8
#define MAXIMUM_ARG_BUFFER_ENTRY 8
//For both tiers, the maximum number of unique samplers per app are 96 for iOS and tvOS, and at least 1024 for macOS
//these limits are only applicable to samplers that have their supportArgumentBuffers property set to YES.

#if IOS
#define MAXIMUM_UNIQUE_SAMPLERS 96
#elif OSX
#define MAXIMUM_UNIQUE_SAMPLERS 1024
#endif

enum ArgumentBuffersTier
{
    ArgumentBuffersTier1 = 0,
    ArgumentBuffersTier2 = 1,
};// API_AVAILABLE(macos(10.13), ios(11.0));

#if 0
struct RenderPipelineReflection
{
    Argument vertex_arguments;
    //An array of objects that describe the arguments of a vertex function.
    
    //Obtaining the Arguments of the Fragment Function
    Argument fragment_argument;
    //An array of objects that describe the arguments of a fragment function.
    
    //Using Tile Functions
    Argument tile_arguments;
    //An array of objects that describe the arguments of a tile shading function.
    
    //Constants
    //MTLAutoreleasedRenderPipelineReflection
    //A conve
};
#endif

