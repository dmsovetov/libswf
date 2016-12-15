// as_mcloader.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script MovieClipLoader implementation code for the gameswf SWF player library.

#include "gameswf/gameswf_as_classes/as_mcloader.h"
#include "gameswf/gameswf_root.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_sprite.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/avm/Function.h"
//#include "gameswf/gameswf_log.h"

namespace gameswf
{

	void	as_mcloader_addlistener(const fn_call& fn)
	{
		as_mcloader* mcl = cast_to<as_mcloader>(fn.this_ptr);
		assert(mcl);

		if (fn.nargs == 1)
		{
			mcl->m_listeners.add(fn.arg(0).asObject());
			fn.result->setBool(true);
            assert(false);
		//	mcl->get_root()->add_listener(mcl);
			return;
		}
		fn.result->setBool(false);
	}

	void	as_mcloader_removelistener(const fn_call& fn)
	{
		as_mcloader* mcl = cast_to<as_mcloader>(fn.this_ptr);
		assert(mcl);

		if (fn.nargs == 1)
		{
			mcl->m_listeners.remove(fn.arg(0).asObject());
			fn.result->setBool(true);
			return;
		}
		fn.result->setBool(false);
	}

	void	as_mcloader_loadclip(const fn_call& fn)
	{
		as_mcloader* mcl = cast_to<as_mcloader>(fn.this_ptr);
		assert(mcl);

		fn.result->setBool(false);	// on default
		if (fn.nargs == 2)
		{
			array<Value> event_args;	// for event handler args
			event_args.push_back(Value());	// undefined

            file_download_callback downloader = get_file_download_callback();
            file_download*         download   = downloader ? downloader(fn.arg(0).asString()) : NULL;

            if (!download)
            {
                IF_VERBOSE_ACTION(log_msg("can't create movie from %s\n", fn.arg(0).asCString()));
                event_args.push_back("URLNotFound");	// 2-d param
                mcl->m_listeners.notify(event_id(event_id::ONLOAD_ERROR, &event_args));
                return;
            }

			as_mcloader::loadable_movie lm;
            lm.m_file = download;
			lm.m_target = cast_to<character>(fn.env->find_target(fn.arg(1)));
			mcl->m_lm.push_back(lm);

			mcl->m_listeners.notify(event_id(event_id::ONLOAD_START, &event_args));
			fn.result->setBool(true);
		}
	}

	void	as_mcloader_unloadclip(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			fn.env->load_file("", fn.arg(0));
			fn.result->setBool(true);
			return;
		}
		fn.result->setBool(false);
	}

	void	as_mcloader_getprogress(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			sprite_instance* m = cast_to<sprite_instance>(fn.arg(0).asObject());
			if (m)
			{
				Object* info = new Object(fn.get_player());
				info->set_member("bytesLoaded", (int) m->get_loaded_bytes());
				info->set_member("bytesTotal", (int) m->get_file_bytes());
				fn.result->setObject(info);
				return;
			}
		}
		fn.result->setObject(NULL);
	}

	void	as_global_mcloader_ctor(const fn_call& fn)
	// Constructor for ActionScript class MovieClipLoader
	{
		fn.result->setObject(new as_mcloader(fn.get_player()));
	}

	as_mcloader::as_mcloader(player* player) :
		Object(player)
	{
		builtin_member("addListener", as_mcloader_addlistener);
		builtin_member("removeListener", as_mcloader_removelistener);
		builtin_member("loadClip", as_mcloader_loadclip);
		builtin_member("unloadClip", as_mcloader_unloadclip);
		builtin_member("getProgress", as_mcloader_getprogress);
	}

	as_mcloader::~as_mcloader()
	{
	}

    character* as_mcloader::create_instance(character *target, const tu_string& fileName)
    {
        player*         player = target->get_player();
        character_def*	md     = NULL;

        if (player->is_valid_movie(fileName.c_str()))
        {
            md = player->create_movie(fileName.c_str());
        }
        else
        {
            bitmap_info* bi = render::create_bitmap_info_empty();
            bi->set_identifier(fileName);
            bi->layout();
            md = new bitmap_character(player, bi);
        }

        character* ch = target->replace_me(md);

        return ch;
    }

	void	as_mcloader::advance(float delta_time)
	{
		if (m_lm.size() == 0)
		{
            assert(false);
		//	get_root()->remove_listener(this);
			return;
		}

		for (int i = 0; i < m_lm.size();)
		{
			array<Value> event_args;		// for event handler args
			event_args.push_back(m_lm[i].m_target.get_ptr());

            int loaded = m_lm[i].m_file->get_bytes_loaded();
            int total  = m_lm[i].m_file->get_bytes_total();


            // onLoadProgress
            event_args.push_back(loaded);
            event_args.push_back(total);

            m_listeners.notify(event_id(event_id::ONLOAD_PROGRESS, &event_args));

            if( loaded < total )
            {
                i++;
                continue;
            }

            // onLoadComplete
            m_listeners.notify(event_id(event_id::ONLOAD_COMPLETE, &event_args));

            // Create instance
            character* ch = create_instance(m_lm[i].m_target.get_ptr(), m_lm[i].m_file->get_path());

            event_args[0] = ch;
            m_listeners.notify(event_id(event_id::ONLOAD_INIT, &event_args));

            m_lm.remove(i);
		}
	}

};
