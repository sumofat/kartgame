#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if !defined(RESOURCE_H)

// NOTE(Ray Garner): We give the ability to pass in channels
//because we dont do swizzling the channels here yet. 
//For now we assume the data on disk has been preconditioned 
//the way we need it but this may not always be the case.

void get_image_from_disc(char* file,LoadedTexture* loaded_texture,int desired_channels)
{
    stbi_set_flip_vertically_on_load(true);
    int dimx;
    int dimy;
    //NOTE(Ray):Depends on your pipeline wether or not you will need this or not.
#if IOS
    //    stbi_convert_iphone_png_to_rgb(0);
    //    stbi_set_unpremultiply_on_load(1);
#endif
    int comp;
    stbi_info(file,&dimx, &dimy, &comp);
        
    loaded_texture->texels = stbi_load(file, (&dimx), (&dimy), (int*)&loaded_texture->channel_count, desired_channels);
    loaded_texture->dim = f2_create(dimx, dimy);
    loaded_texture->width_over_height = dimx/dimy;
    // NOTE(Ray Garner):stbi_info always returns 8bits/1byte per channels if we want to load 16bit or float need to use a 
    //a different api. for now we go with this. 
    //Will probaby need a different path for HDR textures etc..
    loaded_texture->bytes_per_pixel = desired_channels;
    loaded_texture->align_percentage = f2_create(0.5f,0.5f);
    loaded_texture->channel_count = desired_channels;
    loaded_texture->texture = {};
    loaded_texture->size = dimx * dimy * loaded_texture->bytes_per_pixel;
}
    
LoadedTexture get_loaded_image(char* file,int desired_channels)
{
    LoadedTexture tex;
    get_image_from_disc(file,&tex,desired_channels);
    ASSERT(tex.texels);
    return tex;
}

#define RESOURCE_H
#endif
