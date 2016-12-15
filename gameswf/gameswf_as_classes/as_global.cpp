// as_global.h	-- Thatcher Ulrich <tu@tulrich.com> 2003
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script global functions implementation

#include "gameswf/gameswf_action.h"	// for Object
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_root.h"
#include "as_global.h"

namespace gameswf
{

	//
	// Built-in objects
	//

	void	as_global_trace(Frame* frame)
	{
		assert(frame->nargs() >= 1);

        Str val;

        for( int i = 0; i < frame->nargs(); i++ ) {
            val += frame->arg(i).asString();

            if( i < (frame->nargs() - 1) ) {
                val += " ";
            }
        }

		log_msg("%s\n", val.c_str());
	}

	// setInterval(functionReference:Function, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	// setInterval(objectReference:Object, methodName:String, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	void  as_global_setinterval(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			gc_ptr<as_timer> t = new as_timer(fn.get_player());

			int first_arg_index = 2;
			if (fn.arg(0).isFunction())
			{
				t->m_func = fn.arg(0).asFunction();
				t->m_interval = fn.arg(1).asFloat() / 1000.0f;
				assert(fn.env);
				t->m_this_ptr = fn.env->get_target();
			}
			else
			if (fn.arg(0).asObject() != NULL)
			{
				Value func;
				Object* this_ptr = fn.arg(0).asObject();
				this_ptr->get_member(fn.arg(1).asString(), &func);

				t->m_func = func.asFunction();
				t->m_interval = fn.arg(2).asFloat() / 1000.0f;
				t->m_this_ptr = this_ptr;
				first_arg_index = 3;
			}
			else
			{
				// invalid args
				return;
			}

			// pass args
			t->m_arg.resize(fn.nargs - first_arg_index);
			for (int i = first_arg_index; i < fn.nargs; i++)
			{
				t->m_arg.push_back(fn.arg(i));
			}

			fn.get_root()->add_listener(t);
			fn.result->setObject(t);
		}
	}

	// setTimeout(functionReference:Function, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	// setTimeout(objectReference:Object, methodName:String, interval:Number, [param1:Object, param2, ..., paramN]) : Number
	void  as_global_settimeout(const fn_call& fn)
	{
		// create interval timer
		as_global_setinterval(fn);
		as_timer* t = cast_to<as_timer>(fn.result->asObject());

		if (t)
		{
			t->m_do_once = true;
		}
	}

	void  as_global_clearinterval(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
			fn.get_root()->remove_listener(fn.arg(0).asObject());
		}
	}

	void	as_global_update_after_event(const fn_call& fn)
	{
		// isn't required for gameswf
	}
	
	void	as_global_assetpropflags(const fn_call& fn)
	// Undocumented ASSetPropFlags function
	// Works only for Object for now
	{
        assert(false);
		int version = fn.get_player()->get_root()->get_movie_version();

		// Check the arguments
		assert(fn.nargs == 3 || fn.nargs == 4);
		assert((version == 5) ? (fn.nargs == 3) : true);

		// object
		Object* obj = fn.arg(0).asObject();
		if (obj == NULL)
		{
			log_error("error: assetpropflags for NULL object\n");
			return;
		}

		// The second argument is a list of child names,
		// may be in the form array(like ["abc", "def", "ggggg"]) or in the form a string(like "abc, def, ggggg")
		// the NULL second parameter means that assetpropflags is applied to all children
		Object* props = fn.arg(1).asObject();

		int as_prop_flags_mask = 7; // DONT_ENUM | DONT_DELETE | READ_ONLY;

		// a number which represents three bitwise flags which
		// are used to determine whether the list of child names should be hidden,
		// un-hidden, protected from over-write, un-protected from over-write,
		// protected from deletion and un-protected from deletion
		int true_flags = fn.arg(2).asInt() & as_prop_flags_mask;

		// Is another integer bitmask that works like true_flags,
		// except it sets the attributes to false. The
		// false_flags bitmask is applied before true_flags is applied

		// ASSetPropFlags was exposed in Flash 5, however the fourth argument 'false_flags'
		// was not required as it always defaulted to the value '~0'. 
		int false_flags = (fn.nargs == 3 ? 
				 (version == 5 ? ~0 : 0) : fn.arg(3).asInt()) & as_prop_flags_mask;

		// Evan: it seems that if true_flags == 0 and false_flags == 0, this function
		// acts as if the parameters where (object, null, 0x1, 0) ...
		if (false_flags == 0 && true_flags == 0)
		{
			props = NULL;
			false_flags = 0;
			true_flags = 0x1;
		}
/*
		if (props == NULL)
		{
			// Takes all members of the object and sets its property flags
			for (string_hash<Value>::const_iterator it = obj->m_members.begin();
				it != obj->m_members.end(); ++it)
			{
            #if ENABLE_AS2
				const Value& val = it->second;
				int flags = val.get_flags();
				flags = flags & (~false_flags);
				flags |= true_flags;
				val.set_flags(flags);
            #else
                assert(false);
            #endif
			}
		}
		else
		{
			// Takes all string type prop and sets property flags of obj[prop]
			for (string_hash<Value>::const_iterator it = props->m_members.begin();
				it != props->m_members.end(); ++it)
			{
				const Value& key = it->second;
				if (key.isString())
				{
					string_hash<Value>::iterator obj_it = obj->m_members.find(key.asString());
					if (obj_it != obj->m_members.end())
					{
                    #if ENABLE_AS2
						const Value& val = obj_it->second;
						int flags = val.get_flags();
						flags = flags & (~false_flags);
						flags |= true_flags;
						val.set_flags(flags);
                    #else
                        assert(false);
                    #endif
					}
				}
			}
		}
*/
	}

	// getVersion() : String
	void	as_global_get_version(const fn_call& fn)
	// Returns a string containing Flash Player version and platform information.
	{
		fn.result->setString(get_gameswf_version());
	}


	void as_timer::advance(float delta_time)
	{
		assert(m_func != NULL);

		m_time_remainder += delta_time;
		if (m_time_remainder >= m_interval)
		{
			m_time_remainder = fmod(m_time_remainder - m_interval, m_interval);

			as_environment env(m_func->get_player());
			int n = m_arg.size();
			{
				for (int i = 0; i < n; i++)
				{
					env.push(m_arg[i]);
				}
			}

			// keep alive
			gc_ptr<Object> obj = m_this_ptr.get_ptr();
			Value callback(m_func.get_ptr());

			call_method(callback, &env, obj.get_ptr(), n, env.get_top_index(), "timer");

			if (m_do_once)
			{
                assert(false);
			//	m_func->get_root()->remove_listener(this);
			}
		}
	}


};
