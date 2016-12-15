// array.h	-- Thatcher Ulrich <tu@tulrich.com> 2003, Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Array implementation code for the gameswf SWF player library.


#ifndef GAMESWF_AS_ARRAY_H
#define GAMESWF_AS_ARRAY_H

#include "gameswf/gameswf_action.h"	// for Object
#include "gameswf/avm/Function.h"

namespace gameswf
{

    // ** class Array
	class Array : public Object {
    public:

        enum {
			CaseInsensitive     = 1,
			Descending          = 2,
			UniqueSort          = 4,
			ReturnIndexedArray  = 8,
			Numeric             = 16
        };

                            AvmDeclareType( AS_ARRAY, Object )

                            Array( player* player, int size = 0 );
                            Array( player* player, const ValueArray& values );

        // ** Object
		virtual const char* to_string( void );
        virtual void        copy_to( Object* target );
        virtual bool        set_member( const Str& name, const Value& value );
        virtual bool        get_member( const Str& name, Value* value );

        // ** Array
		int                 push( const ValueArray& values );
        int                 push( const Value& value );
        void                resize( int size );
        bool                every( Function* function, const Value& instance ) const;
        bool                some( Function* function, const Value& instance ) const;
        Value               filter( Function* function, const Value& instance ) const;
        Value               map( Function* function, const Value& instance ) const;
        void                forEach( Function* function, const Value& instance ) const;
	//	void                remove( int index, Value* val );
     //   void                insert( int index, const Value& val );
        Value               splice( int startIndex, int deleteCount, const ValueArray& values = ValueArray() );
        Value               slice( int startIndex = 0, int endIndex = -1 ) const;
		Value               pop( void );
        Value               concat( const ValueArray& values ) const;
	//	void                erase( const String& index);
	//	void                clear( void );
        Value               shift( void );
	//	void                sort( int options, Function* compare );
		int                 length( void ) const;
        const ValueArray&   items( void ) const;
        int                 indexOf( const Value& value, int fromIndex = 0 ) const;
        int                 lastIndexOf( const Value& value, int fromIndex = -1 ) const;
        Value               join( const Str& separator = "," ) const;
        Value               reverse( void );
        void                sort( const Value& first, const Value& second );
        void                sortOn( const Value& fieldName, const Value& options = Value::null );

    private:

        Str                 m_stringValue;
        ValueArray          m_array;
	};

    // ** class ArrayComparator
    class ArrayComparator {
    public:

        typedef int ( *CompareThunk )( const Value& a, const Value& b );

        // ** enum Type
        enum Type {
            Numeric,
            Text,
            TextCaseInsensitive
        };

    public:

                            ArrayComparator( Function* function, const StrArray& fields, const IntegerArray& flags );

        Type                typeFromFlags( int flags ) const;
        CompareThunk        comparatorFromFlags( int flags ) const;
        bool operator() ( const Value& a, const Value& b );

    private:

        int                 compareProperties( const Str& name, int flags, const Value& a, const Value& b ) const;
        bool                compareProperties( int count, const Value& a, const Value& b ) const;

        static int          compare( const Value& a, const Value& b );
        static int          compareNumeric( const Value& a, const Value& b );
        static int          compareCaseInsensitive( const Value& a, const Value& b );

    private:

        Function*           m_function;
        IntegerArray        m_flags;
        StrArray            m_fields;
        int                 m_descend;
        CompareThunk        m_compareValues;
    };

    AvmBeginClass( Array )
        AvmDeclareMethod( init )
        AvmDeclareMethod( ctor )
        AvmDeclareMethod( toString )
        AvmDeclareMethod( push )
        AvmDeclareMethod( shift )
        AvmDeclareMethod( splice )
        AvmDeclareMethod( slice )
        AvmDeclareMethod( concat )
        AvmDeclareMethod( unshift )
        AvmDeclareMethod( sort )
        AvmDeclareMethod( sortOn )
        AvmDeclareMethod( pop )
        AvmDeclareMethod( length )
        AvmDeclareMethod( every )
        AvmDeclareMethod( some )
        AvmDeclareMethod( map )
        AvmDeclareMethod( forEach )
        AvmDeclareMethod( filter )
        AvmDeclareMethod( indexOf )
        AvmDeclareMethod( lastIndexOf )
        AvmDeclareMethod( join )
        AvmDeclareMethod( reverse )
    AvmEndClass
/*
	// this is "_global.Array" object
	struct as_global_array : public FunctionNative
	{
		enum option
		{
			CASEINSENSITIVE = 1,
			DESCENDING = 2,
			UNIQUESORT = 4,
			RETURNINDEXEDARRAY = 8,
			NUMERIC = 16
		};

		as_global_array(player* player);
	};
*/

}	// end namespace gameswf


#endif // GAMESWF_AS_ARRAY_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
