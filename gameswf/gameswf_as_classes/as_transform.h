// as_transform.h	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_AS_TRANSFORM_H
#define GAMESWF_AS_TRANSFORM_H

#include "gameswf/gameswf_action.h"	// for Object
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_as_classes/as_color_transform.h"

namespace gameswf
{

	void	as_global_transform_ctor(const fn_call& fn);

	struct as_transform : public Object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_TRANSFORM };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return Object::is(class_id);
		}

		as_transform(player* player, character* movie_clip);

		exported_module virtual bool	set_member(const tu_string& name, const Value& val);
		exported_module virtual bool	get_member(const tu_string& name, Value* val);

		gc_ptr<as_color_transform> m_color_transform;
		gc_ptr<character> m_movie;
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_TRANSFORM_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
