// as_mcloader.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script MovieClipLoader implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_MCLOADER_H
#define GAMESWF_AS_MCLOADER_H

#include "gameswf/gameswf_action.h"	// for Object
#include "gameswf/gameswf_listener.h"	// for listener

namespace gameswf
{

	void	as_global_mcloader_ctor(const fn_call& fn);

	struct as_mcloader : public Object
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_MCLOADER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return Object::is(class_id);
		}

		struct loadable_movie
		{
			loadable_movie()
			{
			}

            gc_ptr<file_download> m_file; ///!!!Plarium
			weak_ptr<character> m_target;
		};

		listener m_listeners;
		array<loadable_movie> m_lm;

		as_mcloader(player* player);
		~as_mcloader();
		
		virtual void	advance(float delta_time);

        character*      create_instance(character *target, const tu_string& fileName); ///!!!Plarium
	};

}	// end namespace gameswf


#endif // GAMESWF_AS_XMLSOCKET_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
