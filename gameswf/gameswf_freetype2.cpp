//#include "base.h"

#if 0
#include "gameswf_render.h"
#include "gameswf_freetype2.h"
#include "gameswf_log.h"
#include "gameswf_canvas.h"
#include "gameswf_root.h"

#include "../base/tu_file.h"

#include <memory>

#include <ft2build.h>
#include <freetype/freetype.h> //FT_FREETYPE_H
#include <freetype/ftstroke.h> //FT_STROKER_H
#include <freetype/ftadvanc.h> //FT_ADVANCES_H
#include <freetype/ftlcdfil.h> //FT_LCD_FILTER_H

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
const struct {
    int          code;
    const char*  message;
} FT_Errors[] =
#include FT_ERRORS_H

float round (float v)
{
	return floor(v+0.5f);
}

extern gameswf::bitmap_info* create_bitmap_info(gameswf::bitmap_info* atlas, int width, int height, const gameswf::rect& uv);

extern bool  CreateDistanceField(int in_w, int in_h, const unsigned char* in_data, int scale_down, float spread, int *w, int *h, unsigned char**data);

void computegradient(double *img, int w, int h, double *gx, double *gy)
{
    int i,j,k;
    double glength;
#define SQRT2 1.4142136
    for(i = 1; i < h-1; i++) { // Avoid edges where the kernels would spill over
        for(j = 1; j < w-1; j++) {
            k = i*w + j;
            if((img[k]>0.0) && (img[k]<1.0)) { // Compute gradient for edge pixels only
                gx[k] = -img[k-w-1] - SQRT2*img[k-1] - img[k+w-1] + img[k-w+1] + SQRT2*img[k+1] + img[k+w+1];
                gy[k] = -img[k-w-1] - SQRT2*img[k-w] - img[k+w-1] + img[k-w+1] + SQRT2*img[k+w] + img[k+w+1];
                glength = gx[k]*gx[k] + gy[k]*gy[k];
                if(glength > 0.0) { // Avoid division by zero
                    glength = sqrt(glength);
                    gx[k]=gx[k]/glength;
                    gy[k]=gy[k]/glength;
                }
            }
        }
    }
    // TODO: Compute reasonable values for gx, gy also around the image edges.
    // (These are zero now, which reduces the accuracy for a 1-pixel wide region
    // around the image edge.) 2x2 kernels would be suitable for this.
}
double edgedf(double gx, double gy, double a)
{
    double df, glength, temp, a1;
    
    if ((gx == 0) || (gy == 0)) { // Either A) gu or gv are zero, or B) both
        df = 0.5-a;  // Linear approximation is A) correct or B) a fair guess
    } else {
        glength = sqrt(gx*gx + gy*gy);
        if(glength>0) {
            gx = gx/glength;
            gy = gy/glength;
        }
        /* Everything is symmetric wrt sign and transposition,
         * so move to first octant (gx>=0, gy>=0, gx>=gy) to
         * avoid handling all possible edge directions.
         */
        gx = fabs(gx);
        gy = fabs(gy);
        if(gx<gy) {
            temp = gx;
            gx = gy;
            gy = temp;
        }
        a1 = 0.5*gy/gx;
        if (a < a1) { // 0 <= a < a1
            df = 0.5*(gx + gy) - sqrt(2.0*gx*gy*a);
        } else if (a < (1.0-a1)) { // a1 <= a <= 1-a1
            df = (0.5-a)*gx;
        } else { // 1-a1 < a <= 1
            df = -0.5*(gx + gy) + sqrt(2.0*gx*gy*(1.0-a));
        }
    }
    return df;
}
double distaa3(double *img, double *gximg, double *gyimg, int w, int c, int xc, int yc, int xi, int yi)
{
    double di, df, dx, dy, gx, gy, a;
    int closest;
    
    closest = c-xc-yc*w; // Index to the edge pixel pointed to from c
    a = img[closest];    // Grayscale value at the edge pixel
    gx = gximg[closest]; // X gradient component at the edge pixel
    gy = gyimg[closest]; // Y gradient component at the edge pixel
    
    if(a > 1.0) a = 1.0;
    if(a < 0.0) a = 0.0; // Clip grayscale values outside the range [0,1]
    if(a == 0.0) return 1000000.0; // Not an object pixel, return "very far" ("don't know yet")
    
    dx = (double)xi;
    dy = (double)yi;
    di = sqrt(dx*dx + dy*dy); // Length of integer vector, like a traditional EDT
    if(di==0) { // Use local gradient only at edges
        // Estimate based on local gradient only
        df = edgedf(gx, gy, a);
    } else {
        // Estimate gradient based on direction to edge (accurate for large di)
        df = edgedf(dx, dy, a);
    }
    return di + df; // Same metric as edtaa2, except at edges (where di=0)
}

// Shorthand macro: add ubiquitous parameters dist, gx, gy, img and w and call distaa3()
#define DISTAA(c,xc,yc,xi,yi) (distaa3(img, gx, gy, w, c, xc, yc, xi, yi))
void edtaa3(double *img, double *gx, double *gy, int w, int h, short *distx, short *disty, double *dist)
{
    int x, y, i, c;
    int offset_u, offset_ur, offset_r, offset_rd,
    offset_d, offset_dl, offset_l, offset_lu;
    double olddist, newdist;
    int cdistx, cdisty, newdistx, newdisty;
    int changed;
    double epsilon = 1e-3;
    
    /* Initialize index offsets for the current image width */
    offset_u = -w;
    offset_ur = -w+1;
    offset_r = 1;
    offset_rd = w+1;
    offset_d = w;
    offset_dl = w-1;
    offset_l = -1;
    offset_lu = -w-1;
    
    /* Initialize the distance images */
    for(i=0; i<w*h; i++) {
        distx[i] = 0; // At first, all pixels point to
        disty[i] = 0; // themselves as the closest known.
        if(img[i] <= 0.0)
        {
            dist[i]= 1000000.0; // Big value, means "not set yet"
        }
        else if (img[i]<1.0) {
            dist[i] = edgedf(gx[i], gy[i], img[i]); // Gradient-assisted estimate
        }
        else {
            dist[i]= 0.0; // Inside the object
        }
    }
    
    /* Perform the transformation */
    do
    {
        changed = 0;
        
        /* Scan rows, except first row */
        for(y=1; y<h; y++)
        {
            
            /* move index to leftmost pixel of current row */
            i = y*w;
            
            /* scan right, propagate distances from above & left */
            
            /* Leftmost pixel is special, has no left neighbors */
            olddist = dist[i];
            if(olddist > 0) // If non-zero distance or not set yet
            {
                c = i + offset_u; // Index of candidate for testing
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_ur;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
            i++;
            
            /* Middle pixels have all neighbors */
            for(x=1; x<w-1; x++, i++)
            {
                olddist = dist[i];
                if(olddist <= 0) continue; // No need to update further
                
                c = i+offset_l;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_lu;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_u;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_ur;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
            
            /* Rightmost pixel of row is special, has no right neighbors */
            olddist = dist[i];
            if(olddist > 0) // If not already zero distance
            {
                c = i+offset_l;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_lu;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_u;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
            
            /* Move index to second rightmost pixel of current row. */
            /* Rightmost pixel is skipped, it has no right neighbor. */
            i = y*w + w-2;
            
            /* scan left, propagate distance from right */
            for(x=w-2; x>=0; x--, i--)
            {
                olddist = dist[i];
                if(olddist <= 0) continue; // Already zero distance
                
                c = i+offset_r;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
        }
        
        /* Scan rows in reverse order, except last row */
        for(y=h-2; y>=0; y--)
        {
            /* move index to rightmost pixel of current row */
            i = y*w + w-1;
            
            /* Scan left, propagate distances from below & right */
            
            /* Rightmost pixel is special, has no right neighbors */
            olddist = dist[i];
            if(olddist > 0) // If not already zero distance
            {
                c = i+offset_d;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_dl;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
            i--;
            
            /* Middle pixels have all neighbors */
            for(x=w-2; x>0; x--, i--)
            {
                olddist = dist[i];
                if(olddist <= 0) continue; // Already zero distance
                
                c = i+offset_r;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_rd;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_d;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_dl;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
            /* Leftmost pixel is special, has no left neighbors */
            olddist = dist[i];
            if(olddist > 0) // If not already zero distance
            {
                c = i+offset_r;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_rd;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }
                
                c = i+offset_d;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
            
            /* Move index to second leftmost pixel of current row. */
            /* Leftmost pixel is skipped, it has no left neighbor. */
            i = y*w + 1;
            for(x=1; x<w; x++, i++)
            {
                /* scan right, propagate distance from left */
                olddist = dist[i];
                if(olddist <= 0) continue; // Already zero distance
                
                c = i+offset_l;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist[i]=newdist;
                    changed = 1;
                }
            }
        }
    }
    while(changed); // Sweep until no more updates are made
    
    /* The transformation is completed. */
    
}

unsigned char * make_distance_map( unsigned char *img, unsigned int width, unsigned int height )
{
    short * xdist = (short *)  malloc( width * height * sizeof(short) );
    short * ydist = (short *)  malloc( width * height * sizeof(short) );
    double * gx   = (double *) calloc( width * height, sizeof(double) );
    double * gy      = (double *) calloc( width * height, sizeof(double) );
    double * data    = (double *) calloc( width * height, sizeof(double) );
    double * outside = (double *) calloc( width * height, sizeof(double) );
    double * inside  = (double *) calloc( width * height, sizeof(double) );
    int i;
    
    // Convert img into double (data)
    double img_min = 255, img_max = -255;
    for( i=0; i<width*height; ++i)
    {
        double v = img[i];
        data[i] = v;
        if (v > img_max) img_max = v;
        if (v < img_min) img_min = v;
    }
    // Rescale image levels between 0 and 1
    for( i=0; i<width*height; ++i)
    {
        data[i] = (img[i]-img_min)/img_max;
    }
    
    // Compute outside = edtaa3(bitmap); % Transform background (0's)
    computegradient( data, width, height, gx, gy);
    edtaa3(data, gx, gy, height, width, xdist, ydist, outside);
    for( i=0; i<width*height; ++i)
        if( outside[i] < 0 )
            outside[i] = 0.0;
    
    // Compute inside = edtaa3(1-bitmap); % Transform foreground (1's)
    memset(gx, 0, sizeof(double)*width*height );
    memset(gy, 0, sizeof(double)*width*height );
    for( i=0; i<width*height; ++i)
        data[i] = 1 - data[i];
    computegradient( data, width, height, gx, gy);
    edtaa3(data, gx, gy, height, width, xdist, ydist, inside);
    for( i=0; i<width*height; ++i)
        if( inside[i] < 0 )
            inside[i] = 0.0;
    
    // distmap = outside - inside; % Bipolar distance field
    unsigned char *out = (unsigned char *) malloc( width * height * sizeof(unsigned char) );
    for( i=0; i<width*height; ++i)
    {
        outside[i] -= inside[i];
        outside[i] = 128+outside[i]*16;
        if( outside[i] < 0 ) outside[i] = 0;
        if( outside[i] > 255 ) outside[i] = 255;
        out[i] = 255 - (unsigned char) outside[i];
        //out[i] = (unsigned char) outside[i];
    }
    
    free( xdist );
    free( ydist );
    free( gx );
    free( gy );
    free( data );
    free( outside );
    free( inside );
    return out;
}


vector_t   * vector_new( size_t item_size )
{
	assert( item_size );

    vector_t *self = (vector_t *) malloc( sizeof(vector_t) );
    if( !self )
    {
        fprintf( stderr, "line %d: No more memory for allocating data\n", __LINE__ );
        exit( EXIT_FAILURE );
    }
    self->item_size = item_size;
    self->size      = 0;
    self->capacity  = 1;
    self->items     = malloc( self->item_size * self->capacity );
    return self;
}

void vector_delete( vector_t *self )
{
	assert( self );

    free( self->items );
    free( self );
}

void vector_push_back( vector_t *self, const void *item )
{
	vector_insert( self, self->size, item );
}

const void * vector_back( const vector_t *self )
{
    assert( self );
    assert( self->size );
    
    return vector_get( self, self->size-1 );
}

void vector_set( vector_t *self, const size_t index, const void *item )
{
    assert( self );
    assert( self->size );
    assert( index  < self->size );

    memcpy( (char *)(self->items) + index * self->item_size, item, self->item_size );
}

const void * vector_get( const vector_t *self, size_t index )
{
	assert( self );
    assert( self->size );
    assert( index  < self->size );

    return (char*)(self->items) + index * self->item_size;
}
void vector_reserve( vector_t *self, const size_t size )
{
    assert( self );

    if( self->capacity < size)
    {
        self->items = realloc( self->items, size * self->item_size );
        self->capacity = size;
    }
}
size_t vector_size( const vector_t *self )
{
	assert( self );

    return self->size;
}

// ----------------------------------------------------- vector_erase_range ---
void vector_erase_range( vector_t *self, const size_t first, const size_t last )
{
    assert( self );
    assert( first < self->size );
    assert( last  < self->size+1 );
    assert( first < last );

    memmove( (char *)(self->items) + first * self->item_size,
             (char *)(self->items) + last  * self->item_size,
             (self->size - last)   * self->item_size);
    self->size -= (last-first);
}


// ----------------------------------------------------------- vector_erase ---
void vector_erase( vector_t *self, const size_t index )
{
    assert( self );
    assert( index < self->size );

    vector_erase_range( self, index, index+1 );
}


void vector_clear( vector_t *self )
{
	assert( self );

    self->size = 0;
}

void vector_insert( vector_t *self, const size_t index, const void *item )
{
	assert( self );
    assert( index <= self->size);

    if( self->capacity <= self->size )
    {
        vector_reserve(self, 2 * self->capacity );
    }
    if( index < self->size )
    {
        memmove( (char *)(self->items) + (index + 1) * self->item_size,
                 (char *)(self->items) + (index + 0) * self->item_size,
                 (self->size - index)  * self->item_size);
    }
    self->size++;
    vector_set( self, index, item );
}


// ------------------------------------------------------ texture_atlas_new ---
texture_atlas_t * texture_atlas_new( const size_t width, const size_t height, const size_t depth )
{
    assert( (depth == 1) || (depth == 3) || (depth == 4) );

    texture_atlas_t *self = (texture_atlas_t *) malloc( sizeof(texture_atlas_t) );
    if( self == NULL)
    {
        fprintf( stderr,  "line %d: No more memory for allocating data\n", __LINE__ );
        exit( EXIT_FAILURE );
    }
    self->nodes = vector_new( sizeof(ivec3) );
    self->used = 0;
    self->width = width;
    self->height = height;
    self->depth = depth;
    self->id = 0;

    // We want a one pixel border around the whole atlas to avoid any artefact when
    // sampling texture
    ivec3 node = {{1,1,(int)width-2}};

    vector_push_back( self->nodes, &node );
    self->data = (unsigned char *) calloc( width*height*depth, sizeof(unsigned char) );

    if( self->data == NULL)
    {
        fprintf( stderr, "line %d: No more memory for allocating data\n", __LINE__ );
        exit( EXIT_FAILURE );
    }

    return self;
}

// --------------------------------------------------- texture_atlas_delete ---
void texture_atlas_delete( texture_atlas_t *self )
{
    assert( self );
    vector_delete( self->nodes );
    if( self->data )
    {
        free( self->data );
    }
    if( !self->id )
    {
    //    glDeleteTextures( 1, &self->id );
    }
    free( self );
}

// --------------------------------------------------- texture_atlas_upload ---
void texture_atlas_upload( texture_atlas_t * self )
{
    assert( self );
    assert( self->data );
/*
    if( !self->id )
    {
        glGenTextures( 1, &self->id );
    }

    glBindTexture( GL_TEXTURE_2D, self->id );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    if( self->depth == 4 )
    {
#ifdef GL_UNSIGNED_INT_8_8_8_8_REV
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, self->width, self->height,
                      0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, self->data );
#else
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, self->width, self->height,
                      0, GL_RGBA, GL_UNSIGNED_BYTE, self->data );
#endif
    }
    else if( self->depth == 3 )
    {
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, self->width, self->height,
                      0, GL_RGB, GL_UNSIGNED_BYTE, self->data );
    }
    else
    {
        glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, self->width, self->height,
                      0, GL_ALPHA, GL_UNSIGNED_BYTE, self->data );
    }/**/
}
// ----------------------------------------------- texture_atlas_set_region ---
void texture_atlas_set_region(	 texture_atlas_t * self,
                         		 const size_t x, const size_t y,
                         		 const size_t width, const size_t height,
                         		 const unsigned char * data, const size_t stride )
{
    assert( self );
    assert( x > 0);
    assert( y > 0);
    assert( x < (self->width-1));
    assert( (x + width) <= (self->width-1));
    assert( y < (self->height-1));
    assert( (y + height) <= (self->height-1));

    size_t i;
    size_t depth = self->depth;
    size_t charsize = sizeof(char);
    for( i=0; i<height; ++i )
    {
        memcpy( self->data+((y+i)*self->width + x ) * charsize * depth, 
                data + (i*stride) * charsize, width * charsize * depth  );
    }
}


// ------------------------------------------------------ texture_atlas_fit ---
int texture_atlas_fit( texture_atlas_t * self, const size_t index, const size_t width, const size_t height )
{
    assert( self );

    ivec3 *node = (ivec3 *) (vector_get( self->nodes, index ));
    int x = node->x, y, width_left = width;
	size_t i = index;

	if ( (x + width) > (self->width-1) )
    {
		return -1;
    }
	y = node->y;
	while( width_left > 0 )
	{
        node = (ivec3 *) (vector_get( self->nodes, i ));
        if( node->y > y )
        {
            y = node->y;
        }
		if( (y + height) > (self->height-1) )
        {
			return -1;
        }
		width_left -= node->z;
		++i;
	}
	return y;
}


// ---------------------------------------------------- texture_atlas_merge ---
void texture_atlas_merge( texture_atlas_t * self )
{
    assert( self );

    ivec3 *node, *next;
    size_t i;

	for( i=0; i< self->nodes->size-1; ++i )
    {
        node = (ivec3 *) (vector_get( self->nodes, i ));
        next = (ivec3 *) (vector_get( self->nodes, i+1 ));
		if( node->y == next->y )
		{
			node->z += next->z;
            vector_erase( self->nodes, i+1 );
			--i;
		}
    }
}


// ----------------------------------------------- texture_atlas_get_region ---
ivec4 texture_atlas_get_region( texture_atlas_t * self, const size_t width, const size_t height )
{
    assert( self );

	int best_index, y, best_height, best_width;
    ivec3 *node, *prev;
    ivec4 region = {{0,0,(int)width,(int)height}};
    size_t i;

    best_height = INT_MAX;
    best_width  = INT_MAX;
    best_index  = -1;
	for( i=0; i<self->nodes->size; ++i )
	{
        y = texture_atlas_fit( self, i, width, height );
		if( y >= 0 )
		{
            node = (ivec3 *) vector_get( self->nodes, i );
			if( ( (y + height) < best_height ) ||
                ( ((y + height) == best_height) && 
				(node->z < best_width)) )
			{
				best_height = y + height;
				best_index = i;
				best_width = node->z;
				region.x = node->x;
				region.y = y;
			}
        }
    }
   
	if( best_index == -1 )
    {
        region.x = -1;
        region.y = -1;
        region.w = 0;
        region.h = 0;
        return region;
    }

    node = (ivec3 *) malloc( sizeof(ivec3) );
    if( node == NULL)
    {
        fprintf( stderr, "line %d: No more memory for allocating data\n", __LINE__ );
        exit( EXIT_FAILURE );
    }
    node->x = region.x;
    node->y = region.y + height;
    node->z = width;
    vector_insert( self->nodes, best_index, node );
    free( node );

    for(i = best_index+1; i < self->nodes->size; ++i)
    {
        node = (ivec3 *) vector_get( self->nodes, i );
        prev = (ivec3 *) vector_get( self->nodes, i-1 );

        if (node->x < (prev->x + prev->z) )
        {
            int shrink = prev->x + prev->z - node->x;
            node->x += shrink;
            node->z -= shrink;
            if (node->z <= 0)
            {
                vector_erase( self->nodes, i );
                --i;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    texture_atlas_merge( self );
    self->used += width * height;
    return region;
}


// ---------------------------------------------------- texture_atlas_clear ---
void  texture_atlas_clear( texture_atlas_t * self )
{
    assert( self );
    assert( self->data );

    vector_clear( self->nodes );
    self->used = 0;
    // We want a one pixel border around the whole atlas to avoid any artefact when
    // sampling texture
    ivec3 node = {{1,1,(int)self->width-2}};
    vector_push_back( self->nodes, &node );
    memset( self->data, 0, self->width*self->height*self->depth );
}

/**/





int texture_font_load_face( FT_Library * library, const void *data, unsigned long dataSize, const float size, FT_Face * face )
{
    assert( library );
//    assert( filename );
    assert( size );

    size_t hres = 64;
    FT_Error error;
    FT_Matrix matrix = { (int)((1.0/hres) * 0x10000L),
                         (int)((0.0)      * 0x10000L),
                         (int)((0.0)      * 0x10000L),
                         (int)((1.0)      * 0x10000L) };

    /* Initialize library */
    error = FT_Init_FreeType( library );
    if( error )
    {
        fprintf(stderr, "FT_Error (0x%02x) : %s\n", FT_Errors[error].code, FT_Errors[error].message);
        return 0;
    }

    /* Load face */
//    error = FT_New_Face( *library, filename, 0, face );
    error = FT_New_Memory_Face( *library, ( FT_Byte* )data, dataSize, 0, face );
    if( error )
    {
        fprintf( stderr, "FT_Error (line %d, code 0x%02x) : %s\n", __LINE__, FT_Errors[error].code, FT_Errors[error].message);
        FT_Done_FreeType( *library );
        return 0;
    }

    /* Select charmap */
    error = FT_Select_Charmap( *face, FT_ENCODING_UNICODE );
    if( error )
    {
        fprintf( stderr, "FT_Error (line %d, code 0x%02x) : %s\n", __LINE__, FT_Errors[error].code, FT_Errors[error].message );
        FT_Done_Face( *face );
        FT_Done_FreeType( *library );
        return 0;
    }

    /* Set char size */
    error = FT_Set_Char_Size( *face, (int)(size*64), 0, 72*hres, 72 );
    if( error )
    {
        fprintf( stderr, "FT_Error (line %d, code 0x%02x) : %s\n", __LINE__, FT_Errors[error].code, FT_Errors[error].message );
        FT_Done_Face( *face );
        FT_Done_FreeType( *library );
        return 0;
    }

    /* Set transform matrix */
    FT_Set_Transform( *face, &matrix, NULL );

    return 1;
}





/******************************************************************************
 ******************************************************************************/

// ------------------------------------------------------ texture_glyph_new ---
texture_glyph_t * texture_glyph_new( void )
{
    texture_glyph_t *self = (texture_glyph_t *) malloc( sizeof(texture_glyph_t) );
    if( self == NULL)
    {
        fprintf( stderr, "line %d: No more memory for allocating data\n", __LINE__ );
        exit( EXIT_FAILURE );
    }
    self->id        = 0;
    self->width     = 0;
    self->height    = 0;
    self->outline_type = 0;
    self->outline_thickness = 0.0;
    self->offset_x  = 0;
    self->offset_y  = 0;
    self->advance_x = 0.0;
    self->advance_y = 0.0;
    self->s0        = 0.0;
    self->t0        = 0.0;
    self->s1        = 0.0;
    self->t1        = 0.0;
    self->kerning   = vector_new( sizeof(kerning_t) );
    return self;
}

void texture_glyph_delete( texture_glyph_t *self )
{
    assert( self );
    vector_delete( self->kerning );
    free( self );
}



/******************************************************************************
 ******************************************************************************/
// ------------------------------------------------------- texture_font_new ---
texture_font_t * texture_font_new( texture_atlas_t * atlas, const char * filename, tu_file *file, const float size)
{
    assert( filename );
    assert( size );

    texture_font_t *self = (texture_font_t *) malloc( sizeof(texture_font_t) );
    if( self == NULL)
    {
        fprintf( stderr, "line %d: No more memory for allocating data\n", __LINE__ );
        exit( EXIT_FAILURE );
    }
    self->glyphs = vector_new( sizeof(texture_glyph_t *) );
    self->atlas = atlas;
    self->height = 0;
    self->ascender = 0;
    self->descender = 0;
    self->filename = strdup( filename );
    self->size = size;
    self->outline_type = 0;
    self->outline_thickness = 0.0;
    self->hinting = 1;
    self->filtering = 1;
    // FT_LCD_FILTER_LIGHT   is (0x00, 0x55, 0x56, 0x55, 0x00)
    // FT_LCD_FILTER_DEFAULT is (0x10, 0x40, 0x70, 0x40, 0x10)
    self->lcd_weights[0] = 0x10;
    self->lcd_weights[1] = 0x40;
    self->lcd_weights[2] = 0x70;
    self->lcd_weights[3] = 0x40;
    self->lcd_weights[4] = 0x10;

    /* Read file */
    self->fileSize = file->size();
    self->fileData = malloc( self->fileSize );
    file->read_bytes(self->fileData, self->fileSize);

    /* Get font metrics at high resolution */
    FT_Library library;
    FT_Face face;
    if( !texture_font_load_face( &library, self->fileData, self->fileSize, self->size*100, &face ) )
    {
        return self;
    }

    // 64 * 64 because of 26.6 encoding AND the transform matrix used
    // in texture_font_load_face (hres = 64)
    self->underline_position = face->underline_position / (float)(64.0f*64.0f) * self->size;
    self->underline_position = round( self->underline_position );
    if( self->underline_position > -2 )
    {
        self->underline_position = -2.0;
    }

    self->underline_thickness = face->underline_thickness / (float)(64.0f*64.0f) * self->size;
    self->underline_thickness = round( self->underline_thickness );
    if( self->underline_thickness < 1 )
    {
        self->underline_thickness = 1.0f;
    }

    FT_Size_Metrics metrics = face->size->metrics; 
    self->ascender = (metrics.ascender >> 6) / 100.0f;
    self->descender = (metrics.descender >> 6) / 100.0f;
    self->height = (metrics.height >> 6) / 100.0f;
    self->linegap = self->height - self->ascender + self->descender;
    FT_Done_Face( face );
    FT_Done_FreeType( library );

    /* -1 is a special glyph */
///!!!    texture_font_get_glyph( self, -1 );

    return self;
}


// ---------------------------------------------------- texture_font_delete ---
void texture_font_delete( texture_font_t *self )
{
    assert( self );

    if( self->filename )
    {
        free( self->filename );
    }

    if( self->fileData )
    {
        free( self->fileData );
    }

    size_t i;
    texture_glyph_t *glyph;
    for( i=0; i<vector_size( self->glyphs ); ++i)
    {
        glyph = *(texture_glyph_t **) vector_get( self->glyphs, i );
        texture_glyph_delete( glyph);
    }

    vector_delete( self->glyphs );
    free( self );
}


texture_glyph_t * texture_font_get_glyph( texture_font_t * self, wchar_t charcode )
{
    assert( self );

    size_t i;
    wchar_t buffer[2] = {0,0};
    texture_glyph_t *glyph;

    assert( self );
    assert( self->filename );
    assert( self->atlas );

    /* Check if charcode has been already loaded */
    for( i=0; i<self->glyphs->size; ++i )
    {
        glyph = *(texture_glyph_t **) vector_get( self->glyphs, i );
        // If charcode is -1, we don't care about outline type or thickness
        if( (glyph->charcode == charcode) &&
            ((charcode == (wchar_t)(-1) ) || 
             ((glyph->outline_type == self->outline_type) &&
              (glyph->outline_thickness == self->outline_thickness)) ))
        {
            return glyph;
        }
    }

    /* charcode -1 is special : it is used for line drawing (overline,
     * underline, strikethrough) and background.
     */
    if( charcode == (wchar_t)(-1) )
    {
        size_t width  = self->atlas->width;
        size_t height = self->atlas->height;
        ivec4 region = texture_atlas_get_region( self->atlas, 5, 5 );
        texture_glyph_t * glyph = texture_glyph_new( );
        static /*unsigned*/ char data[4*4*3] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                                            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                                            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                                            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
        if ( region.x < 0 )
        {
            fprintf( stderr, "Texture atlas is full (line %d)\n",  __LINE__ );
            return NULL;
        }
        texture_atlas_set_region( self->atlas, region.x, region.y, 4, 4, ( unsigned char* )data, 0 );
        glyph->charcode = (wchar_t)(-1);
        glyph->s0 = (region.x+2)/(float)width;
        glyph->t0 = (region.y+2)/(float)height;
        glyph->s1 = (region.x+3)/(float)width;
        glyph->t1 = (region.y+3)/(float)height;
        vector_push_back( self->glyphs, &glyph );
        return glyph; //*(texture_glyph_t **) vector_back( self->glyphs );
    }

    /* Glyph has not been already loaded */
    buffer[0] = charcode;
    if( texture_font_load_glyphs( self, buffer ) == 0 )
    {
        return *(texture_glyph_t **) vector_back( self->glyphs );
    }
    return NULL;
}

texture_glyph_t * texture_font_get_empty( texture_atlas_t * atlas )
{
    size_t width  = atlas->width;
    size_t height = atlas->height;
    ivec4 region = texture_atlas_get_region( atlas, 5, 5 );
    texture_glyph_t * glyph = texture_glyph_new( );
    static /*unsigned*/ char data[4*4*3] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    if ( region.x < 0 )
    {
        fprintf( stderr, "Texture atlas is full (line %d)\n",  __LINE__ );
        return NULL;
    }
    texture_atlas_set_region( atlas, region.x, region.y, 4, 4, ( unsigned char* )data, 0 );
    glyph->charcode = (wchar_t)(-1);
    glyph->s0 = (region.x+2)/(float)width;
    glyph->t0 = (region.y+2)/(float)height;
    glyph->s1 = (region.x+3)/(float)width;
    glyph->t1 = (region.y+3)/(float)height;
    glyph->width = 5;
    glyph->height = 5;

    return glyph;
}


// ----------------------------------------------- texture_font_load_glyphs ---
size_t texture_font_load_glyphs( texture_font_t * self, const wchar_t * charcodes )
{
    assert( self );
    assert( charcodes );

    size_t i, x, y, width, height, depth, w, h;
    FT_Library library;
    FT_Error error;
    FT_Face face;
    FT_Glyph ft_glyph = NULL;
    FT_GlyphSlot slot  = NULL;
    FT_Bitmap ft_bitmap;

    FT_UInt glyph_index;
    texture_glyph_t *glyph;
    ivec4 region;
    size_t missed = 0;
    width  = self->atlas->width;
    height = self->atlas->height;
    depth  = self->atlas->depth;

    if( !texture_font_load_face( &library, self->fileData, self->fileSize, self->size, &face ) )
    {
        return wcslen(charcodes);
    }

    /* Load each glyph */
    for( i=0; i < wcslen(charcodes); ++i )
    {
        glyph_index = FT_Get_Char_Index( face, charcodes[i] );
        // WARNING: We use texture-atlas depth to guess if user wants
        //          LCD subpixel rendering
        FT_Int32 flags = 0;

        if( self->outline_type > 0 )
        {
            flags |= FT_LOAD_NO_BITMAP;
        }
        else
        {
            flags |= FT_LOAD_RENDER;
        }

        if( !self->hinting )
        {
            flags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
        }
        else
        {
            flags |= FT_LOAD_FORCE_AUTOHINT;
        }


        if( depth == 3 )
        {
            FT_Library_SetLcdFilter( library, FT_LCD_FILTER_LIGHT );
            flags |= FT_LOAD_TARGET_LCD;
            if( self->filtering )
            {
                FT_Library_SetLcdFilterWeights( library, self->lcd_weights );
            }
        }
        error = FT_Load_Glyph( face, glyph_index, flags );

        if( error )
        {
            fprintf( stderr, "FT_Error (line %d, code 0x%02x) : %s\n", __LINE__, FT_Errors[error].code, FT_Errors[error].message );
            FT_Done_FreeType( library );
            return wcslen(charcodes)-i;
        }

        int ft_bitmap_width = 0;
        int ft_bitmap_rows = 0;
        int ft_bitmap_pitch = 0;
        int ft_glyph_top = 0;
        int ft_glyph_left = 0;
        if( self->outline_type == 0 )
        {
            slot            = face->glyph;
            ft_bitmap       = slot->bitmap;
            ft_bitmap_width = slot->bitmap.width;
            ft_bitmap_rows  = slot->bitmap.rows;
            ft_bitmap_pitch = slot->bitmap.pitch;
            ft_glyph_top    = slot->bitmap_top;
            ft_glyph_left   = slot->bitmap_left;
        }
        else
        {
            FT_Stroker stroker;
            error = FT_Stroker_New( library, &stroker );
            if( error )
            {
                fprintf(stderr, "FT_Error (0x%02x) : %s\n", FT_Errors[error].code, FT_Errors[error].message);
                return 0;
            }
            FT_Stroker_Set( stroker,
                            (int)(self->outline_thickness *64),
                            FT_STROKER_LINECAP_ROUND,
                            FT_STROKER_LINEJOIN_ROUND,
                            0);
            error = FT_Get_Glyph( face->glyph, &ft_glyph);
            if( error )
            {
                fprintf(stderr, "FT_Error (0x%02x) : %s\n", FT_Errors[error].code, FT_Errors[error].message);
                return 0;
            }

            if( self->outline_type == 1 )
            {
                error = FT_Glyph_Stroke( &ft_glyph, stroker, 1 );
            }
            else if ( self->outline_type == 2 )
            {
                error = FT_Glyph_StrokeBorder( &ft_glyph, stroker, 0, 1 );
            }
            else if ( self->outline_type == 3 )
            {
                error = FT_Glyph_StrokeBorder( &ft_glyph, stroker, 1, 1 );
            }
            if( error )
            {
                fprintf(stderr, "FT_Error (0x%02x) : %s\n", FT_Errors[error].code, FT_Errors[error].message);
                return 0;
            }
          
            if( depth == 1)
            {
                error = FT_Glyph_To_Bitmap( &ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1);
                if( error )
                {
                    fprintf(stderr, "FT_Error (0x%02x) : %s\n", FT_Errors[error].code, FT_Errors[error].message);
                    return 0;
                }
            }
            else
            {
#ifndef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
#error Requires freetype  with subpixel rendering option ( FT_CONFIG_OPTION_SUBPIXEL_RENDERING )
#endif
                error = FT_Glyph_To_Bitmap( &ft_glyph, FT_RENDER_MODE_LCD, 0, 1);
                if( error )
                {
                    fprintf(stderr, "FT_Error (0x%02x) : %s\n", FT_Errors[error].code, FT_Errors[error].message);
                    return 0;
                }
            }
            FT_BitmapGlyph ft_bitmap_glyph = (FT_BitmapGlyph) ft_glyph;
            ft_bitmap       = ft_bitmap_glyph->bitmap;
            ft_bitmap_width = ft_bitmap.width;
            ft_bitmap_rows  = ft_bitmap.rows;
            ft_bitmap_pitch = ft_bitmap.pitch;
            ft_glyph_top    = ft_bitmap_glyph->top;
            ft_glyph_left   = ft_bitmap_glyph->left;
            FT_Stroker_Done(stroker);
        }


        // We want each glyph to be separated by at least one black pixel
        // (for example for shader used in demo-subpixel.c)
        w = ft_bitmap_width/depth + 1;
        h = ft_bitmap_rows + 1;
        region = texture_atlas_get_region( self->atlas, w, h );
        if ( region.x < 0 )
        {
            missed++;
            fprintf( stderr, "Texture atlas is full (line %d)\n",  __LINE__ );
            continue;
        }
        w = w - 1;
        h = h - 1;
        x = region.x;
        y = region.y;
        
        texture_atlas_set_region( self->atlas, x, y, w, h,
                                  ft_bitmap.buffer, ft_bitmap.pitch );

        glyph = texture_glyph_new( );
        glyph->charcode = charcodes[i];
        glyph->width    = w;
        glyph->height   = h;
        glyph->outline_type = self->outline_type;
        glyph->outline_thickness = self->outline_thickness;
        glyph->offset_x = ft_glyph_left;
        glyph->offset_y = ft_glyph_top;
        glyph->s0       = x/(float)width;
        glyph->t0       = y/(float)height;
        glyph->s1       = (x + glyph->width)/(float)width;
        glyph->t1       = (y + glyph->height)/(float)height;

        // Discard hinting to get advance
        FT_Load_Glyph( face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
        slot = face->glyph;
        
        glyph->advance_x = slot->advance.x/64.0f;
        glyph->advance_y = slot->advance.y/64.0f;

        vector_push_back( self->glyphs, &glyph );

        if( self->outline_type > 0 )
        {
            FT_Done_Glyph( ft_glyph );
        }
    }
    FT_Done_Face( face );
    FT_Done_FreeType( library );
    texture_atlas_upload( self->atlas );
    texture_font_generate_kerning( self );
    return missed;
}

// ------------------------------------------ texture_font_generate_kerning ---
void texture_font_generate_kerning( texture_font_t *self )
{
    assert( self );

    size_t i, j;
    FT_Library library;
    FT_Face face;
    FT_UInt glyph_index, prev_index;
    texture_glyph_t *glyph, *prev_glyph;
    FT_Vector kerning;

    /* Load font */
    if( !texture_font_load_face( &library, self->fileData, self->fileSize, self->size, &face ) )
    {
        return;
    }

    /* For each glyph couple combination, check if kerning is necessary */
    /* Starts at index 1 since 0 is for the special backgroudn glyph */
    for( i=1; i<self->glyphs->size; ++i )
    {
        glyph = *(texture_glyph_t **) vector_get( self->glyphs, i );
        glyph_index = FT_Get_Char_Index( face, glyph->charcode );
        vector_clear( glyph->kerning );

        for( j=1; j<self->glyphs->size; ++j )
        {
            prev_glyph = *(texture_glyph_t **) vector_get( self->glyphs, j );
            prev_index = FT_Get_Char_Index( face, prev_glyph->charcode );
            FT_Get_Kerning( face, prev_index, glyph_index, FT_KERNING_UNFITTED, &kerning );
            // printf("%c(%d)-%c(%d): %ld\n",
            //       prev_glyph->charcode, prev_glyph->charcode,
            //       glyph_index, glyph_index, kerning.x);
            if( kerning.x )
            {
                // 64 * 64 because of 26.6 encoding AND the transform matrix used
                // in texture_font_load_face (hres = 64)
                kerning_t k = {prev_glyph->charcode, kerning.x / (float)(64.0f*64.0f)};
                vector_push_back( glyph->kerning, &k );
            }
        }
    }
    FT_Done_Face( face );
    FT_Done_FreeType( library );
}


namespace gameswf
{
	glyph_provider *  create_glyph_provider_freetype2()
	{
		FT_Library m_lib = NULL;
		int	error = FT_Init_FreeType(&m_lib);
		if (error)
		{
			fprintf(stderr, "FreeType provider: can't init FreeType!  error = %d\n", error);
			return NULL;
		}
		return new glyph_freetype2_provider();
	}

	glyph_freetype2_provider::glyph_freetype2_provider()
	{
        m_scalefactor   = 1.0f;
        m_atlas         = texture_atlas_new( 1024, 1024, 1 );
        m_bitmap        = NULL;
        m_empty_glyph   = texture_font_get_empty( m_atlas );
	}

	glyph_freetype2_provider::~glyph_freetype2_provider()
	{
        texture_glyph_delete( m_empty_glyph );
	}
    
    void glyph_freetype2_provider::set_scale(float scale)
    {
        m_scalefactor = scale;
    }
    
    face_entity2 * glyph_freetype2_provider::get_face_entity(const tu_string& fontname, bool is_bold, bool is_italic)
    {
        // form hash key
		tu_string key = fontname;
		if (is_bold)
		{
			key += "B";
		}
		if (is_italic)
		{
			key += "I";
		}
        gc_ptr<face_entity2> fe;
		if (m_face_entity.get(key, &fe))
		{
			return fe.get_ptr();
		}

        fe = new face_entity2();
        
        m_face_entity.add(key, fe);
        return fe.get_ptr();
    }
    
    extern bool get_fontfile(const char* font_name, tu_string& file_name, bool is_bold, bool is_italic);

	bitmap_info * glyph_freetype2_provider::get_char_image(	character_def* shape_glyph, Uint16 code, 
															const tu_string& fontname, bool is_bold, bool is_italic, int fontsize,
															rect* bounds, float* advance)
	{
        fontsize = ceil(fontsize * m_scalefactor);
        
		// form hash key
		int key = (fontsize << 16) | code;
        
        
        if(!m_bitmap)
        {
            if (m_atlas->depth == 1) {
                m_bitmap = render::create_bitmap_info_alpha(m_atlas->width, m_atlas->height, m_atlas->data);
            }
            else if(m_atlas->depth == 3)
            {
                // if lcd filter
                image::rgb *_rgb = new image::rgb(m_atlas->width, m_atlas->height);
                _rgb->m_data = m_atlas->data;
                m_bitmap = render::create_bitmap_info_rgb(_rgb);
            }
            else if(m_atlas->depth == 4)
            {
                image::rgba *_rgb = new image::rgba(m_atlas->width, m_atlas->height);
                _rgb->m_data = m_atlas->data;
                m_bitmap = render::create_bitmap_info_rgba(_rgb);
            }
        }

        file_opener_callback openFile = get_file_opener_callback();
        assert(openFile);
       
        glyph_entity* ge = NULL;
        face_entity2* fe = get_face_entity(fontname, is_bold, is_italic);
        if (fe && fe->m_ge.get(key, &ge) == false)
        {
            std::auto_ptr<tu_file> file( openFile(fontname) );
            assert(file.get());

            texture_glyph_t *glyph = NULL;

            if( file->get_error() )
            {
                glyph = m_empty_glyph;
            }
            else
            {
                fe->m_font = texture_font_new( m_atlas, fontname.c_str(), file.get(), fontsize );
                glyph = texture_font_get_glyph( fe->m_font, code );
            }

            ge = new glyph_entity();

            if(!glyph) {
                return ge->m_bi;
            }

            if(m_bitmap)
            {
                unsigned char * data = m_atlas->data;
                m_bitmap->update(m_atlas->width, m_atlas->height, (char *)data);
            }

            rect uv;
            uv.m_x_max = glyph->s0;
            uv.m_y_max = glyph->t0;
            uv.m_x_min = glyph->s1;
            uv.m_y_min = glyph->t1;

            ge->m_bi = create_bitmap_info(m_bitmap, glyph->width, glyph->height,  uv);
            
            ge->m_bounds.m_x_max = glyph->width;// * 0.9375f;
            ge->m_bounds.m_y_max = glyph->height;// * 0.9375f;
            
            ge->m_bounds.m_x_min = glyph->offset_x;
            ge->m_bounds.m_y_min = int( glyph->height ) - glyph->offset_y;

        
            float scale   = (16.0f / fontsize) * 64.0f;	// hack
            ge->m_advance = (float) glyph->advance_x * scale;// * fontsize * 1.6;

            
            ge->m_bounds.m_x_max /= m_scalefactor;
            ge->m_bounds.m_y_max /= m_scalefactor;
            ge->m_bounds.m_x_min /= m_scalefactor;
            ge->m_bounds.m_y_min /= m_scalefactor;
       //     ge->m_advance /= m_scalefactor;

            if( fe->m_font )
            {
                texture_font_delete( fe->m_font );
                fe->m_font = NULL;
            }

            fe->m_ge.add(key, ge);
        }
    
        if (bounds)
        {
            *bounds = ge->m_bounds;
        }

        if (advance)
        {
            *advance = ge->m_advance;
        }

		return ge->m_bi;
	}
};
#endif