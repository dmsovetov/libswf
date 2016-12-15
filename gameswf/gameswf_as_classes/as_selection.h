// as_color.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Selection class lets you set and control the text field in which 
// the insertion point is located (that is, the field that has focus).
// Selection-span indexes are zero-based
// (for example, the first position is 0, the second position is 1, and so on). 


#ifndef GAMESWF_AS_SELECTION_H
#define GAMESWF_AS_SELECTION_H

#include "gameswf/gameswf_action.h"	// for Object

namespace gameswf
{

	// Create built-in Selection object.
	Object* selection_init(player* player);

	struct as_selection : public Object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_SELECTION };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return Object::is(class_id);
		}

		as_selection(player* player);
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_SELECTION_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
