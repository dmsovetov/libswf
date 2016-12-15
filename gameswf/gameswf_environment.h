// gameswf_environment.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript "environment", essentially VM state?

#ifndef GAMESWF_ENVIRONMENT_H
#define GAMESWF_ENVIRONMENT_H

#include "base/weak_ptr.h"
#include "gameswf/gameswf.h"
#include "gameswf/avm/Value.h"

namespace gameswf
{

	#define GLOBAL_REGISTER_COUNT 4

	struct character;
	struct sprite_instance;

	exported_module tu_string get_full_url(const tu_string& workdir, const char* url);

	//
	// with_stack_entry
	//
	// The "with" stack is for Pascal-like with-scoping.

	struct with_stack_entry
	{
		gc_ptr<Object>	m_object;
		int	m_block_end_pc;
		
		with_stack_entry() :
			m_object(NULL),
			m_block_end_pc(0)
		{
		}

		with_stack_entry(Object* obj, int end)	:
			m_object(obj),
			m_block_end_pc(end)
		{
		}
	};

	// stack access/manipulation
	// @@ TODO do more checking on these
	struct vm_stack : private array<Value>
	{
		vm_stack() :
			m_stack_size(0)
		{
		}

		inline Value&	operator[](int index) 
		{
//			assert(index >= 0 && index < m_stack_size);
			return array<Value>::operator[](index);
		}

		inline const Value&	operator[](int index) const 
		{
//			assert(index >= 0 && index < m_stack_size);
			return array<Value>::operator[](index);
		}

		void reset(const Value& val)
		{
			(*this)[m_stack_size] = val;
		}

		void reset(const char* val)
		{
			(*this)[m_stack_size].setString(val);
		}

		void reset(const wchar_t* val)
		{
			(*this)[m_stack_size] = Value(val);
		}

		void reset(bool val)
		{
			(*this)[m_stack_size].setBool(val);
		}

		void reset(int val)
		{
			(*this)[m_stack_size].setInt(val);
		}

		void reset(float val)
		{
			(*this)[m_stack_size].setNumber(val);
		}

		void reset(double val)
		{
			(*this)[m_stack_size].setNumber(val);
		}

		void reset(Object* val)
		{
			(*this)[m_stack_size].setObject(val);
		}

		template<class T>
		void	push(T val) 
		{
			if (m_stack_size < array<Value>::size())
			{
				// this reduces NEW operators
//				(*this)[m_stack_size] = Value(val);
				reset(val);
			}
			else
			{
				push_back(Value(val)); 
			}
			m_stack_size++;
		}

		Value&	pop();
		exported_module void	drop(int count);
		Value&	top(int dist) { return (*this)[m_stack_size - 1 - dist]; }
		Value&	bottom(int index) { return (*this)[index]; }
		inline int	get_top_index() const { return m_stack_size - 1; }
		inline int	size() const { return m_stack_size; }
		void resize(int new_size);
		void clear_refs(hash<Object*, bool>* visited_objects, Object* this_ptr);

		// return object that contains the property
		Object* find_property(const char* name);

		// get value of property
		bool get_property(const char* name, Value* val);

	private:

		int m_stack_size;
	};

	struct as_environment : public vm_stack
	{
	//	vm_stack m_scope;	// scope stack for AVM2
		Value	m_global_register[GLOBAL_REGISTER_COUNT];
		array<Value>	m_local_register;	// function2 uses this
		gc_ptr<Object>	m_target;

		// For local vars.  Use empty names to separate frames.
		struct frame_slot
		{
			tu_string	m_name;
			Value	m_value;

			frame_slot() {}
			frame_slot(const tu_string& name, const Value& val) : m_name(name), m_value(val) {}
		};
		array<frame_slot>	m_local_frames;

		weak_ptr<player> m_player;

		exported_module as_environment(player* player);
		exported_module ~as_environment();

		bool	set_member(const tu_string& name, const Value& val);
		bool	get_member(const tu_string& name, Value* val);

		int get_stack_size() const { return size(); }
		void set_stack_size(int n) { resize(n); }

		character*	get_target() const;
		void set_target(character* target);
		void set_target(Value& target, character* original_target);

	//	Value	get_variable(const tu_string& varname, const array<with_stack_entry>& with_stack) const;
		// no path stuff:
	//	Value	get_variable_raw(const tu_string& varname, const array<with_stack_entry>& with_stack) const;

	//	void	set_variable(const tu_string& path, const Value& val, const array<with_stack_entry>& with_stack);
		// no path stuff:
	//	void	set_variable_raw(const tu_string& path, const Value& val, const array<with_stack_entry>& with_stack);

	//	void	set_local(const tu_string& varname, const Value& val);
	//	void	add_local(const tu_string& varname, const Value& val);	// when you know it doesn't exist.
	//	void	declare_local(const tu_string& varname);	// Declare varname; undefined unless it already exists.

		// Parameter/local stack frame management.
		int	get_local_frame_top() const { return m_local_frames.size(); }
		void	set_local_frame_top(int t) { assert(t <= m_local_frames.size()); m_local_frames.resize(t); }
		void	add_frame_barrier() { m_local_frames.push_back(frame_slot()); }

		// Local registers.
		void	add_local_registers(int register_count)
		{
			// Flash 8 can have zero register (+1 for zero)
			m_local_register.resize(m_local_register.size() + register_count + 1);
		}
		void	drop_local_registers(int register_count)
		{
			// Flash 8 can have zero register (-1 for zero)
			m_local_register.resize(m_local_register.size() - register_count - 1);
		}

		Value* get_register(int reg);
		void set_register(int reg, const Value& val);

		// may be used in outside of class instance
		static bool	parse_path(const tu_string& var_path, tu_string* path, tu_string* var);

		// Internal.
		int	find_local(const tu_string& varname, bool ignore_barrier) const;
		character* load_file(const char* url, const Value& target, int method = 0);
		Object*	find_target(const Value& target) const;
		void clear_refs(hash<Object*, bool>* visited_objects, Object* this_ptr);
		player* get_player() const;
		root* get_root() const;

		private:

		Value*	local_register_ptr(int reg);

	};


	// Parameters/environment for C functions callable from ActionScript.
	struct fn_call
	{
		Value* result;
		Object* this_ptr;
		const Value& this_value;	// Number or String or Boolean value
		as_environment* env;
		int nargs;
		int first_arg_bottom_index;
        mutable void* user_data; ///!!!Plarium
        const char* name;        ///!!!Plarium

		fn_call(Value* res_in, const Value& this_in, as_environment* env_in, int nargs_in, int first_in, const char* name) :
			result(res_in),
			this_value(this_in),
			env(env_in),
			nargs(nargs_in),
			first_arg_bottom_index(first_in),
            user_data(NULL), ///!!!Plarium
            name(name)       ///!!!Plarium
		{
			this_ptr = this_in.asObject();
		}

		Value& arg(int n) const
		// Access a particular argument.
		{
			assert(n < nargs);
			return env->bottom(first_arg_bottom_index - n);
		}

        ValueArray args(int startIndex = 0) const
        {
            ValueArray result;

            for( int i = startIndex; i < nargs; i++ ) {
                result.push_back( arg(i) );
            }

            return result;
        }

        exported_module void attach_user_data(void* value ) const { user_data = value; } ///!!!Plarium

		exported_module player* get_player() const;
		exported_module root* get_root() const;
	};

}

#endif
