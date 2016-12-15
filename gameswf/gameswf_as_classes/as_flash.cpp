// as_flash.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf/gameswf_as_classes/as_flash.h"
#include "gameswf/gameswf_as_classes/as_geom.h"
#include "gameswf/gameswf_as_classes/as_event.h"

namespace gameswf
{
	//
	// flash object
	//

	Object* flash_init(player* player)
	{
		// Create built-in flash object.
		Object*	flash_obj = new Object(player);

		// constant
		flash_obj->builtin_member("geom", geom_init(player));

		// flash9, events handler
		flash_obj->builtin_member("Events",  event_init(player));

		return flash_obj;
	}

}
