// as_netconnection.cpp	-- Vitaly Alexeev <tishka92@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gameswf/gameswf_as_classes/as_netconnection.h"
#include "gameswf/avm/Function.h"

namespace gameswf
{

	void	as_global_netconnection_ctor(const fn_call& fn)
	// Constructor for ActionScript class NetConnection.
	{
		fn.result->setObject(new as_netconnection(fn.get_player()));
	}

	void	as_netconnection_connect(const fn_call& fn)
	{
		// Opens a local connection through which you can play back video files
		// from an HTTP address or from the local file system.
		as_netconnection* nc = cast_to<as_netconnection>(fn.this_ptr);
		assert(nc);
    UNUSED(nc);

		if (fn.nargs == 1)
		{
			assert(fn.env);
			
			if (fn.arg(0).isNull())
			// local file system
			{
				fn.result->setBool(true);
				return;
			}
			else
			// from an HTTP address
			{
				//todo
			}
		}

		fn.result->setBool(false);
		return;
	}

	as_netconnection::as_netconnection(player* player) :
		Object(player)
	{
		set_member("connect", &as_netconnection_connect);
	}


} // end of gameswf namespace
