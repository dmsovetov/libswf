// as_sharedobject.cpp	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_sharedobject.h"
#include "gameswf/avm/Function.h"

namespace gameswf
{
	void	as_sharedobject_getlocal(const fn_call& fn)
	{
		as_sharedobject* object = cast_to<as_sharedobject>(fn.this_ptr);

		*fn.result = as_sharedobject::get_local( fn.arg( 0 ).asString(), object->get_player() ).get_ptr();
	}

	void	as_sharedobject_flush(const fn_call& fn)
	{
		//todo
	}

	as_sharedobject::as_sharedobject( player * player ) : Object( player )
	{
		builtin_member( "getLocal", &as_sharedobject_getlocal );
		builtin_member( "flush", &as_sharedobject_flush );
	}

	gc_ptr<Object> as_sharedobject::get_local( const tu_string & name, player * player )
	{
		string_hash<gc_ptr<Object> >::const_iterator it = m_local.find( name );

		if( it == m_local.end() )
		{
			gc_ptr<Object> new_object = new as_sharedobject( player );

			m_local.add( name, new_object );
			return new_object;
		}

		return it->second;
	}

	bool	as_sharedobject::get_member(const tu_string& name, Value* val)
	{
		if( Object::get_member( name, val ) )
		{
			return true;
		}

		Object * object = new Object( get_player() );

		set_member( name, object );
		val->setObject( object );

		return true;
	}

	string_hash<gc_ptr<Object> > as_sharedobject::m_local;
}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
