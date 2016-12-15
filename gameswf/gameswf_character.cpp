// gameswf_character.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_render.h"

namespace gameswf
{

	character::character(player* player, character* parent, int id)	:
		EventDispatcher(player),
		m_id(id),
		m_parent(parent),
		m_depth(0),
		m_ratio(0.0f),
		m_clip_depth(0),
		m_blend_mode(0),
		m_visible(true),
		m_display_callback(NULL),
		m_display_callback_user_ptr(NULL)
	{
		// loadMovieClip() requires that the following will be commented out
		// assert((parent == NULL && m_id == -1)	|| (parent != NULL && m_id >= 0));
	}

	bool	character::get_member(const tu_string& name, Value* val)
	// Set *val to the value of the named member and
	// return true, if we have the named member.
	// Otherwise leave *val alone and return false.
	{
		// first try character members
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			default:
				break;

			case M_GLOBAL:
                assert(false);
			//	val->setObject(get_global());
				return true;

			case M_ROOT:
			case M_LEVEL0:
				val->setObject(get_root_movie());
				return true;

			case M_THIS:
			case MDOT:
				val->setObject(this);
				return true;

			case M_X:
			{
				const matrix&	m = get_matrix();
				val->setNumber(TWIPS_TO_PIXELS(m.m_[0][2]));
				return true;
			}
			case M_Y:
			{
				const matrix&	m = get_matrix();
				val->setNumber(TWIPS_TO_PIXELS(m.m_[1][2]));
				return true;
			}
			case M_XSCALE:
			{
				const matrix& m = get_matrix();	// @@ or get_world_matrix()?  Test this.
				float xscale = m.get_x_scale();
				val->setNumber(xscale * 100);		// result in percent
				return true;
			}
			case M_YSCALE:
			{
				const matrix& m = get_matrix();	// @@ or get_world_matrix()?  Test this.
				float yscale = m.get_y_scale();
				val->setNumber(yscale * 100);		// result in percent
				return true;
			}
			case M_ALPHA:
			{
				// Alpha units are in percent.
				val->setNumber(get_cxform().m_[3][0] * 100.f);
				return true;
			}
			case M_VISIBLE:
			{
				val->setBool(get_visible());
				return true;
			}
			case M_WIDTH:
			{
				val->setNumber((int) TWIPS_TO_PIXELS(get_width()));
				return true;
			}
			case M_HEIGHT:
			{
				val->setNumber((int) TWIPS_TO_PIXELS(get_height()));
				return true;
			}
			case M_ROTATION:
			{
				// Verified against Macromedia player using samples/test_rotation.swf
				float	angle = get_matrix().get_rotation();

				// Result is CLOCKWISE DEGREES, [-180,180]
				angle *= 180.0f / float(M_PI);

				val->setNumber(angle);
				return true;
			}
			case M_TARGET:
			{
				// Full path to this object; e.g. "/_level0/sprite1/sprite2/ourSprite"
				character* parent = get_parent();
				if (parent == NULL)	// root
				{
					val->setString("/");
					return true;
				}

				Value target;
				parent->get_member("_target", &target);

					// if s != "/"(root) add "/"
				tu_string s = target.asString();
				s += s == "/" ? "" : "/";

				// add own name
				if (get_name().length() == 0)
				{
					s += "noname";
				}
				else
				{
					s += get_name();
				}

				val->setString(s);
				return true;
			}
			case M_NAME:
			{
				val->setString(get_name());
				return true;
			}
			case M_DROPTARGET:
			{
				// Absolute path in slash syntax where we were last dropped (?)
				// @@ TODO
				val->setString("/_root");
				return true;
			}
			case M_URL:
			{
				// our URL.
				val->setString("gameswf");
				return true;
			}
			case M_HIGHQUALITY:
			{
				// Whether we're in high quality mode or not.
				val->setBool(true);
				return true;
			}
			case M_FOCUSRECT:
			{
				// Is a yellow rectangle visible around a focused movie clip (?)
				val->setBool(false);
				return true;
			}
			case M_SOUNDBUFTIME:
			{
				// Number of seconds before sound starts to stream.
				val->setNumber(0.0);
				return true;
			}
			case M_XMOUSE:
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				get_mouse_state(&x, &y, &buttons);

				matrix	m = get_world_matrix();

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;

				m.transform_by_inverse(&b, a);

				val->setNumber(TWIPS_TO_PIXELS(b.m_x));
				return true;
			}
			case M_YMOUSE:
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				get_mouse_state(&x, &y, &buttons);

				matrix	m = get_world_matrix();

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;

				m.transform_by_inverse(&b, a);

				val->setNumber(TWIPS_TO_PIXELS(b.m_y));
				return true;
			}
			case M_PARENT:
			case MDOT2:
			{
				val->setObject(cast_to<Object>(m_parent.get_ptr()));
				return true;
			}
		}	// end switch

		return Object::get_member(name, val);
	}

	// TODO: call_watcher
	bool	character::set_member(const tu_string& name, const Value& val)
	{
		// first try character members
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			default:
				break;

			case M_X:
			{
				if( !isnan( val.asNumber() ) )
				{
					matrix	m = get_matrix();
					m.m_[0][2] = (float) PIXELS_TO_TWIPS(val.asNumber());
					set_matrix(m);
				}
				return true;
			}
			case M_Y:
			{
				if( !isnan( val.asNumber() ) )
				{
					matrix	m = get_matrix();
					m.m_[1][2] = (float) PIXELS_TO_TWIPS(val.asNumber());
					set_matrix(m);
				}
				return true;
			}
			case M_XSCALE:
			{
				matrix	m = get_matrix();

				// Decompose matrix and insert the desired value.
				float	x_scale = val.asFloat() / 100.f;	// input is in percent
				float	y_scale = m.get_y_scale();
				float	rotation = m.get_rotation();
				m.set_scale_rotation(x_scale, y_scale, rotation);

				set_matrix(m);
				return true;
			}
			case M_YSCALE:
			{
				matrix	m = get_matrix();

				// Decompose matrix and insert the desired value.
				float	x_scale = m.get_x_scale();
				float	y_scale = val.asFloat() / 100.f;	// input is in percent
				float	rotation = m.get_rotation();
				m.set_scale_rotation(x_scale, y_scale, rotation);

				set_matrix(m);
				return true;
			}
			case M_ALPHA:
			{
				// Set alpha modulate, in percent.
				cxform	cx = get_cxform();
				cx.m_[3][0] = val.asFloat() / 100.f;
				set_cxform(cx);
				return true;
			}
			case M_VISIBLE:
			{
				set_visible(val.asBool());
				return true;
			}
			case M_WIDTH:
			{
				if (val.asFloat() > 0)
				{
					matrix	m = get_matrix();

					// Decompose matrix and insert the desired value.
					float	x_scale = m.get_x_scale();
					float	y_scale = m.get_y_scale();
					float	rotation = m.get_rotation();

					// get current width
					float current_width = TWIPS_TO_PIXELS(get_width());

					// set new scale
					x_scale /= current_width / val.asFloat();

					m.set_scale_rotation(x_scale, y_scale, rotation);
					set_matrix(m);
				}
				return true;
			}
			case M_HEIGHT:
			{
				if (val.asFloat() > 0)
				{
					matrix	m = get_matrix();

					// Decompose matrix and insert the desired value.
					float	x_scale = m.get_x_scale();
					float	y_scale = m.get_y_scale();
					float	rotation = m.get_rotation();

					// get current height
					float current_height = TWIPS_TO_PIXELS(get_height());

					// set new scale
					y_scale /= current_height / val.asFloat();

					m.set_scale_rotation(x_scale, y_scale, rotation);
					set_matrix(m);
				}
				return true;
			}
			case M_ROTATION:
			{
				matrix	m = get_matrix();

				// Decompose matrix and insert the desired value.
				float	x_scale = m.get_x_scale();
				float	y_scale = m.get_y_scale();
				float	rotation = val.asFloat() * float(M_PI) / 180.f;	// input is in degrees
				m.set_scale_rotation(x_scale, y_scale, rotation);

				set_matrix(m);
				return true;
			}
			case M_NAME:
			{
				set_name(val.asString());
				return true;
			}
			case M_HIGHQUALITY:
			{
				// @@ global { 0, 1, 2 }
				//				// Whether we're in high quality mode or not.
				//				val->set(true);
				return true;
			}
			case M_FOCUSRECT:
			{
				//				// Is a yellow rectangle visible around a focused movie clip (?)
				//				val->set(false);
				return true;
			}
			case M_SOUNDBUFTIME:
			{
				// @@ global
				//				// Number of seconds before sound starts to stream.
				//				val->set(0.0);
				return true;
			}
		}	// end switch

		return Object::set_member(name, val);
	}

	float	character::get_width()
	{
		rect bound;
		get_bound(&bound);
		float w = bound.m_x_max - bound.m_x_min;
		return w >= FLT_MIN ? w : 0;
	}

	float	character::get_height()
	{
		rect bound;
		get_bound(&bound);
		float h = bound.m_y_max - bound.m_y_min;
		return h >= FLT_MIN ? h : 0;
	}

	void character::get_bound(rect* bound)
	{
		character_def* def = get_character_def();
		assert(def);
		def->get_bound(bound);
		get_matrix().transform(bound);
	}


//	bool	character::is_visible()
//	{
		// The increase of performance that gives the skipping of invisible characters
		// less than expenses for performance of test of visibility
		// get_bound() & get_world_matrix() are heavy recursive operations
//		return true;

//		rect bound;
//		get_bound(&bound);
		
//		matrix m;
//		m.set_inverse(get_matrix());
//		m.transform(&bound);

//		m = get_world_matrix();
//		m.transform(&bound);

//		return render::is_visible(bound);
//	}

	void character::enumerate(as_environment* env)
	{
        assert(false);
	//	Object::enumerate(env);
	}

	///!!!Plarium
    void scaling_grid::calculate_slice(int index, float& x, float& y, float& scaleX, float& scaleY) const
    {
        assert(index >= 0 && index < 9);

        point size   = calculate_size(scaleX, scaleY);
        point offset = calculate_offset(index, size);

        calculate_scale(index, size, scaleX, scaleY);

        x = ceil( x + offset.m_x);
        y = ceil( y + offset.m_y);
    }

    void scaling_grid::calculate_scale(int index, const point& size, float& scaleX, float& scaleY) const
    {
        scaleX = size.m_x / m_width  * (scaleX < 0 ? -1 : 1);
        scaleY = size.m_y / m_height * (scaleY < 0 ? -1 : 1);

        switch(index)
        {
        case CENTER_TOP:    scaleY = 1.0f; break;
        case RIGHT_CENTER:  scaleX = 1.0f; break;
        case CENTER_BOTTOM: scaleY = 1.0f; break;
        case LEFT_CENTER:   scaleX = 1.0f; break;
        case LEFT_BOTTOM:
        case RIGHT_BOTTOM:
        case RIGHT_TOP:
        case LEFT_TOP:      scaleX = 1.0f;
                            scaleY = 1.0f;
                            break;
        }
    }

    point scaling_grid::calculate_offset(int index, const point& size) const
    {
        float dx   = floorf((size.m_x - m_width)  * 0.5f);
        float dy   = floorf((size.m_y - m_height) * 0.5f);

        switch(index)
        {
        case LEFT_TOP:      return point(-dx,   -dy  );
        case CENTER_TOP:    return point( 0.0f, -dy  );
        case RIGHT_TOP:     return point( dx,   -dy  );
        case RIGHT_CENTER:  return point( dx,    0.0f);
        case RIGHT_BOTTOM:  return point( dx,    dy  );
        case CENTER_BOTTOM: return point( 0.0f,  dy  );
        case LEFT_BOTTOM:   return point(-dx,    dy  );
        case LEFT_CENTER:   return point(-dx,    0.0f);
        }

        return point(0.0f, 0.0f);
    }

    float scaling_grid::get_slice_width(int index) const
    {
        return m_bounds[index].m_x_max - m_bounds[index].m_x_min;
    }

    float scaling_grid::get_slice_height(int index) const
    {
        return m_bounds[index].m_y_max - m_bounds[index].m_y_min;
    }

    float scaling_grid::get_vertical_border_size() const
    {
        float left = get_slice_height(LEFT_CENTER);
        if( left > 0.0f ) return left;

        float right = get_slice_height(RIGHT_CENTER);
        if( right > 0.0f ) return right;

        return get_slice_height(CENTER);
    }

    float scaling_grid::get_horizontal_border_size() const
    {
        float top = get_slice_height(CENTER_TOP);
        if( top > 0.0f ) return top;

        float bottom = get_slice_height(CENTER_BOTTOM);
        if( bottom > 0.0f ) return bottom;

        return get_slice_height(CENTER);
    }

    point scaling_grid::calculate_size(float scaleX, float scaleY) const
    {
        float vBorder = m_borderSize[LEFT] + m_borderSize[RIGHT];
        float hBorder = m_borderSize[TOP] + m_borderSize[BOTTOM];

        float sizeX   = (m_width  + vBorder) * fabs( scaleX ) - vBorder;
        float sizeY   = (m_height + hBorder) * fabs( scaleY ) - hBorder;

        return point( sizeX, sizeY );
    }

    float scaling_grid::calculate_border_width(int border) const
    {
        static const int idxTop[]    = { LEFT_TOP, CENTER_TOP, RIGHT_TOP };
        static const int idxBottom[] = { LEFT_BOTTOM, CENTER_BOTTOM, RIGHT_BOTTOM };
        static const int idxLeft[]   = { LEFT_TOP, LEFT_CENTER, LEFT_BOTTOM };
        static const int idxRight[]  = { RIGHT_TOP, RIGHT_CENTER, RIGHT_BOTTOM };

        float      result  = 0.0f;
        const int* indices = NULL;

        switch(border) {
        case LEFT:   indices = idxLeft;   break;
        case RIGHT:  indices = idxRight;  break;
        case TOP:    indices = idxTop;    break;
        case BOTTOM: indices = idxBottom; break;
        }

        for( int i = 0; i < 3 && result == 0.0f; i++ ) {
            result = get_slice_width( indices[i] );
        }

        return result;
    }

    float scaling_grid::calculate_border_height(int border) const
    {
        static const int idxTop[]    = { LEFT_TOP, CENTER_TOP, RIGHT_TOP };
        static const int idxBottom[] = { LEFT_BOTTOM, CENTER_BOTTOM, RIGHT_BOTTOM };
        static const int idxLeft[]   = { LEFT_TOP, LEFT_CENTER, LEFT_BOTTOM };
        static const int idxRight[]  = { RIGHT_TOP, RIGHT_CENTER, RIGHT_BOTTOM };

        float      result  = 0.0f;
        const int* indices = NULL;

        switch(border) {
            case LEFT:   indices = idxLeft;   break;
            case RIGHT:  indices = idxRight;  break;
            case TOP:    indices = idxTop;    break;
            case BOTTOM: indices = idxBottom; break;
        }

        for( int i = 0; i < 3 && result == 0.0f; i++ ) {
            result = get_slice_height( indices[i] );
        }
        
        return result;
    }

    void scaling_grid::calculate()
    {
        static const int idxTop[]    = { LEFT_TOP, CENTER_TOP, RIGHT_TOP, CENTER };
        static const int idxBottom[] = { LEFT_BOTTOM, CENTER_BOTTOM, RIGHT_BOTTOM, CENTER };
        static const int idxLeft[]   = { LEFT_TOP, LEFT_CENTER, LEFT_BOTTOM, CENTER };
        static const int idxRight[]  = { RIGHT_TOP, RIGHT_CENTER, RIGHT_BOTTOM, CENTER };

        memset(&m_borderSize, 0, sizeof(m_borderSize));

        for( int i = 0; i < 4 && m_borderSize[LEFT] == 0.0f; i++ ) {
            m_borderSize[LEFT] = get_slice_width( idxLeft[i] );
        }

        for( int i = 0; i < 4 && m_borderSize[RIGHT] == 0.0f; i++ ) {
            m_borderSize[RIGHT] = get_slice_width( idxRight[i] );
        }

        for( int i = 0; i < 4 && m_borderSize[TOP] == 0.0f; i++ ) {
            m_borderSize[TOP] = get_slice_height( idxTop[i] );
        }

        for( int i = 0; i < 4 && m_borderSize[BOTTOM] == 0.0f; i++ ) {
            m_borderSize[BOTTOM] = get_slice_height( idxBottom[i] );
        }

        if(calculate_border_width(LEFT) == 0.0f)    m_borderSize[LEFT]    = 0.0f;
        if(calculate_border_width(RIGHT) == 0.0f)   m_borderSize[RIGHT]   = 0.0f;
        if(calculate_border_height(TOP) == 0.0f)    m_borderSize[TOP]     = 0.0f;
        if(calculate_border_height(BOTTOM) == 0.0f) m_borderSize[BOTTOM]  = 0.0f;

        m_width  = m_splitter.m_x_max - m_splitter.m_x_min;
        m_height = m_splitter.m_y_max - m_splitter.m_y_min;
    }

}
