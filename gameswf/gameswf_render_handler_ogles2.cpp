// gameswf_render_handler_ogles2.cpp	-- Yuriy Zapoev <zapoev.yuriy@gmail.com> 2013

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// OpenGL ES2 based video handler for mobile units

#include "base/tu_config.h"

#ifdef TU_USE_OGLES2
#include <string.h>	// for memset()

#ifdef _WIN32
	#error "Use PoverVX SDK"
#elif defined(__ANDROID__)
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#elif defined(__APPLE__)
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
#else
	#error "unknown platform"
#endif

#include "gameswf.h"
#include "gameswf_types.h"
#include "gameswf_character.h"

struct t_vertex_type
{
    float        position[3];
    float        texcoord[2];
    unsigned int color;
    union
    {
        struct
        {
            float        transform0[4];
            float        transform1[4];
            float        transform2[4];
            float        transform3[4];
        };
        float m16[16];
    };
};

enum {
    kVertexAttrib_Position,
    kVertexAttrib_TexCoords,
    kVertexAttrib_Color,
    kVertexAttrib_Transform0,
    kVertexAttrib_Transform1,
    kVertexAttrib_Transform2,
    kVertexAttrib_Transform3,
    
    kVertexAttrib_MAX,
};

using namespace gameswf;


#define APPEND_VERTEX(x, y, z, c, u, v, p, idx) \
    p[idx].position[0] = x; p[idx].position[1] = y; p[idx].position[2] = z;     \
    p[idx].texcoord[0] = u; p[idx].texcoord[1] = v;                             \
    p[idx].color = c;

#define APPEND_VERTEX_TRANSFORM(m, p, idx)                           \
    memcpy(&p[idx].transform0, &m[0],  sizeof(float)*16);


#define APPEND_VERTEX_FULLY(x, y, z, c, u, v, m, p, idx)    \
        APPEND_VERTEX(x, y, z, c, u, v, p, idx)             \
        APPEND_VERTEX_TRANSFORM(m, p, idx)


float * ortho_center_rh(float * mat, float l, float r, float b, float t, float zNear, float zFar)
{
	mat[0] = 2.0f / (r - l);			mat[1] = 0.0f;				mat[2]  = 0.0f;						mat[3] = 0.0f;
	mat[4] = 0.0f;						mat[5] = 2.0f / (t - b);	mat[6]  = 0.0f;						mat[7] = 0.0f;
	mat[8] = 0.0f;						mat[9] = 0.0f;				mat[10] = 1.0f / (zNear - zFar);	mat[11] = 0.0f;
	mat[12] = (1.0f + r)/(1.0f - r);    mat[13] = (t + b)/(b - t);	mat[14] = -zNear / (zFar - zNear);	mat[15] = 1.0f;
	return mat;
}

float * identity(float * mat)
{
    mat[0] = 1.0f; mat[4] = 0.0f; mat[8] = 0.0f;  mat[12] = 0.0f;
    mat[1] = 0.0f; mat[5] = 1.0f; mat[9] = 0.0f;  mat[13] = 0.0f;
    mat[2] = 0.0f; mat[6] = 0.0f; mat[10] = 1.0f; mat[14] = 0.0f;
    mat[3] = 0.0f; mat[7] = 0.0f; mat[11] = 0.0f; mat[15] = 1.0f;
    
	return mat;
}

inline float* const transpose(float* pOut, const float* pIn) throw ()
{
    int x, z;
    
    for (z = 0; z < 4; ++z) {
        for (x = 0; x < 4; ++x) {
            pOut[(z * 4) + x] = pIn[(x * 4) + z];
        }
    }
    
    return pOut;
}

matrix affine_transform(float x, float y, float sx, float sy)
{
    matrix t, s;

    t.concatenate_translation(x, y);
    s.set_scale_rotation(sx, sy, 0.0f);
    t.concatenate(s);

    return t;
}

const char * vert_shader = "" \
	"#ifdef GL_ES                   \n" \
	"precision highp float;         \n" \
	"#endif                         \n" \
	"attribute vec4 a_position;     \n" \
	"attribute vec4 a_color;        \n" \
	"attribute vec2 a_texcoord;	    \n" \
	"attribute vec4 a_transform0;   \n" \
	"attribute vec4 a_transform1;   \n" \
	"attribute vec4 a_transform2;   \n" \
	"attribute vec4 a_transform3;   \n" \
	"                               \n" \
    "uniform  mat4 u_MVPMatrix;     \n" \
    "                               \n" \
	"varying vec2 v_texcoord;       \n" \
	"varying vec4 v_color;          \n" \
	"                               \n" \
	"void main()                                        \n" \
	"{                                                  \n" \
	"     v_texcoord =  a_texcoord;                     \n" \
	"     v_color    =  a_color;                        \n" \
	"     float x = dot(a_position, a_transform0);      \n" \
	"     float y = dot(a_position, a_transform1);      \n" \
	"     float z = dot(a_position, a_transform2);      \n" \
	"     float w = dot(a_position, a_transform3);      \n" \
    "     gl_Position = u_MVPMatrix * vec4(x, y, z, w); \n" \
	"}                                                  \n";

const char * frag_shader = "" \
	"#ifdef GL_ES                    \n" \
	"  precision highp float;        \n" \
	"#endif                          \n" \
    "                                \n" \
	"uniform sampler2D   u_texture;  \n" \
    "                                \n" \
	"varying vec2 v_texcoord;        \n" \
	"varying vec4 v_color;           \n" \
	"                                \n" \
	"void main()                     \n" \
	"{                               \n" \
    "   vec4 c = texture2D(u_texture, v_texcoord);    \n"    \
    "   gl_FragColor = c * v_color;//vec4(1.0);         \n"   \
	"}                              \n";

const char * frag_shader_c = "" \
    "#ifdef GL_ES                   \n" \
    "  precision highp float;       \n" \
    "#endif                         \n" \
    "                               \n"
    "varying vec2 v_texcoord;       \n" \
    "varying vec4 v_color;          \n" \
    "                               \n" \
    "void main()                    \n" \
    "{                              \n" \
    "   gl_FragColor = v_color;     \n" \
    "}                              \n";

const char * frag_shader_a = "" \
    "#ifdef GL_ES                   \n" \
    "  precision highp float;       \n" \
    "#endif                         \n" \
    "                               \n" \
    "uniform sampler2D   u_texture; \n" \
    "                               \n" \
    "varying vec2 v_texcoord;       \n" \
    "varying vec4 v_color;          \n" \
    "                               \n" \
    "void main()                    \n" \
    "{                              \n" \
    "   vec4 c = texture2D(u_texture, v_texcoord);              \n" \
    "   lowp float alpha = c.a;                                 \n" \
    "   gl_FragColor = vec4(v_color.rgb, v_color.a * alpha);    \n" \
    "}                              \n";

struct shader_layout
{
    GLuint program;
    GLint  mat_location;
    GLint  tex_location;
};

void create_texture(int format, int w, int h, void* data, int level)
{
	int internal_format = format;
	glTexImage2D(GL_TEXTURE_2D, level, internal_format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
}

GLint create_program(const char * vsd, const char * psd)
{   
	GLint i = 0;
	char str[65536] = "";
	GLint compiled = 0;
    
	assert(vsd);
	assert(psd);
    
	GLint uiProgram = glCreateProgram();
    
	GLint uiVertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLint uiFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
	//vertex
	glShaderSource(uiVertexShader, 1, &vsd, NULL);
    glCompileShader(uiVertexShader);
    glGetShaderiv(uiVertexShader, GL_COMPILE_STATUS, &compiled);
	glGetShaderiv(uiVertexShader, GL_INFO_LOG_LENGTH, &i);
    if (!compiled)
	{
		glGetShaderInfoLog(uiVertexShader, i, &i, str);
		str[i-1] = '\0';
		printf( "\n VS compile log = \n'%s'", str);
	}
	else if(i>1)
	{
		glGetShaderInfoLog(uiVertexShader, i, &i, str);
		str[i-1] = '\0';
		printf( "\n VS compile log = '%s'", str);
	}
    
	//fragment
	glShaderSource(uiFragmentShader, 1, &psd, NULL);
    glCompileShader(uiFragmentShader);
    glGetShaderiv(uiFragmentShader, GL_COMPILE_STATUS, &compiled);
 	glGetShaderiv(uiFragmentShader, GL_INFO_LOG_LENGTH, &i);
	if (!compiled)
	{
		glGetShaderInfoLog(uiFragmentShader, i, &i, str);
		str[i-1] = '\0';
		printf("\n PS compile log = '%s'", str);
	}
	else if(i>1)
	{
		glGetShaderInfoLog(uiFragmentShader, i, &i, str);
		str[i-1] = '\0';
		printf("\n PS compile log = '\n%s'", str);
	}
    
	if( uiVertexShader )    glAttachShader( uiProgram, uiVertexShader );
	if( uiFragmentShader )	glAttachShader( uiProgram, uiFragmentShader );
    
    glBindAttribLocation(uiProgram, kVertexAttrib_Position,   "a_position");
    glBindAttribLocation(uiProgram, kVertexAttrib_TexCoords,  "a_texcoord");
    glBindAttribLocation(uiProgram, kVertexAttrib_Color,      "a_color");
    glBindAttribLocation(uiProgram, kVertexAttrib_Transform0, "a_transform0");
    glBindAttribLocation(uiProgram, kVertexAttrib_Transform1, "a_transform1");
    glBindAttribLocation(uiProgram, kVertexAttrib_Transform2, "a_transform2");
    glBindAttribLocation(uiProgram, kVertexAttrib_Transform3, "a_transform3");
    /**/
	glLinkProgram( uiProgram );
    
	glDeleteShader(uiVertexShader);
	glDeleteShader(uiFragmentShader);
    
    glGetProgramiv( uiProgram, GL_LINK_STATUS, &compiled );
    
    if( !compiled  )
	{
		glGetShaderiv(uiProgram, GL_INFO_LOG_LENGTH, &i);
		glGetShaderInfoLog(uiProgram, sizeof(str), NULL, str );
		str[i-1] = '\0';
		printf("\n GLSL compile log = '%s'\n", str);
        
        glDeleteShader(uiProgram);
        return 0;
	}
    
    return uiProgram;
}

// choose the resampling method:
// 1 = hardware (experimental, should be fast, somewhat buggy)
// 2 = fast software bilinear (default)
// 3 = use image::resample(), slow software resampling
#define RESAMPLE_METHOD 2


// bitmap_info_ogl declaration
struct bitmap_info_ogl : public gameswf::bitmap_info
{
	unsigned int	m_texture_id;
    unsigned int    m_format;
    rect            m_uv;
    bitmap_info*    m_atlas;
	int m_width;
	int m_height;
	image::image_base* m_suspended_image;

	bitmap_info_ogl();
	bitmap_info_ogl(int width, int height, Uint8* data);
	bitmap_info_ogl(image::rgb* im);
	bitmap_info_ogl(image::rgba* im);
    bitmap_info_ogl(bitmap_info* atlas, int width, int height, const rect& uv);

	virtual void layout();

	// get byte per pixel
	virtual int get_bpp() const
	{
		if (m_suspended_image)
		{
			switch (m_suspended_image->m_type)
			{
				default: return 0;
				case image::image_base::RGB: return 3;
				case image::image_base::RGBA: return 4;
				case image::image_base::ALPHA: return 1;
			};
		}
		return 0;
	}

	virtual unsigned char* get_data() const
	{
		if (m_suspended_image)
		{
			return m_suspended_image->m_data;
		}
		return NULL;
	}

	virtual void activate()
	{
		assert(m_texture_id > 0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
	}

    bool update(int w, int h, char *data)
    {
        layout();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, m_format, GL_UNSIGNED_BYTE, data);
        return true;
    }

	~bitmap_info_ogl()
	{
		if (m_texture_id > 0)
		{
			glDeleteTextures(1, (GLuint*) &m_texture_id);
			m_texture_id = 0;	// for debuging
		}
		delete m_suspended_image;
	}
		
	virtual int get_width() const { return m_width; }
	virtual int get_height() const { return m_height; }

};

struct video_handler_ogles : public gameswf::video_handler
{
	GLuint          m_texture;
	float           m_scoord;
	float           m_tcoord;
	gameswf::rgba   m_background_color;

	video_handler_ogles():
		m_texture(0),
		m_scoord(0),
		m_tcoord(0),
		m_background_color(0,0,0,0)	// current background color
	{
	}

	~video_handler_ogles()
	{
		glDeleteTextures(1, &m_texture);
	}

	void display(Uint8* data, int width, int height, 
		const gameswf::matrix* m, const gameswf::rect* bounds, const gameswf::rgba& color)
	{
        assert(false);
    }

};

struct render_handler_ogles2 : public gameswf::render_handler
{
	// Some renderer state.
    unsigned short            m_indicines_count;
    unsigned short          * m_indicines_buff;
    t_vertex_type           * m_vertex_buff;
    int                       m_vertex_count;
    gameswf::bitmap_info    * m_last_bitmap_info;
    int                       m_last_blend_mode; ///!!!Plarium
    
    shader_layout           m_program_textured;
    shader_layout           m_program_colored;
    shader_layout           m_program_text;

    shader_layout       *   m_current_program;
    bitmap_blend_mode       m_curr_blend_mode;

    float                   m_proj_matrix[16];
	// Enable/disable antialiasing.
	bool                            m_enable_antialias;
    
	int                             m_mask_level;	// nested mask level
	
	// Output size.
	float                           m_display_width;
	float                           m_display_height;
//	GLuint                          m_program;
	gameswf::matrix                 m_current_matrix;
	gameswf::cxform                 m_current_cxform;
    const gameswf::scaling_grid*    m_current_scaling_grid;
    int                             m_slice_index;


	render_handler_ogles2() :
		m_enable_antialias(false),
		m_display_width(0),
		m_display_height(0),
		m_mask_level(0),
        m_indicines_count(0),
        m_indicines_buff(NULL),
        m_vertex_count(0),
        m_vertex_buff(NULL),
        m_last_bitmap_info(NULL),
        m_current_scaling_grid(NULL),
        m_slice_index(0)
	{
	}

	~render_handler_ogles2()
	{
	}

	void open()
	{
        m_vertex_count = 0;
        m_vertex_buff  = new t_vertex_type[65535]();

        m_indicines_count = 0;
        m_indicines_buff  = new unsigned short[65535]();
        
        m_program_textured.program = create_program(vert_shader, frag_shader);
        m_program_textured.mat_location = glGetUniformLocation(m_program_textured.program, "u_MVPMatrix");
        m_program_textured.tex_location = glGetUniformLocation(m_program_textured.program, "u_texture");
        
        m_program_colored.program = create_program(vert_shader, frag_shader_c);
        m_program_colored.mat_location = glGetUniformLocation(m_program_colored.program, "u_MVPMatrix");

        m_program_text.program = create_program(vert_shader, frag_shader_a);
        m_program_text.mat_location = glGetUniformLocation(m_program_text.program, "u_MVPMatrix");
        m_program_text.tex_location = glGetUniformLocation(m_program_text.program, "u_texture");
        
        GLenum err = glGetError();
        printf("\n%d", err);
		// Scan for extensions used by gameswf
	}
    
    void layout_attribs()
    {
        glEnableVertexAttribArray(kVertexAttrib_Position);
        glEnableVertexAttribArray(kVertexAttrib_TexCoords);
        glEnableVertexAttribArray(kVertexAttrib_Color);
        glEnableVertexAttribArray(kVertexAttrib_Transform0);
        glEnableVertexAttribArray(kVertexAttrib_Transform1);
        glEnableVertexAttribArray(kVertexAttrib_Transform2);
        glEnableVertexAttribArray(kVertexAttrib_Transform3);
        
        long offset = (long)m_vertex_buff;
        int diff = offsetof( t_vertex_type, position);
        glVertexAttribPointer(kVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE,  sizeof(t_vertex_type), (void*) (offset + diff));
        
        diff = offsetof( t_vertex_type, texcoord);
        glVertexAttribPointer(kVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE,  sizeof(t_vertex_type), (void*)(offset + diff));
    
        diff = offsetof( t_vertex_type, color);
        glVertexAttribPointer(kVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(t_vertex_type), (void*)(offset + diff));
        
        
        diff = offsetof( t_vertex_type, transform0);
        glVertexAttribPointer(kVertexAttrib_Transform0, 4, GL_FLOAT, GL_FALSE,  sizeof(t_vertex_type), (void*)(offset + diff));
        
        diff = offsetof( t_vertex_type, transform1);
        glVertexAttribPointer(kVertexAttrib_Transform1, 4, GL_FLOAT, GL_FALSE,  sizeof(t_vertex_type), (void*)(offset + diff));
        
        diff = offsetof( t_vertex_type, transform2);
        glVertexAttribPointer(kVertexAttrib_Transform2, 4, GL_FLOAT, GL_FALSE,  sizeof(t_vertex_type), (void*)(offset + diff));
        
        diff = offsetof( t_vertex_type, transform3);
        glVertexAttribPointer(kVertexAttrib_Transform3, 4, GL_FLOAT, GL_FALSE,  sizeof(t_vertex_type), (void*)(offset + diff));
    }
    
    void flush()
    {
        if( m_indicines_count > 0 && m_vertex_count > 0)
        {
            glDrawElements(GL_TRIANGLES, m_indicines_count, GL_UNSIGNED_SHORT, m_indicines_buff);
        }
        m_vertex_count = 0;
        m_indicines_count = 0;
    }

	void set_antialiased(bool enable)
	{
		m_enable_antialias = enable;
	}
    virtual void    set_user_data(void * , void *)
    {
        
    };
	
	struct fill_style
	{
		enum mode
		{
			INVALID,
			COLOR,
			BITMAP_WRAP,
			BITMAP_CLAMP,
			LINEAR_GRADIENT,
			RADIAL_GRADIENT,
		};
		mode                    m_mode;
		mutable gameswf::rgba   m_color;
		gameswf::bitmap_info *	m_bitmap_info;
		gameswf::matrix         m_bitmap_matrix;
		gameswf::cxform         m_bitmap_color_transform;
        bitmap_blend_mode       m_blend_mode; ///!!!Plarium
		bool                    m_has_nonzero_bitmap_additive_color;
		float                   m_width;	// for line style
		mutable float           pS[3];
		mutable float           pT[3];
		
		fill_style()
        : m_mode(INVALID),
          m_bitmap_info(NULL),
		  m_has_nonzero_bitmap_additive_color(false),
          m_blend_mode(BLEND_NORMAL) ///!!!Plarium
		{
            memset(pS, 0, sizeof(pS));
            memset(pT, 0, sizeof(pT));
		}
        const float * getS() const  {return pS;}
        const float * getP() const  {return pT;}

   		// Push our style into OpenGL.
		void	apply(/*const matrix& current_matrix*/) const
		{
			assert(m_mode != INVALID);
			
			if (m_mode == COLOR)
			{
                if( m_bitmap_info )
                {
                    m_bitmap_info->layout();
                }
//              glColor4ub(m_color.m_r, m_color.m_g, m_color.m_b, m_color.m_a);
//				glDisable(GL_TEXTURE_2D);
			}
			else if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
			{
				assert(m_bitmap_info != NULL);
///             glColor4ub(c.m_r, c.m_g, c.m_b, c.m_a);

				if (m_bitmap_info == NULL)
				{
//					glDisable(GL_TEXTURE_2D);
				}
				else
				{
					// Set up the texture for rendering.
					{
						// Do the modulate part of the color transform in the first pass.
						// The additive part, if any, needs to happen in a second pass.
                        float r = m_bitmap_color_transform.m_[0][0];
                        float g = m_bitmap_color_transform.m_[1][0];
						float b = m_bitmap_color_transform.m_[2][0];
						float a = m_bitmap_color_transform.m_[3][0];
                        m_color = gameswf::rgba( r * 255, g * 255, b * 255, a * 255 );
///						glColor4f(r, g, b, a);
					}

					m_bitmap_info->layout();
					if (m_mode == BITMAP_CLAMP)
					{
                    // !!!Plarium: disabled for scaling grid tiling
					//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					}
					else
					{
						assert(m_mode == BITMAP_WRAP);
	//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					}
				}
			}
		}


        // Return true if we need to do a second pass to make a valid color.  This is for cxforms with additive
		// parts; this is the simplest way (that we know of) to implement an additive color with stock OpenGL.
		bool	needs_second_pass() const
		{
			if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
			{
				return m_has_nonzero_bitmap_additive_color;
			}
			else
			{
				return false;
			}
		}

   		// Set OpenGL state for a necessary second pass.
		void	apply_second_pass() const
		{
			assert(needs_second_pass());

			// The additive color also seems to be modulated by the texture. So,
			// maybe we can fake this in one pass using using the mean value of 
			// the colors: c0*t+c1*t = ((c0+c1)/2) * t*2
			// I don't know what the alpha component of the color is for.
			//glDisable(GL_TEXTURE_2D);

			float r = m_bitmap_color_transform.m_[0][1] /*/ 255.0f*/;
			float g = m_bitmap_color_transform.m_[1][1] /*/ 255.0f*/;
			float b = m_bitmap_color_transform.m_[2][1] /*/ 255.0f*/;
			float a = m_bitmap_color_transform.m_[3][1] /*/ 255.0f*/;
            m_color = gameswf::rgba( r, g, b, a );

			glBlendFunc(GL_ONE, GL_ONE);
		}

		void	cleanup_second_pass() const
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}


		void	disable() { m_mode = INVALID; }
		void	set_color(gameswf::rgba color)
        {
            m_mode = COLOR;
            m_color = color;
        }
		void	set_bitmap(gameswf::bitmap_info* bi, const gameswf::matrix& m, bitmap_wrap_mode wm, const gameswf::cxform& color_transform, bitmap_blend_mode bm)
		{
			m_mode = (wm == WRAP_REPEAT) ? BITMAP_WRAP : BITMAP_CLAMP;
			m_bitmap_info = bi;
			m_bitmap_matrix = m;
            m_blend_mode = bm;
			m_bitmap_color_transform = color_transform;
			m_bitmap_color_transform.clamp();

			m_color = gameswf::rgba(
				Uint8(m_bitmap_color_transform.m_[0][0] * 255.0f),
				Uint8(m_bitmap_color_transform.m_[1][0] * 255.0f), 
				Uint8(m_bitmap_color_transform.m_[2][0] * 255.0f), 
				Uint8(m_bitmap_color_transform.m_[3][0] * 255.0f));

			if (m_bitmap_color_transform.m_[0][1] > 1.0f
			    || m_bitmap_color_transform.m_[1][1] > 1.0f
			    || m_bitmap_color_transform.m_[2][1] > 1.0f
			    || m_bitmap_color_transform.m_[3][1] > 1.0f)
			{
				m_has_nonzero_bitmap_additive_color = true;
			}
			else
			{
				m_has_nonzero_bitmap_additive_color = false;
			}
            
            float	inv_width  = 1.0f / m_bitmap_info->get_width();
			float	inv_height = 1.0f / m_bitmap_info->get_height();
            
            pS[0] = m_bitmap_matrix.m_[0][0] * inv_width;
			pS[1] = m_bitmap_matrix.m_[0][1] * inv_width;
			pS[2] = m_bitmap_matrix.m_[0][2] * inv_width;
            
			pT[0] = m_bitmap_matrix.m_[1][0] * inv_height;
			pT[1] = m_bitmap_matrix.m_[1][1] * inv_height;
			pT[2] = m_bitmap_matrix.m_[1][2] * inv_height;
		}
		bool	is_valid() const { return m_mode != INVALID; }
	};


	// Style state.
	enum style_index
	{
		LEFT_STYLE = 0,
		RIGHT_STYLE,
		LINE_STYLE,

		STYLE_COUNT
	};
	fill_style	m_current_styles[STYLE_COUNT];


	gameswf::bitmap_info*	create_bitmap_info_rgb(image::rgb* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_styleX_bitmap(), to set a
	// bitmap fill style.
	{
		return new bitmap_info_ogl(im);
	}


	gameswf::bitmap_info*	create_bitmap_info_rgba(image::rgba* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_style_bitmap(), to set a
	// bitmap fill style.
	//
	// This version takes an image with an alpha channel.
	{
		return new bitmap_info_ogl(im);
	}


	gameswf::bitmap_info*	create_bitmap_info_empty()
	// Create a placeholder bitmap_info.  Used when
	// DO_NOT_LOAD_BITMAPS is set; then later on the host program
	// can use movie_definition::get_bitmap_info_count() and
	// movie_definition::get_bitmap_info() to stuff precomputed
	// textures into these bitmap infos.
	{
		return new bitmap_info_ogl;
	}

	gameswf::bitmap_info*	create_bitmap_info_alpha(int w, int h, Uint8* data)
	// Create a bitmap_info so that it contains an alpha texture
	// with the given data (1 byte per texel).
	//
	// Munges *data (in order to make mipmaps)!!
	{
		return new bitmap_info_ogl(w, h, data);
	}

	gameswf::video_handler*	create_video_handler()
	{
		return new video_handler_ogles();
	}


	void	begin_display( gameswf::rgba bkg_color,
		int viewport_x0, int viewport_y0,
		int viewport_width, int viewport_height,
		float x0, float x1, float y0, float y1)

	{
		m_display_width = fabsf(x1 - x0);
		m_display_height = fabsf(y1 - y0);

        m_last_blend_mode = BLEND_NORMAL; ///!!!Plarium

		//glViewport(viewport_x0, viewport_y0, viewport_width, viewport_height);
        
        float tmp0[16] = {0};
        
        identity(tmp0);
        ortho_center_rh(m_proj_matrix, 0.0, x1, y1, 0.0, -1.0f, 1.0f);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        set_blend_mode(render_handler::BLEND_NORMAL);
        set_program(&m_program_textured);

        layout_attribs();
		// Clear the background, if background color has alpha > 0.
		if (bkg_color.m_a > 0)
		{
            unsigned int c = bkg_color.m_color;
            APPEND_VERTEX(x0, y0, 0.0f, c, 0.0f, 0.0f, m_vertex_buff, m_vertex_count); m_vertex_count++;
            APPEND_VERTEX(x1, y0, 0.0f, c, 0.0f, 0.0f, m_vertex_buff, m_vertex_count); m_vertex_count++;
            APPEND_VERTEX(x0, y1, 0.0f, c, 0.0f, 0.0f, m_vertex_buff, m_vertex_count); m_vertex_count++;
            APPEND_VERTEX(x1, y1, 0.0f, c, 0.0f, 0.0f, m_vertex_buff, m_vertex_count); m_vertex_count++;
            
            APPEND_VERTEX_TRANSFORM(tmp0, m_vertex_buff, 0);
            APPEND_VERTEX_TRANSFORM(tmp0, m_vertex_buff, 1);
            APPEND_VERTEX_TRANSFORM(tmp0, m_vertex_buff, 2);
            APPEND_VERTEX_TRANSFORM(tmp0, m_vertex_buff, 3);
            
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            m_vertex_count = 0;
		}
	}

	// Clean up after rendering a frame.  Client program is still
	// responsible for calling glSwapBuffers() or whatever.
	void	end_display()
	{
        flush();
        
        glDisableVertexAttribArray(kVertexAttrib_Position);
        glDisableVertexAttribArray(kVertexAttrib_TexCoords);
        glDisableVertexAttribArray(kVertexAttrib_Color);
        glDisableVertexAttribArray(kVertexAttrib_Transform0);
        glDisableVertexAttribArray(kVertexAttrib_Transform1);
        glDisableVertexAttribArray(kVertexAttrib_Transform2);
        glDisableVertexAttribArray(kVertexAttrib_Transform3);
        
        m_vertex_count = 0;
        m_indicines_count = 0;

        set_program(NULL);
        set_bitmap(NULL);
        m_vertex_count = 0;
	}

    void set_bitmap(gameswf::bitmap_info * bi)
    {
        if(m_last_bitmap_info != bi)
        {
            flush();
            m_last_bitmap_info = bi;
            if (m_last_bitmap_info) {
                m_last_bitmap_info->layout();
            }
        }
    }

    void set_program(shader_layout * program)
    {
        if(m_current_program != program)
        {
            flush();
            m_current_program = program;

            if (m_current_program) {
                glUseProgram(m_current_program->program);
                glUniformMatrix4fv(m_current_program->mat_location, 1, false, m_proj_matrix);
            //    bind_program(m_current_program->program, m_current_program->mat_location);
            }
            else
            {
                bind_program(0);
            }
        }
    }

    void bind_program(unsigned int id, unsigned int location = 0)
    {
        glUseProgram(id);

        if( id == 0 )
        {
            return;
        }

        glUniformMatrix4fv(location, 1, false, m_proj_matrix);
    }

    void set_blend_mode(render_handler::bitmap_blend_mode bm)
    {
        //        return;
        if(bm == m_curr_blend_mode)
        {
            return;
        }
        flush();
        m_curr_blend_mode = bm;
        switch (m_curr_blend_mode)
        {
            case BLEND_NORMAL:      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  break;
            case BLEND_LAYER:       break;
            case BLEND_MULTIPLY:    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);  break;
            case BLEND_SCREEN:      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);        break;
            case BLEND_LIGHTEN:     break;

            case BLEND_DIFFERENCE:  break;
            case BLEND_ADD:         glBlendFunc(GL_ONE, GL_ONE); break;
            case BLEND_SUBTRACT:    break;
            case BLEND_INVERT:      break;

            case BLEND_ALPHA:       break;
            case BLEND_ERASE:       break;
            case BLEND_OVERLAY:     break;
            case BLEND_HARDLIGHT:   break;

            default:                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
        }
    }

	// Set the current transform for mesh & line-strip rendering.
	void	set_matrix(const gameswf::matrix& m)
	{
		m_current_matrix = m;
	}


	void	set_cxform(const gameswf::cxform& cx)
	// Set the current color transform for mesh & line-strip rendering.
	{
		m_current_cxform = cx;
	}

    void    set_scaling_grid(const gameswf::scaling_grid* scalingGrid)
    {
        m_slice_index = 0;
        m_current_scaling_grid = scalingGrid;
    }

    void    set_slice_index(int index)
    {
        m_slice_index = index;
    }
	
  	// multiply current matrix with opengl matrix
	static void	apply_matrix(const gameswf::matrix& m)
	{
		float	mat[16];
		memset(&mat[0], 0, sizeof(mat));
		mat[0] = m.m_[0][0];
		mat[1] = m.m_[1][0];
		mat[4] = m.m_[0][1];
		mat[5] = m.m_[1][1];
		mat[10] = 1;
		mat[12] = m.m_[0][2];
		mat[13] = m.m_[1][2];
		mat[15] = 1;
//		glMultMatrixf(mat);
	}

	void	fill_style_disable(int fill_side)
	// Don't fill on the {0 == left, 1 == right} side of a path.
	{
		assert(fill_side >= 0 && fill_side < 2);
		m_current_styles[fill_side].disable();
	}


	void	line_style_disable()
	// Don't draw a line on this path.
	{
		m_current_styles[LINE_STYLE].disable();
	}


	void	fill_style_color(int fill_side, const gameswf::rgba& color)
	// Set fill style for the left interior of the shape.  If
	// enable is false, turn off fill for the left interior.
	{
		assert(fill_side >= 0 && fill_side < 2);

		m_current_styles[fill_side].set_color(m_current_cxform.transform(color));
	}


	void	line_style_color(gameswf::rgba color)
	// Set the line style of the shape.  If enable is false, turn
	// off lines for following curve segments.
	{
		m_current_styles[LINE_STYLE].set_color(m_current_cxform.transform(color));
	}


	void	fill_style_bitmap(int fill_side, gameswf::bitmap_info* bi, const gameswf::matrix& m,
		bitmap_wrap_mode wm, bitmap_blend_mode bm)
	{
		assert(fill_side >= 0 && fill_side < 2);
		m_current_styles[fill_side].set_bitmap(bi, m, wm, m_current_cxform, bm); ///!!!Plarium
	}
	
	void	line_style_width(float width)
	{
		m_current_styles[LINE_STYLE].m_width = width;
	}

    void    emit_vertices(int primitive_type, const void* coords, int vertex_count, const matrix& transform, float uScale = 1.0f, float vScale = 1.0f)
    {
        const float zeroU[3] = {0.0f};
        const float zeroV[3] = {0.0f};

        if( primitive_type == GL_TRIANGLES )
        {
            assert(false);
        }

        // create transposed trasform matrix (xy rotating, position)
        float	mat[16] = {0.0f};
        mat[0] = transform.m_[0][0];
        mat[1] = transform.m_[0][1];
        mat[3] = transform.m_[0][2];

		mat[4] = transform.m_[1][0];
        mat[5] = transform.m_[1][1];
        mat[7] = transform.m_[1][2];

        mat[10] = 1.0f;
        mat[11] = 1.0f;
        mat[15] = 1.0f;

        fill_style & lstyle = m_current_styles[LEFT_STYLE];
        gameswf::rgba color = lstyle.m_color;
        const float *pu     = lstyle.pS;
		const float *pv     = lstyle.pT;

        if( !(lstyle.m_mode == fill_style::BITMAP_CLAMP || lstyle.m_mode == fill_style::BITMAP_WRAP))
        {
            pu = zeroU;
            pv = zeroV;
            set_bitmap(NULL);
            set_program(&m_program_colored);
        }
        else
        {
            set_bitmap(lstyle.m_bitmap_info);
            set_program(&m_program_textured);
        }

        ///!!!Plarium
        if( m_last_blend_mode != lstyle.m_blend_mode )
        {
            flush();

            m_last_blend_mode = lstyle.m_blend_mode;
            if( m_last_blend_mode == bitmap_blend_mode::BLEND_ADD ) {
                glBlendFunc(GL_ONE, GL_ONE);
            } else {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
        }

        if( (m_last_bitmap_info != lstyle.m_bitmap_info) )
        {
            flush();

            m_last_bitmap_info = lstyle.m_bitmap_info;
            lstyle.apply();
        }/**/
        /*if( m_last_bitmap_info )
         {
         m_last_bitmap_info->layout();
         }*/


        // add vertex information to vertex bufer
        for(int i = 0; i < vertex_count; ++i)
		{
            coord_component x0 = ((coord_component*)coords)[i*2+0];
			coord_component y0 = ((coord_component*)coords)[i*2+1];

			float u = (x0 * pu[0]) + (y0 * pu[1]) + pu[2];
			float v = (x0 * pv[0]) + (y0 * pv[1]) + pv[2];

            APPEND_VERTEX_FULLY(x0, y0, 0.0f, color.m_color, u * uScale, v * vScale, mat, m_vertex_buff, m_vertex_count);

            m_vertex_count++;
        }

        // add index information to vertex buffer
        int idx = m_indicines_count;
        for (int v = 0; v < vertex_count-2; v++)
        {
            if (v & 1)
            {
                m_indicines_buff[idx++] = m_vertex_count-vertex_count + v + 0;
                m_indicines_buff[idx++] = m_vertex_count-vertex_count + v + 1;
                m_indicines_buff[idx++] = m_vertex_count-vertex_count + v + 2;
            }
            else
            {
                m_indicines_buff[idx++] = m_vertex_count-vertex_count + v + 0;
                m_indicines_buff[idx++] = m_vertex_count-vertex_count + v + 2;
                m_indicines_buff[idx++] = m_vertex_count-vertex_count + v + 1;
            }
        }
        m_indicines_count = idx;
/*
        if (m_current_styles[LEFT_STYLE].needs_second_pass())
		{
			m_current_styles[LEFT_STYLE].apply_second_pass();
			glDrawArrays(primitive_type, 0, vertex_count);
			m_current_styles[LEFT_STYLE].cleanup_second_pass();
		}*/
    }

    #define USE_SLICE9_TESSELATION (1)

    void    draw_slice(int primitive_type, const void* coords, int vertex_count)
    {
        assert(m_current_scaling_grid != NULL);
        assert(m_slice_index != -1);

        float sx = m_current_matrix.m_[0][0];   // ** No rotations for slice9 grids
        float sy = m_current_matrix.m_[1][1];   // ** No rotations for slice9 grids
        float x  = m_current_matrix.m_[0][2];
        float y  = m_current_matrix.m_[1][2];

        m_current_scaling_grid->calculate_slice(m_slice_index, x, y, sx, sy);

    #if USE_SLICE9_TESSELATION
        draw_tesselated_slice(primitive_type, coords, vertex_count, x, y, sx, sy);
    #else
        matrix t = affine_transform(x, y, sx, sy);
        emit_vertices(primitive_type, coords, vertex_count, t, sx, sy);
    #endif
    }

    void    draw_tesselated_slice(int primitive_type, const void* coords, int vertex_count, float x, float y, float sx, float sy)
    {
    //    float height = m_current_scaling_grid->get_vertical_border_size();
        float height = m_current_scaling_grid->m_height;
        int   count  = floorf(fabs(sy));

        y += ceil(-height * fabs(sy) * 0.5f + height * 0.5f);

        for( int i = 0; i < count; i++ )
        {
            draw_tesselated_slice_row(primitive_type, coords, vertex_count, x, y, sx, sy < 0.0f ? -1.0f : 1.0f);
            y += height;
        }

        float hExtra = sy - count;
        if( hExtra > 0.0f )
        {
            draw_tesselated_slice_row(primitive_type, coords, vertex_count, x, y - height * 0.5f + height * hExtra * 0.5f, sx, hExtra);
        }
    }

    void    draw_tesselated_slice_row(int primitive_type, const void* coords, int vertex_count, float x, float y, float sx, float sy)
    {
        assert(m_current_scaling_grid != NULL);
        assert(m_slice_index != -1);

     //   float width = m_current_scaling_grid->get_horizontal_border_size();
        float width  = m_current_scaling_grid->m_width;
        int   count = floorf(sx);

        x += ceil(-width * sx * 0.5f + width * 0.5f);

        for( int i = 0; i < count; i++ )
        {
            matrix transform = affine_transform(x, y, 1.0f, sy);
            emit_vertices(primitive_type, coords, vertex_count, transform, 1.0f, fabs(sy));

            x += width;
        }

        float wExtra = sx - count;
        if( wExtra > 0.0f )
        {
            matrix transform = affine_transform(x - width * 0.5f + width * wExtra * 0.5f, y, wExtra, sy);
            emit_vertices(primitive_type, coords, vertex_count, transform, wExtra, 1.0f / sy);
        }
    }

	// Helper for draw_mesh_strip and draw_triangle_list.
	void	draw_mesh_primitive(int primitive_type, const void* coords, int vertex_count)
	{
        if( m_current_scaling_grid )
        {
            draw_slice(primitive_type, coords, vertex_count);
            return;
        }

        emit_vertices(primitive_type, coords, vertex_count, m_current_matrix);
	}

	void draw_mesh_strip(const void* coords, int vertex_count)
	{
		draw_mesh_primitive(GL_TRIANGLE_STRIP, coords, vertex_count);
	}
			
	void	draw_triangle_list(const void* coords, int vertex_count)
	{
		draw_mesh_primitive(GL_TRIANGLES, coords, vertex_count);
	}

  	// Draw the line strip formed by the sequence of points.
	void	draw_line_strip(const void* coords, int vertex_count)
	{
        return; ///!!!
		// Set up current style.
		m_current_styles[LINE_STYLE].apply();

		// apply line width

		float scale = fabsf(m_current_matrix.get_x_scale()) + fabsf(m_current_matrix.get_y_scale());
		float w = m_current_styles[LINE_STYLE].m_width * scale / 2.0f;
        w = TWIPS_TO_PIXELS(w);


		glLineWidth(w <= 1.0f ? 1.0f : w);

		apply_matrix(m_current_matrix);

		glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
		
        // Draw a round dot on the beginning and end coordinates to lines.
        glDrawArrays(GL_POINTS, 0, vertex_count);
		// restore defaults
        glLineWidth(1);

	}

	// Draw a rectangle textured with the given bitmap, with the
	// given color.	 Apply given transform; ignore any currently
	// set transforms.
	//
	// Intended for textured glyph rendering.
	void	draw_bitmap( const gameswf::matrix& m, gameswf::bitmap_info* bi, const gameswf::rect& coords, const gameswf::rect& _uv, gameswf::rgba color)
	{
        bitmap_info_ogl* img = static_cast<bitmap_info_ogl*>( bi );

		assert(bi);
        assert(img->m_atlas);

        set_bitmap(img->m_atlas);
        set_blend_mode(BLEND_NORMAL);
        set_program(&m_program_text);

        rect uv =img->m_uv;

        float mat[16] = {0.0f};
        mat[0] = m.m_[0][0]; mat[4] = m.m_[1][0]; mat[10] = 1.0f;
        mat[1] = m.m_[0][1]; mat[5] = m.m_[1][1]; mat[11] = 1.0f;
        mat[3] = m.m_[0][2]; mat[7] = m.m_[1][2]; mat[15] = 1.0f;


        gameswf::point a = gameswf::point(coords.m_x_max, coords.m_y_min);
        gameswf::point b = gameswf::point(coords.m_x_min, coords.m_y_min);
        gameswf::point c = gameswf::point(coords.m_x_max, coords.m_y_max);
        gameswf::point d = gameswf::point(b.m_x + c.m_x - a.m_x, b.m_y + c.m_y - a.m_y);
		d.m_x = b.m_x + c.m_x - a.m_x;
		d.m_y = b.m_y + c.m_y - a.m_y;


        //       int idx = m_indicines_count;
        m_indicines_buff[m_indicines_count++] = m_vertex_count + 0;
        m_indicines_buff[m_indicines_count++] = m_vertex_count + 2;
        m_indicines_buff[m_indicines_count++] = m_vertex_count + 1;

        m_indicines_buff[m_indicines_count++] = m_vertex_count + 2;
        m_indicines_buff[m_indicines_count++] = m_vertex_count + 1;
        m_indicines_buff[m_indicines_count++] = m_vertex_count + 3;
        //        m_indicines_count += 6;

        unsigned int co = color.m_color;
        APPEND_VERTEX_FULLY(a.m_x, a.m_y, 0.0f, co, uv.m_x_min, uv.m_y_min, mat, m_vertex_buff, m_vertex_count);m_vertex_count++;
        APPEND_VERTEX_FULLY(b.m_x, b.m_y, 0.0f, co, uv.m_x_max, uv.m_y_min, mat, m_vertex_buff, m_vertex_count);m_vertex_count++;
        APPEND_VERTEX_FULLY(c.m_x, c.m_y, 0.0f, co, uv.m_x_min, uv.m_y_max, mat, m_vertex_buff, m_vertex_count);m_vertex_count++;
        APPEND_VERTEX_FULLY(d.m_x, d.m_y, 0.0f, co, uv.m_x_max, uv.m_y_max, mat, m_vertex_buff, m_vertex_count);m_vertex_count++;
	}
	
	bool test_stencil_buffer(const gameswf::rect& bound, Uint8 pattern)
	{
		return false;
	}

	void begin_submit_mask()
	{
        flush();
		if (m_mask_level == 0)
		{
			assert(glIsEnabled(GL_STENCIL_TEST) == false);
            glDepthMask(GL_TRUE);
			glEnable(GL_STENCIL_TEST);
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT);
		}

		// disable framebuffer writes
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// we set the stencil buffer to 'm_mask_level+1' 
		// where we draw any polygon and stencil buffer is 'm_mask_level'
		glStencilFunc(GL_EQUAL, m_mask_level++, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); 
	}

	// called after begin_submit_mask and the drawing of mask polygons
	void end_submit_mask()
	{
        flush();
		// enable framebuffer writes
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_FALSE);

		// we draw only where the stencil is m_mask_level (where the current mask was drawn)
		glStencilFunc(GL_EQUAL, m_mask_level, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);	
	}

	void disable_mask()
	{
        flush();
		assert(m_mask_level > 0);
		if (--m_mask_level == 0)
		{
			glDisable(GL_STENCIL_TEST); 
			return;
		}

		// begin submit previous mask

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, 0);
        glDepthMask(GL_TRUE);

		// we set the stencil buffer to 'm_mask_level' 
		// where the stencil buffer m_mask_level + 1
		glStencilFunc(GL_EQUAL, m_mask_level + 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR); 

		// draw the quad to fill stencil buffer
        float x = 0.0f;
        float y = 0.0f;
        float w = m_display_width;
        float h = m_display_height;
		
        APPEND_VERTEX(x, y, 0.0f, 0, 0.0f, 0.0f, m_vertex_buff, m_vertex_count);
        APPEND_VERTEX(w, y, 0.0f, 0, 0.0f, 0.0f, m_vertex_buff, m_vertex_count);
        APPEND_VERTEX(x, h, 0.0f, 0, 0.0f, 0.0f, m_vertex_buff, m_vertex_count);
        APPEND_VERTEX(w, h, 0.0f, 0, 0.0f, 0.0f, m_vertex_buff, m_vertex_count);
        
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		end_submit_mask();
	}

	bool is_visible(const gameswf::rect& bound)
	{
		gameswf::rect viewport;
		viewport.m_x_min = 0;
		viewport.m_y_min = 0;
		viewport.m_x_max = m_display_width;
		viewport.m_y_max = m_display_height;
		return viewport.bound_test(bound);
	}

};	// end struct render_handler_ogles2


// Code from Alex Streit
//
// Creates an OpenGL texture of the specified dst dimensions, from a
// resampled version of the given src image.  Does a bilinear
// resampling to create the dst image.
void	software_resample( int bytes_per_pixel, int src_width, int src_height, int src_pitch, uint8* src_data, int dst_width, int dst_height)
{
//	printf("original bitmap %dx%d, resampled bitmap %dx%d\n", src_width, src_height, dst_width, dst_height);

	assert(bytes_per_pixel == 3 || bytes_per_pixel == 4);

//	assert(dst_width >= src_width);
//	assert(dst_height >= src_height);

//	unsigned int	internal_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;
	unsigned int	input_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;

	// FAST bi-linear filtering
	// the code here is designed to be fast, not readable
	Uint8* rescaled = new Uint8[dst_width * dst_height * bytes_per_pixel];
	float Uf, Vf;           // fractional parts
	float Ui, Vi;           // integral parts
	float w1, w2, w3, w4;	// weighting
	Uint8* psrc;
	Uint8* pdst = rescaled;

	// i1,i2,i3,i4 are the offsets of the surrounding 4 pixels
	const int i1 = 0;
	const int i2 = bytes_per_pixel;
	int i3 = src_pitch;
	int i4 = src_pitch + bytes_per_pixel;

	// change in source u and v
	float dv = (float)(src_height - 2) / dst_height;
	float du = (float)(src_width - 2) / dst_width;

	// source u and source v
	float U;
	float V = 0;

#define BYTE_SAMPLE(offset)	\
	(Uint8) (w1 * psrc[i1 + (offset)] + w2 * psrc[i2 + (offset)] + w3 * psrc[i3 + (offset)] + w4 * psrc[i4 + (offset)])

	if (bytes_per_pixel == 3)
	{
		for (int v = 0; v < dst_height; ++v)
		{
			Vf = modff(V, &Vi);
			V += dv;
			U = 0;

			for (int u = 0; u < dst_width; ++u)
			{
				Uf = modff(U, &Ui);
				U += du;

				w1 = (1 - Uf) * (1 - Vf);
				w2 = Uf * (1 - Vf);
				w3 = (1 - Uf) * Vf;
				w4 = Uf * Vf;
				psrc = &src_data[(int) (Vi * src_pitch) + (int) (Ui * bytes_per_pixel)];

				*pdst++ = BYTE_SAMPLE(0);	// red
				*pdst++ = BYTE_SAMPLE(1);	// green
				*pdst++ = BYTE_SAMPLE(2);	// blue

				psrc += 3;
			}
		}

#ifdef DEBUG_WRITE_TEXTURES_TO_PPM
		static int s_image_sequence = 0;
		char temp[256];
		sprintf(temp, "image%d.ppm", s_image_sequence++);
		FILE* f = fopen(temp, "wb");
		if (f)
		{
			fprintf(f, "P6\n# test code\n%d %d\n255\n", dst_width, dst_height);
			fwrite(rescaled, dst_width * dst_height * 3, 1, f);
			fclose(f);
		}
#endif
	}
	else
	{
		assert(bytes_per_pixel == 4);

		for (int v = 0; v < dst_height; ++v)
		{
			Vf = modff(V, &Vi);
			V += dv;
			U = 0;

			for (int u = 0; u < dst_width; ++u)
			{
				Uf = modff(U, &Ui);
				U += du;

				w1 = (1 - Uf) * (1 - Vf);
				w2 = Uf * (1 - Vf);
				w3 = (1 - Uf) * Vf;
				w4 = Uf * Vf;
				psrc = &src_data[(int) (Vi * src_pitch) + (int) (Ui * bytes_per_pixel)];

				*pdst++ = BYTE_SAMPLE(0);	// red
				*pdst++ = BYTE_SAMPLE(1);	// green
				*pdst++ = BYTE_SAMPLE(2);	// blue
				*pdst++ = BYTE_SAMPLE(3);	// alpha

				psrc += 4;
			}
		}
	}

//	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, dst_width, dst_height, 0, input_format, GL_UNSIGNED_BYTE, rescaled);
	create_texture(input_format, dst_width, dst_height, rescaled, 0);

	delete [] rescaled;
}

bitmap_info_ogl::bitmap_info_ogl() :
    m_atlas(NULL),
	m_texture_id(0),
	m_width(0),
	m_height(0),
	m_suspended_image(0)
{
}

bitmap_info_ogl::bitmap_info_ogl(image::rgba* im) :
    m_atlas(NULL),
	m_texture_id(0),
	m_width(im->m_width),
	m_height(im->m_height)
{
	assert(im);
	m_suspended_image = image::create_rgba(im->m_width, im->m_height);
	memcpy(m_suspended_image->m_data, im->m_data, im->m_pitch * im->m_height);
}

bitmap_info_ogl::bitmap_info_ogl(int width, int height, Uint8* data) :
    m_atlas(NULL),
	m_texture_id(0),
	m_width(width),
	m_height(height)
{
	assert(width > 0 && height > 0 && data);
	m_suspended_image = image::create_alpha(width, height);
	memcpy(m_suspended_image->m_data, data, m_suspended_image->m_pitch * m_suspended_image->m_height);
}

bitmap_info_ogl::bitmap_info_ogl(image::rgb* im) :
    m_atlas(NULL),
	m_texture_id(0),
	m_width(im->m_width),
	m_height(im->m_height)
{
	assert(im);
	m_suspended_image = image::create_rgb(im->m_width, im->m_height);
	memcpy(m_suspended_image->m_data, im->m_data, im->m_pitch * im->m_height);
}

bitmap_info_ogl::bitmap_info_ogl(bitmap_info* atlas, int width, int height, const rect& uv) :
    m_atlas(atlas),
    m_width(width),
    m_height(height),
    m_uv(uv)
{

}

// layout image to opengl texture memory
void bitmap_info_ogl::layout()
{
	if (m_texture_id == 0)
	{
        if (!m_suspended_image)
        {
            m_suspended_image = new image::rgb(1, 1);
            m_suspended_image->m_data[0] = 255;
            m_suspended_image->m_data[1] = 0;
            m_suspended_image->m_data[2] = 255;
        }

		// Create the texture.
//		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, (GLuint*) &m_texture_id);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);

        m_width = m_suspended_image->m_width;
		m_height = m_suspended_image->m_height;

        bool isPOT = (m_width == (m_width & ~(m_width - 1))) && (m_height == (m_height & ~(m_height - 1)));

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, isPOT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, isPOT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		int bpp = 4;
		m_format = GL_RGBA;

		switch (m_suspended_image->m_type)
		{
			case image::image_base::RGB:
			{
				bpp = 3;
				m_format = GL_RGB;
			}

			case image::image_base::RGBA:
			{
				int	w = /*p2(*/m_suspended_image->m_width/*)*/;
				int	h = /*p2(*/m_suspended_image->m_height/*)*/;
			/*	if (w != m_suspended_image->m_width || h != m_suspended_image->m_height)
				{
					// Faster/simpler software bilinear rescale.
					software_resample(bpp, m_suspended_image->m_width, m_suspended_image->m_height,
						m_suspended_image->m_pitch, m_suspended_image->m_data, w, h);
				}
				else*/
				{
					// Use original image directly.
					create_texture(m_format, w, h, m_suspended_image->m_data, 0);
				}
				break;
			}

			case image::image_base::ALPHA:
			{
                m_format = GL_ALPHA;

				int	w = m_suspended_image->m_width;
				int	h = m_suspended_image->m_height;
				create_texture(m_format, w, h, m_suspended_image->m_data, 0);

				break;
			}

			default:
				assert(0);
		}

		delete m_suspended_image;
		m_suspended_image = NULL;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
	//	glEnable(GL_TEXTURE_2D);
	}
}

bitmap_info* create_bitmap_info(bitmap_info* atlas, int w, int h, const rect& uv)
{
    return new bitmap_info_ogl(atlas, w, h, uv);
}

gameswf::render_handler * gameswf::create_render_handler_ogles2()
{
	return new render_handler_ogles2();
}

#endif	// USE_SDL

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
