// as_sharedobject.h	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script SharedObject implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_SHAREOBJECT_H
#define GAMESWF_AS_SHAREOBJECT_H

#include "gameswf/gameswf_action.h"	// for Object

namespace gameswf
{

	class as_sharedobject : public Object
	{
		static string_hash<gc_ptr<Object> > m_local;

	public:

		as_sharedobject( player * player );

		bool	get_member(const tu_string& name, Value* val);

		static gc_ptr<Object> get_local( const tu_string & name, player * player );

	};

}

#endif //GAMESWF_AS_SHAREOBJECT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
