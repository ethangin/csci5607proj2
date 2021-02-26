//CSCI 5607 HW 2 - Image Conversion Instructor: S. J. Guy <sjguy@umn.edu>
//In this assignment you will load and convert between various image formats.
//Additionally, you will manipulate the stored image data by quantizing, cropping, and supressing channels

#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <float.h>

#include <fstream>
using namespace std;

/**
 * Image
 **/
Image::Image (int width_, int height_){

    assert(width_ > 0);
    assert(height_ > 0);

    width           = width_;
    height          = height_;
    num_pixels      = width * height;
    sampling_method = IMAGE_SAMPLING_POINT;
    
    data.raw = new uint8_t[num_pixels*4];
		int b = 0; //which byte to write to
		for (int j = 0; j < height; j++){
			for (int i = 0; i < width; i++){
				data.raw[b++] = 0;
				data.raw[b++] = 0;
				data.raw[b++] = 0;
				data.raw[b++] = 0;
			}
		}

    assert(data.raw != NULL);
}

Image::Image (const Image& src){
	width           = src.width;
	height          = src.height;
	num_pixels      = width * height;
	sampling_method = IMAGE_SAMPLING_POINT;
	
	data.raw = new uint8_t[num_pixels*4];
	
	//memcpy(data.raw, src.data.raw, num_pixels);
	*data.raw = *src.data.raw;
}

Image::Image (char* fname){

	int lastc = strlen(fname);
   int numComponents; //(e.g., Y, YA, RGB, or RGBA)
   data.raw = stbi_load(fname, &width, &height, &numComponents, 4);
	
	if (data.raw == NULL){
		printf("Error loading image: %s", fname);
		exit(-1);
	}
	
	num_pixels = width * height;
	sampling_method = IMAGE_SAMPLING_POINT;
	
}

Image::~Image (){
    delete data.raw;
    data.raw = NULL;
}

void Image::Write(char* fname){
	
	int lastc = strlen(fname);

	switch (fname[lastc-1]){
	   case 'g': //jpeg (or jpg) or png
	     if (fname[lastc-2] == 'p' || fname[lastc-2] == 'e') //jpeg or jpg
	        stbi_write_jpg(fname, width, height, 4, data.raw, 95);  //95% jpeg quality
	     else //png
	        stbi_write_png(fname, width, height, 4, data.raw, width*4);
	     break;
	   case 'a': //tga (targa)
	     stbi_write_tga(fname, width, height, 4, data.raw);
	     break;
	   case 'p': //bmp
	   default:
	     stbi_write_bmp(fname, width, height, 4, data.raw);
	}
}


void Image::Brighten (double factor){
	int x,y;
	for (x = 0 ; x < Width() ; x++){
		for (y = 0 ; y < Height() ; y++){
			Pixel p = GetPixel(x, y);
			Pixel scaled_p = p*factor;
			GetPixel(x,y) = scaled_p;
		}
	}
}

void Image::ExtractChannel(int channel){
	if (channel == 0) {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				Pixel p = GetPixel(i,j);
				SetPixel(i,j, Pixel(p.r, 0, 0, p.a));
			}
		}
   	} else if (channel == 1) {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				Pixel p = GetPixel(i,j);
				SetPixel(i,j, Pixel(0, p.g, 0, p.a));
			}
		}
	} else if (channel == 2) {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				Pixel p = GetPixel(i,j);
				SetPixel(i,j, Pixel(0, 0, p.b, p.a));
			}
		}
	} else {
		printf("Error: channel value %d is not a valid input", channel);
	}
}


void Image::Quantize (int nbits){
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			Pixel p = GetPixel(i,j);
			SetPixel(i,j, PixelQuant(p, nbits));
		}
	}
}

Image* Image::Crop(int x, int y, int w, int h){
	Image *img = new Image(w,h);

	for (int i = 0; i < w-1; i++) {
		for (int j = 0; j < h-1; j++) {
			(*img).SetPixel(i,j, GetPixel(i+x,j+y));
		}
	}

   	return img;
}


void Image::AddNoise (double factor){
	int random;
	float r,g,b;
	Pixel p;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			random = factor * (rand() % 81 - 40);
			p = GetPixel(i,j);
			r = p.r + random;
			g = p.g + random;
			b = p.b + random;
			p.SetClamp(r,g,b);
			SetPixel(i,j,p);
		}
	}
}

void Image::ChangeContrast (double factor){
	float tot_lum = 0;
	int num_pixels = width * height;
	Pixel p, avg;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			tot_lum += GetPixel(i,j).Luminance();
		}
	}

	float avg_lum = tot_lum / num_pixels;
	avg = Pixel(avg_lum * 255, avg_lum * 255, avg_lum * 255);
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			p = PixelLerp(avg, GetPixel(i,j), factor);
			SetPixel(i,j,p);
		}
	}
}


void Image::ChangeSaturation(double factor){
	float lum;
	Pixel p, gray, cur_pixel;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			cur_pixel = GetPixel(i,j);
			lum = cur_pixel.Luminance();
			gray = Pixel(lum * 255, lum * 255, lum * 255);
			p = PixelLerp(gray, cur_pixel, factor);
			SetPixel(i,j,p);
		}
	}
}


//For full credit, check that your dithers aren't making the pictures systematically brighter or darker
void Image::RandomDither (int nbits){
	Pixel p, rand_pix;
	float r,g,b;
	int random;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			p = GetPixel(i,j);
			p = PixelQuant(p, nbits);
			random = rand() % 129 - 64;
			r = p.r + random;
			g = p.g + random;
			b = p.b + random;
			rand_pix.SetClamp(r,g,b);
			SetPixel(i,j,rand_pix);
		}
	}
}

//This bayer method gives the quantization thresholds for an ordered dither.
//This is a 4x4 dither pattern, assumes the values are quantized to 16 levels.
//You can either expand this to a larger bayer pattern. Or (more likely), scale
//the threshold based on the target quantization levels.
static int Bayer4[4][4] ={
    {15,  7, 13,  5},
    { 3, 11,  1,  9},
    {12,  4, 14,  6},
    { 0,  8,  2, 10}
};


void Image::OrderedDither(int nbits){
	/* WORK HERE */
}

/* Error-diffusion parameters */
const double
    ALPHA = 7.0 / 16.0,
    BETA  = 3.0 / 16.0,
    GAMMA = 5.0 / 16.0,
    DELTA = 1.0 / 16.0;

void Image::FloydSteinbergDither(int nbits){
	Pixel p, next_pix;
	float r,g,b;
	int random;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			p = GetPixel(x,y);
			p = PixelQuant(p, nbits);
			SetPixel(x,y,p);
			random = rand() % 129 - 64;
			
			if (ValidCoord(x+1,y) == 1) {
				next_pix = GetPixel(x+1,y);
				r = next_pix.r + random*ALPHA;
				g = next_pix.g + random*ALPHA;
				b = next_pix.b + random*ALPHA;
				next_pix.SetClamp(r,g,b);
				SetPixel(x+1,y, next_pix);
			}

			if (ValidCoord(x-1,y+1) == 1) {
				next_pix = GetPixel(x-1,y+1);
				r = next_pix.r + random*BETA;
				g = next_pix.g + random*BETA;
				b = next_pix.b + random*BETA;
				next_pix.SetClamp(r,g,b);
				SetPixel(x-1,y+1, next_pix);
			}

			if (ValidCoord(x,y+1) == 1) {
				next_pix = GetPixel(x,y+1);
				r = next_pix.r + random*GAMMA;
				g = next_pix.g + random*GAMMA;
				b = next_pix.b + random*GAMMA;
				next_pix.SetClamp(r,g,b);
				SetPixel(x,y+1, next_pix);
			}

			if (ValidCoord(x+1,y+1) == 1) {
				next_pix = GetPixel(x+1,y+1);
				r = next_pix.r + random*DELTA;
				g = next_pix.g + random*DELTA;
				b = next_pix.b + random*DELTA;
				next_pix.SetClamp(r,g,b);
				SetPixel(x+1,y+1, next_pix);
			}
		}
	}
}

void Image::Blur(int n){
   // float r, g, b; //I got better results converting everything to floats, then converting back to bytes
	// Image* img_copy = new Image(*this); //This is will copying the image, so you can read the orginal values for filtering (
                                          //  ... don't forget to delete the copy!
	float r,g,b;
	Image* img_copy = new Image(*this);
	Pixel p;

	if (n == 3) {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				r=g=b=0.0;
				if (ValidCoord(x-1,y) == 1) {
					p = GetPixel(x-1,y);
					r += 0.25 * p.r;
					g += 0.25 * p.g;
					b += 0.25 * p.b;
				}
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					r += 0.5 * p.r;
					g += 0.5 * p.g;
					b += 0.5 * p.b;
				}
				if (ValidCoord(x+1,y) == 1) {
					p = GetPixel(x+1,y);
					r += 0.25 * p.r;
					g += 0.25 * p.g;
					b += 0.25 * p.b;
				}

				p.SetClamp(r,g,b);
				(*img_copy).SetPixel(x,y,p);
			}
		}

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				r=g=b=0;
				if (ValidCoord(x,y-1) == 1) {
					p = GetPixel(x,y-1);
					r += 0.25 * p.r;
					g += 0.25 * p.g;
					b += 0.25 * p.b;
				}
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					r += 0.5 * p.r;
					g += 0.5 * p.g;
					b += 0.5 * p.b;
				}
				if (ValidCoord(x,y+1) == 1) {
					p = GetPixel(x,y+1);
					r += 0.25 * p.r;
					g += 0.25 * p.g;
					b += 0.25 * p.b;
				}

				p.SetClamp(r,g,b);
				(*img_copy).SetPixel(x,y,p);
			}
		}
		
	} else if (n == 5) {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				r=g=b=0.0;
				if (ValidCoord(x-2,y) == 1) {
					p = GetPixel(x-2,y);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}
				if (ValidCoord(x-1,y) == 1) {
					p = GetPixel(x-1,y);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					r += (6.0/16.0) * p.r;
					g += (6.0/16.0) * p.g;
					b += (6.0/16.0) * p.b;
				}
				if (ValidCoord(x+1,y) == 1) {
					p = GetPixel(x+1,y);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x+2,y) == 1) {
					p = GetPixel(x+2,y);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}

				p.SetClamp(r,g,b);
				(*img_copy).SetPixel(x,y,p);
			}
		}

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				r=g=b=0;
				if (ValidCoord(x,y-2) == 1) {
					p = GetPixel(x,y-2);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}
				if (ValidCoord(x,y-1) == 1) {
					p = GetPixel(x,y-1);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					r += (6.0/16.0) * p.r;
					g += (6.0/16.0) * p.g;
					b += (6.0/16.0) * p.b;
				}
				if (ValidCoord(x,y+1) == 1) {
					p = GetPixel(x,y+1);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x,y+2) == 1) {
					p = GetPixel(x,y+2);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}

				p.SetClamp(r,g,b);
				(*img_copy).SetPixel(x,y,p);
			}
		}
	} else if (n == 7) {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				r=g=b=0.0;
				if (ValidCoord(x,y-3) == 1) {
					p = GetPixel(x,y-3);
					r += (1.0/64.0) * p.r;
					g += (1.0/64.0) * p.g;
					b += (1.0/64.0) * p.b;
				}
				if (ValidCoord(x,y-2) == 1) {
					p = GetPixel(x,y-2);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}
				if (ValidCoord(x,y-1) == 1) {
					p = GetPixel(x,y-1);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					r += (22.0/64.0) * p.r;
					g += (22.0/64.0) * p.g;
					b += (22.0/64.0) * p.b;
				}
				if (ValidCoord(x,y+1) == 1) {
					p = GetPixel(x,y+1);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x,y+2) == 1) {
					p = GetPixel(x,y+2);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}
				if (ValidCoord(x,y+3) == 1) {
					p = GetPixel(x,y+3);
					r += (1.0/64.0) * p.r;
					g += (1.0/64.0) * p.g;
					b += (1.0/64.0) * p.b;
				}

				p.SetClamp(r,g,b);
				(*img_copy).SetPixel(x,y,p);
			}
		}

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				r=g=b=0;
				if (ValidCoord(x,y-3) == 1) {
					p = GetPixel(x,y-3);
					r += (1.0/64.0) * p.r;
					g += (1.0/64.0) * p.g;
					b += (1.0/64.0) * p.b;
				}
				if (ValidCoord(x,y-2) == 1) {
					p = GetPixel(x,y-2);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}
				if (ValidCoord(x,y-1) == 1) {
					p = GetPixel(x,y-1);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					r += (22.0/64.0) * p.r;
					g += (22.0/64.0) * p.g;
					b += (22.0/64.0) * p.b;
				}
				if (ValidCoord(x,y+1) == 1) {
					p = GetPixel(x,y+1);
					r += (1.0/4.0) * p.r;
					g += (1.0/4.0) * p.g;
					b += (1.0/4.0) * p.b;
				}
				if (ValidCoord(x,y+2) == 1) {
					p = GetPixel(x,y+2);
					r += (1.0/16.0) * p.r;
					g += (1.0/16.0) * p.g;
					b += (1.0/16.0) * p.b;
				}
				if (ValidCoord(x,y+3) == 1) {
					p = GetPixel(x,y+3);
					r += (1.0/64.0) * p.r;
					g += (1.0/64.0) * p.g;
					b += (1.0/64.0) * p.b;
				}

				p.SetClamp(r,g,b);
				(*img_copy).SetPixel(x,y,p);
			}
		}
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			SetPixel(x,y,(*img_copy).GetPixel(x,y));
		}
	}

	delete img_copy;
}

void Image::Sharpen(int n){
	Image* orig_img = new Image(*this);
	Pixel p;
	Blur(n);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			p = PixelLerp((*orig_img).GetPixel(x,y), GetPixel(x,y),  1.25);

			SetPixel(x,y,p);
		}
	}

	delete orig_img;
}

void Image::EdgeDetect(){
	float r,g,b;
	Image* img_copy_x = new Image(*this);
	Image* img_copy_y = new Image(*this);
	Pixel p,q;

	// X gradient
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			r=g=b=0.0;
			if (ValidCoord(x-1,y-1) == 1) {
				p = GetPixel(x-1,y-1);
				r += p.r;
				g += p.g;
				b += p.b;
			}
			if (ValidCoord(x-1,y) == 1) {
				p = GetPixel(x-1,y);
				r += 2.0 * p.r;
				g += 2.0 * p.g;
				b += 2.0 * p.b;
			}
			if (ValidCoord(x-1,y+1) == 1) {
				p = GetPixel(x-1,y+1);
				r += p.r;
				g += p.g;
				b += p.b;
			}
			if (ValidCoord(x+1,y-1) == 1) {
				p = GetPixel(x+1,y-1);
				r += -1.0 * p.r;
				g += -1.0 * p.g;
				b += -1.0 * p.b;
			}
			if (ValidCoord(x+1,y) == 1) {
				p = GetPixel(x+1,y);
				r += -2.0 * p.r;
				g += -2.0 * p.g;
				b += -2.0 * p.b;
			}
			if (ValidCoord(x+1,y+1) == 1) {
				p = GetPixel(x+1,y+1);
				r += -1.0 * p.r;
				g += -1.0 * p.g;
				b += -1.0 * p.b;
			}
			

			p.SetClamp(r,g,b);
			(*img_copy_x).SetPixel(x,y,p);
		}
	}

	// Y gradient
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			r=g=b=0;
				if (ValidCoord(x-1,y-1) == 1) {
				p = GetPixel(x-1,y-1);
				r += p.r;
				g += p.g;
				b += p.b;
			}
			if (ValidCoord(x,y-1) == 1) {
				p = GetPixel(x,y-1);
				r += 2.0 * p.r;
				g += 2.0 * p.g;
				b += 2.0 * p.b;
			}
			if (ValidCoord(x+1,y-1) == 1) {
				p = GetPixel(x+1,y-1);
				r += p.r;
				g += p.g;
				b += p.b;
			}
			if (ValidCoord(x-1,y+1) == 1) {
				p = GetPixel(x-1,y+1);
				r += -1.0 * p.r;
				g += -1.0 * p.g;
				b += -1.0 * p.b;
			}
			if (ValidCoord(x,y+1) == 1) {
				p = GetPixel(x,y+1);
				r += -2.0 * p.r;
				g += -2.0 * p.g;
				b += -2.0 * p.b;
			}
			if (ValidCoord(x+1,y+1) == 1) {
				p = GetPixel(x+1,y+1);
				r += -1.0 * p.r;
				g += -1.0 * p.g;
				b += -1.0 * p.b;
			}

			p.SetClamp(r,g,b);
			(*img_copy_y).SetPixel(x,y,p);
		}
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			r=g=b=0;
			p = (*img_copy_x).GetPixel(x,y);
			q = (*img_copy_y).GetPixel(x,y);

			r = sqrt( pow(p.r, 2.0) + pow(q.r, 2.0) );
			g = sqrt( pow(p.g, 2.0) + pow(q.g, 2.0) );
			b = sqrt( pow(p.b, 2.0) + pow(q.b, 2.0) );
			p.SetClamp(r,g,b);
			SetPixel(x,y,p);
		}
	}

	delete img_copy_x;
	delete img_copy_y;
}

Image* Image::Scale(double sx, double sy){
	Pixel p;
	Image* img_copy = new Image( sx * width, sy * height);
	int h = (*img_copy).height;
	int w = (*img_copy).width;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (ValidCoord(ceil(x/sx), ceil(y/sy)) == 1) {
				//printf("sampling original at (%f, %f)\n", x/sx, y/sy);
				p = Sample(x/sx, y/sy);
				(*img_copy).SetPixel(x,y,p);
				//printf("width: %d, height: %d finished sampling for (%f, %f)\n", width, height, x/sx,y/sy);
			} else {
				//printf("coordinate (%f, %f) out of range ignored\n", x/sx, y/sy);
			}

		}
	}

	printf("scaled from %d x %d to %d x %d", width, height, w,h);

	return img_copy;
}

Image* Image::Rotate(double angle){
	angle *= 3.14159 / 180.0;
	Pixel p;
	Image* img_copy = new Image(width * cos(angle) + height * sin(angle), width * sin(angle) + height * cos(angle));
	int h = (*img_copy).height;
	int w = (*img_copy).width;
	double u,v;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			u = x * cos(angle) - y * sin(angle) + height * sin(angle);
			v = x * sin(angle) + y * cos(angle) - width * sin(angle);
			if (ValidCoord(ceil(u), ceil(v)) == 1) {
				//printf("sampling original at (%f, %f)\n", x/sx, y/sy);
				p = Sample(u, v);
				(*img_copy).SetPixel(x,y,p);
				//printf("width: %d, height: %d finished sampling for (%f, %f)\n", width, height, x/sx,y/sy);
			} else {
				//printf("coordinate (%f, %f) out of range ignored\n", x/sx, y/sy);
			}

		}
	}

	printf("scaled from %d x %d to %d x %d", width, height, w,h);

	return img_copy;
}

void Image::Fun(){
	/* WORK HERE */
}

/**
 * Image Sample
 **/
void Image::SetSamplingMethod(int method){
   assert((method >= 0) && (method < IMAGE_N_SAMPLING_METHODS));
   sampling_method = method;
}

Pixel Image::Sample (double u, double v){
   	float r,g,b;
	Pixel p, q;
	int x,y, pixel_count;
	double dist, total_val, factor;
	if (sampling_method == 0) { // Point 
		pixel_count = 0;
		for (y = ceil(v-1); y < floor(v+1); y++) {
			for (x = ceil(u-1); x < floor(u+1); x++) {
				if (ValidCoord(x,y) == 1) {
					pixel_count++;
					p = GetPixel(x,y);
					r += p.r;
					g += p.g;
					b += p.b;
				}
			}
		}

		r = r / pixel_count;
		g = g / pixel_count;
		b = b / pixel_count;
		//printf("pixel (%f, %f, %f) sampled from %d points\n", r,g,b,pixel_count);
		p.SetClamp(r,g,b);
	} else if (sampling_method == 1) { // Bilinear
		pixel_count = 0;
		r=g=b=0;
		for (y = ceil(v-2); y < floor(v+2); y++) {
			for (x = ceil(u-2); x < floor(u+2); x++) {
				if (ValidCoord(x,y) == 1) {
					pixel_count++;
				}
			}
		}

		for (y = ceil(v-2); y < floor(v+2); y++) {
			for (x = ceil(u-2); x < floor(u+2); x++) {
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					dist = sqrt(pow(x-u, 2.0) + pow(y-v, 2.0));
					factor = 3.0-dist;
					r += factor * p.r;
					g += factor * p.g;
					b += factor * p.b;
					total_val += factor;
				}
			}
		}

		r /= total_val;
		g /= total_val;
		b /= total_val;
		//printf("pixel (%f, %f, %f) sampled from %d points\n", r,g,b,pixel_count);
		p.SetClamp(r,g,b);
	} else if (sampling_method == 2) { // Gaussian
		pixel_count = 0;
		r=g=b=0;
		for (y = ceil(v-2); y < floor(v+2); y++) {
			for (x = ceil(u-2); x < floor(u+2); x++) {
				if (ValidCoord(x,y) == 1) {
					pixel_count++;
				}
			}
		}

		for (y = ceil(v-2); y < floor(v+2); y++) {
			for (x = ceil(u-2); x < floor(u+2); x++) {
				if (ValidCoord(x,y) == 1) {
					p = GetPixel(x,y);
					dist = sqrt(pow(x-u, 2.0) + pow(y-v, 2.0));
					if (0 < dist && dist < 1) {
						factor = 6;
					} else if (1 < dist && dist < 2) {
						factor = 4;
					} else {
						factor = 1;
					}
					r += factor * p.r;
					g += factor * p.g;
					b += factor * p.b;
					total_val += factor;
				}
			}
		}

		r /= total_val;
		g /= total_val;
		b /= total_val;
		//printf("pixel (%f, %f, %f) sampled from %d points\n", r,g,b,pixel_count);
		p.SetClamp(r,g,b);
	} else {
		printf("Error: %d is not a valid sampling method", sampling_method);
	}
   return p;
}
