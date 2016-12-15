#ifndef GAMESWF_FREETYPE2_H
#define GAMESWF_FREETYPE2_H

#include "gameswf.h"
#include "gameswf_shape.h"
#include "gameswf_canvas.h"


template <class Type> struct _vec3
{
	union
	{
		Type data[3];
		struct { Type x,y,z; };
		struct { Type r,g,b; };
	};
};

template <class Type> struct _vec4
{
	union
	{
		Type data[4];
	//	struct { Type x,y,z,w; };
		struct { Type r,g,b,a; };
		struct { Type x,y,w,h; };
	};
};

typedef  _vec3<int> ivec3;
typedef  _vec4<int> ivec4;


typedef struct
{
    void * items;		/** Pointer to dynamically allocated items. */
    size_t capacity;	/** Number of items that can be held in currently allocated storage. */
    size_t size;		/** Number of items. */
    size_t item_size;	/** Size (in bytes) of a single item. */
} vector_t;

vector_t   * vector_new( size_t item_size );
void         vector_delete( vector_t *self );
const void * vector_get( const vector_t *self, size_t index );
const void * vector_back( const vector_t *self );
size_t       vector_size( const vector_t *self );
void         vector_reserve( vector_t *self, const size_t size );
void         vector_clear( vector_t *self );
void         vector_set( vector_t *self, const size_t index, const void *item );
void         vector_erase( vector_t *self, const size_t index );
void         vector_erase_range( vector_t *self, const size_t first, const size_t last );
void         vector_push_back( vector_t *self, const void *item );
void         vector_insert( vector_t *self, const size_t index, const void *item );



/**
 * A texture atlas is used to pack several small regions into a single texture.
 */

typedef struct
{
    vector_t * nodes;    // Allocated nodes
    size_t     width;    // Width (in pixels) of the underlying texture
    size_t     height;   // Height (in pixels) of the underlying texture
    size_t     depth;    // Depth (in bytes) of the underlying texture
    size_t     used;     // Allocated surface size
    
    unsigned int    id;		// Texture identity (OpenGL)
    unsigned char * data;	// Atlas data
} texture_atlas_t;

texture_atlas_t * texture_atlas_new( const size_t width, const size_t height, const size_t depth );

void        texture_atlas_delete( texture_atlas_t * self );
void        texture_atlas_upload( texture_atlas_t * self );
ivec4       texture_atlas_get_region( texture_atlas_t * self, const size_t width, const size_t height );
void        texture_atlas_set_region( texture_atlas_t * self,
                                      const size_t x, const size_t y,
                                      const size_t width, const size_t height,
                                      const unsigned char *data,
                                      const size_t stride );

void        texture_atlas_clear( texture_atlas_t * self );


/**
 *  Texture font structure.
 */
typedef struct
{
    vector_t        * glyphs;//Vector of glyphs contained in this font.
    texture_atlas_t * atlas;//Atlas structure to store glyphs data.
    
    char * filename;//Font filename
    float  size;//Font size
    void * fileData;
    unsigned long   fileSize;
    
    int hinting;//Whether to use autohint when rendering font
    
    int   outline_type;//Outline type (0 = None, 1 = line, 2 = inner, 3 = outer)
    float outline_thickness;//Outline thickness
    
    int filtering;//Whether to use our own lcd filter.
    
    unsigned char lcd_weights[5];//LCD filter weights
    float height; 
    float linegap;
    float ascender;
    float descender;
    float underline_position;
    float underline_thickness;//The thickness of the underline for this face. Only relevant for scalable formats.
    
} texture_font_t;


/**
 * A structure that describe a glyph.
 */
typedef struct
{
    wchar_t charcode;//Wide character this glyph represents
    unsigned int id;//Glyph id (used for display lists)
    
    size_t width;//Glyph's width in pixels.
    size_t height;//Glyph's height in pixels
    
    int offset_x;
    int offset_y;
    
    float advance_x;
    float advance_y;
    
    float s0;					//First normalized texture coordinate (x) of top-left corner
    float t0;					//Second normalized texture coordinate (y) of top-left corner
    
    float s1;					//First normalized texture coordinate (x) of bottom-right corner
    float t1;					//Second normalized texture coordinate (y) of bottom-right corner
    
    vector_t * kerning;         // A vector of kerning pairs relative to this glyph.
    
    int   outline_type;         //Glyph outline type (0 = None, 1 = line, 2 = inner, 3 = outer)
    float outline_thickness;	//Glyph outline thickness
    
} texture_glyph_t;

typedef struct
{
    wchar_t charcode; //Left character code in the kern pair.
    float   kerning;  //Kerning value (in fractional pixels).
} kerning_t;



texture_font_t  * texture_font_new( texture_atlas_t * atlas, const char * filename, const float size );
void              texture_font_delete( texture_font_t * self );
texture_glyph_t * texture_font_get_glyph( texture_font_t * self, wchar_t charcode );
size_t            texture_font_load_glyphs( texture_font_t * self, const wchar_t * charcodes );

void              texture_font_generate_kerning( texture_font_t *self );
float             texture_glyph_get_kerning( const texture_glyph_t * self, const wchar_t charcode );



namespace gameswf
{
	glyph_provider *  create_glyph_provider_freetype2();
    
    struct face_entity2 : public ref_counted
	{
        face_entity2():m_font(NULL) { }
		~face_entity2() { }
        
		hash<int, glyph_entity*>  m_ge;	// <code, glyph_entity>
        texture_font_t          * m_font;
	};
    
	struct glyph_freetype2_provider : public glyph_provider
	{
        glyph_freetype2_provider();
		~glyph_freetype2_provider();
        
        void              set_scale(float scale);
        face_entity2    * get_face_entity(const tu_string& fontname, bool is_bold, bool is_italic);
		bitmap_info     * get_char_image(character_def* shape_glyph, Uint16 code,
									const tu_string& fontname, bool is_bold, bool is_italic, int fontsize,
									rect* bounds, /*rect* uvbounds,*/ float* advance);
        
        float                               m_scalefactor;
        bitmap_info                       * m_bitmap;
        texture_atlas_t                   * m_atlas;
        texture_glyph_t                   * m_empty_glyph;
        string_hash<gc_ptr<face_entity2> >  m_face_entity;
	};
}

#endif