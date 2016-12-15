// gameswf_object.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A generic bag of attributes.	 Base-class for ActionScript
// script-defined objects.

#ifndef GAMESWF_OBJECT_H
#define GAMESWF_OBJECT_H

#include "Value.h"

#define AvmDeclareObjectType( id )  enum { m_class_id = id };   \
                                    virtual bool is( int classId ) const { return m_class_id == classId; }

#define AvmDeclareType( id, super ) enum { m_class_id = id };   \
                                    virtual bool is( int classId ) const { return m_class_id == classId ? true : super::is( classId ); }

namespace gameswf
{
    typedef string_hash<Value> Members;

    // ** class Object
	class Object : public Object_interface {
    friend class Avm;
    friend class Value;
    public:

                                    AvmDeclareObjectType( AS_OBJECT );

                                    Object( player* player );
		virtual                     ~Object( void );
		
		virtual const char*         to_string( void );
		virtual double              to_number( void );
		virtual bool                to_bool( void ) { return true; }

		//! Returns a closure of a given Function that is associated with this Object.
        FunctionClosure*            enclose( Function* function ) const;
		//! Returns a Value of a slot at a given index.
        const Value&                slot( int index ) const;
		//! Sets a Value of a slot at a given index.
        void                        setSlot( int index, const Value& value );
		//! Returns Traits object associated with this Object.
        const Traits*               traits( void ) const;
		//! Sets a Traits of this object.
        void                        setTraits( const Traits* value );
		//! Returns a Class of this object.
        Class*                      type( void ) const;
		//! Sets an Object Class.
        void                        setType( Class* value );
		//! Returns true if this Object has a public property with a given name.
        bool                        hasOwnProperty( const String* name ) const;
		//! Resolves a property inside this object by a given name and access scope.
        TraitResolution             resolveProperty( const Str& name, Value* value, const Class* accessScope = NULL ) const;
		//! Resolves a property inside this object by a given multiname and access scope.
        TraitResolution             resolveProperty( const Multiname* name, Value* value, const Class* accessScope = NULL ) const;
		//! Sets a property inside this object with a given name and access scope.
        TraitResolution             setProperty( const Str& name, const Value& value, const Class* accessScope = NULL );

		void                        builtin_member( const Str& name, const Value& value );
		virtual bool                set_member( const Str& name, const Value& value );
		virtual bool                get_member( const Str& name, Value* val );
		virtual void                clear_refs( hash<Object*, bool>* visited_objects, Object* this_ptr );
		virtual void                this_alive( void );
		virtual void                alive( void ) {}
		virtual void                copy_to( Object* target );
		Object*                     find_target( const Value& target );

		player*                     get_player( void ) const { return m_player.get_ptr(); }

    protected:
		
		//! Dynamic object properties.
        Members                     m_members;
		//! Pointer to a parent player.
		// !!!: This should be removed from here.
		weak_ptr<player>            m_player;
		//! Associated object traits.
        TraitsWeak                  m_traits;
		//! Object class.
        ClassWeak                   m_class;
		//! Function closure cache.
        mutable FunctionClosures    m_closures;
		//! The boolean flag that indicates that we are running the toString method.
		// !!!: Is this ok?
        bool                        m_isInsideToString;
		//! An array of a fixed object slots.
        ValueArray                  m_slots;
	};

    class String : public Object {
    public:

                            AvmDeclareType( AS_STRING, Object )

                            String( player* player, const Str& string = "" );

        // ** Object
        virtual const char* to_string( void ) { return m_string.c_str(); }

        // ** String
        const Str&          toString( void ) const { return m_string; }
        const char*         toCString( void ) const { return m_string.c_str(); }

        void                setString( const Str& value ) { m_string = value; }
        int                 length( void ) const { return m_string.size(); }
        Value               split( const Str& delimiter, int limit = -1 );
        Value               concat( const ValueArray& strings ) const;
        Value               slice( int startIndex = 0, int endIndex = -1 ) const;
        Value               substr( int startIndex = 0, int length = -1 ) const;
        Value               substring( int startIndex = 0, int endIndex = -1 ) const;
        Value               toUpperCase( void ) const;
        Value               toLowerCase( void ) const;
        Str                 charAt( int index = 0 ) const;
        int                 charCodeAt( int index = 0 ) const;
        int                 indexOf( const Str& value, int startIndex = 0 ) const;
        int                 lastIndexOf( const Str& value, int startIndex = -1 ) const;

        static Value        fromCharCode( player* player, const ValueArray& codes );

    private:

        Str         m_string;
    };

    AvmBeginClass( Object )
        AvmDeclareMethod( hasOwnProperty )
        AvmDeclareMethod( toString )
    AvmEndClass

    AvmBeginClass( String )
        AvmDeclareMethod( init )
        AvmDeclareMethod( length )
        AvmDeclareMethod( split )
        AvmDeclareMethod( concat )
        AvmDeclareMethod( charAt )
        AvmDeclareMethod( slice )
        AvmDeclareMethod( substr )
        AvmDeclareMethod( substring )
        AvmDeclareMethod( indexOf )
        AvmDeclareMethod( lastIndexOf )
        AvmDeclareMethod( charCodeAt )
        AvmDeclareMethod( fromCharCode )
        AvmDeclareMethod( toUpperCase )
        AvmDeclareMethod( toLowerCase )
    AvmEndClass

    AvmBeginStaticClass( Math )
        AvmDeclareMethod( pow )
    AvmEndClass

    typedef gc_ptr<String>      StringPtr;
    typedef array<StringPtr>    Strings;

    typedef gc_ptr<Object>      ObjectPtr;

}

#endif
