// as_class.h	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script 3 Class object

#ifndef GAMESWF_AS_CLASS_H
#define GAMESWF_AS_CLASS_H

#include "Function.h"
#include "Trait.h"
#include "Abc.h"

namespace gameswf {

    // ** class Class
	class Class : public Function {
    friend class Package;
    public:

        // ** enum Flags
        enum Flags {
            Sealed      = 0x1,
            Final       = 0x2,
            Interface   = 0x4,
        };

	public:

                                Class( player* player, Class* superClass, const Str& name, Uint8 flags, CreateInstanceThunk createInstance = NULL, Function* init = NULL, Function* staticInit = NULL );

        // ** Object
        virtual const char*     to_string() { static char str[64]; sprintf( str, "[Class %s]", m_name.c_str() ); return str; }

        // ** Class
        void                    initialize( void );
        Value                   createInstance( void ) const;
        void                    construct( const Value& instance, const Arguments& args = Arguments(), Frame* parentFrame = NULL ) const;
        const Str&              name( void ) const;
        const Function*         init( void ) const;
        Class*                  superClass( void ) const;
        bool                    is( const Class* type ) const;
        bool                    isSealed( void ) const;
        void                    setClassTraits( Traits* value );
        Traits*                 instanceTraits( void ) const;
        void                    setInstanceTraits( Traits* value );
        Package*                package( void ) const;

        bool                    findBuiltIn( const Str& name, Value* value ) const;
        void                    addBuiltIn( const Str& name, const Value& value );

    private:

        // ** Function
        virtual Value           execute( Frame* frame ) const;

	private:

        Str               		m_name;
        Uint8                   m_flags;
        gc_ptr<Function>        m_init;
        gc_ptr<Function>        m_staticInit;
        TraitsPtr               m_instanceTraits;
        TraitsPtr               m_classTraits;
        weak_ptr<Package>       m_package;
		weak_ptr<Class>         m_superClass;
        CreateInstanceThunk     m_createInstance;
        bool                    m_isInitialized;
        Members                 m_builtIn;
	};

    typedef gc_ptr<Class>   ClassPtr;
    typedef weak_ptr<Class> ClassWeak;
    typedef array<ClassPtr> Classes;
}


#endif
