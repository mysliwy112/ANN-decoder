#include "PNGformat.h"

#include<png.h>

#include<stdexcept>
#include<string>
#include<vector>
#include<iostream>


using namespace std;

void write_PNG(std::vector<unsigned char> &data, unsigned long long width, unsigned long long height, int bpp, std::string &filename){

    png_bytep row_pointers[height];
    for(int i=0;i<height;i++){
        row_pointers[i]=data.data()+width*4*i;
    }

    FILE *file = fopen(filename.c_str(), "wb");
    if(!file)
        throw invalid_argument("Can't access file.");

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        throw invalid_argument("Can't create PNG struct.");

    png_infop info = png_create_info_struct(png);
    if (!info)
        throw invalid_argument("Can't create INFO struct.");

    if (setjmp(png_jmpbuf(png)))
        throw invalid_argument("PNG.png stopped working.");

    png_init_io(png, file);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
    png,
    info,
    width, height,
    bpp,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    if (!row_pointers)
        throw invalid_argument("Row pointer problem here.");

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

//    for(int y = 0; y < data.height; y++) {
//        free(row_pointers[y]);
//    }
//    free(row_pointers);

    fclose(file);

    png_destroy_write_struct(&png, &info);
}




am::Image read_PNG(string filename) {
    am::Image image;

    png_byte color_type;
    png_bytep *row_pointers = NULL;


    FILE *fp = fopen(filename.c_str(), "rb");
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();

    png_infop info = png_create_info_struct(png);
    if(!info) abort();

    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    png_read_info(png, info);

    image.width      = png_get_image_width(png, info);
    image.height     = png_get_image_height(png, info);
    color_type = png_get_color_type(png, info);
    image.bpp = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(image.bpp == 16)
    png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && image.bpp < 8)
    png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    if (row_pointers) abort();

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * image.height);
    for(int y = 0; y < image.height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }

    png_read_image(png, row_pointers);

    image.rgba32.resize(image.width*image.height*4,0);
    for(int y = 0; y < image.height; y++) {
        png_bytep row = row_pointers[y];
        for(int x = 0; x < image.width; x++) {
            png_bytep px = &(row[x * 4]);
            image.rgba32[(y*image.width+x)*4]=px[0];
            image.rgba32[(y*image.width+x)*4+1]=px[1];
            image.rgba32[(y*image.width+x)*4+2]=px[2];
            image.rgba32[(y*image.width+x)*4+3]=px[3];
        }
    }

    fclose(fp);
    free(row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    return image;
}
