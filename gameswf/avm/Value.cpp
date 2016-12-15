// gameswf_value.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript value type.

#include "Value.h"
#include "Class.h"
#include "Object.h"
#include "Function.h"
#include "Array.h"

#include "gameswf/gameswf.h"
#include "gameswf/gameswf_root.h"
#include "gameswf/gameswf_action.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_movie_def.h"
#include "gameswf/gameswf_as_classes/as_number.h"
#include "gameswf/gameswf_as_classes/as_boolean.h"
#include "gameswf/gameswf_as_classes/as_string.h"
#include <float.h>

namespace gameswf
{

Value Value::null = static_cast<gameswf::Object*>( NULL );
Value Value::undefined;

bool string_to_number(int* result, const char* str, int base)
// Utility.  Try to convert str to a number.  If successful,
// put the result in *result, and return true.  If not
// successful, put 0 in *result, and return false.
{
    char* tail = 0;
    *result = strtol(str, &tail, base);
    if (tail == str || *tail != 0)
    {
        // Failed conversion to Number.
        return false;
    }
    return true;
}

bool string_to_number(double* result, const char* str)
// Utility.  Try to convert str to a number.  If successful,
// put the result in *result, and return true.  If not
// successful, put 0 in *result, and return false.
{
    char* tail = 0;
    *result = strtod(str, &tail);
    if (tail == str || *tail != 0)
    {
        // Failed conversion to Number.
        return false;
    }
    return true;
}

// ** Value::Value
Value::Value( ::gameswf::Object* value ) : m_type( Undefined )
{
    setObject( value );
}

// ** Value::Value
Value::Value( Function* value ) : m_type( Undefined )
{
    setObject( value );
}

// ** Value::Value
Value::Value( float value ) : m_type( Undefined )
{
    setNumber( value );
}

// ** Value::Value
Value::Value( int value ) : m_type( Undefined )
{
    setNumber( value );
}

// ** Value::Value
Value::Value( unsigned int value ) : m_type( Undefined )
{
    setNumber( value );
}

// ** Value::Value
Value::Value( double value ) : m_type( Undefined )
{
    setNumber( value );
}

// ** Value::Value
Value::Value( bool value ) : m_type( Undefined )
{
    setBool( value );
}

// ** Value::Value
Value::Value( FunctionNative* func ) : m_type( Undefined )
{
    setFunction( func );
}

// ** Value::Value
Value::Value( FunctionNativeThunk thunk ) : m_type( Undefined )
{
    setFunction( new FunctionNative( NULL, thunk, NULL ) );
}

// ** Value::Value
Value::Value( const Value& getter, const Value& setter ) : m_type( Property )
{
    m_property = new as_property( getter, setter );
}

// ** Value::Value
Value::Value( const char* value) : m_type( Undefined )
{
    setString( value );
}

// ** Value::Value
Value::Value( const wchar_t* value )	: m_type( String )
{
    // Encode the string value as UTF-8.
    //
    // Is this dumb?  Alternatives:
    //
    // 1. store a tu_wstring instead of tu_string?
    // Bloats typical ASCII strings, needs a
    // tu_wstring type, and conversion back the
    // other way to interface with char[].
    //
    // 2. store a tu_wstring as a union with
    // tu_string?  Extra complexity.
    //
    // 3. ??
    //
    // Storing UTF-8 seems like a pretty decent
    // way to do it.  Everything else just
    // continues to work.

#if (WCHAR_MAX != MAXLONG)
    tu_string::encode_utf8_from_wchar( &m_string, (const uint16 *)value );
#else
# if (WCHAR_MAX != MAXSHORT)
# error "Can't determine the size of wchar_t"
# else
    tu_string::encode_utf8_from_wchar( &m_string, (const uint32 *)value );
# endif
#endif
}

// ** Value::Value
Value::Value( void ) : m_type( Undefined )
{
}

// ** Value::Value
Value::Value( const Value& value ) : m_type( Undefined )
{
    *this = value;
}

// ** Value::asCString
const char*	Value::asCString( void ) const
{
    return asString().c_str();
}

// ** Value::asString
const Str& Value::asString( void ) const
{
    switch( m_type ) {
    case String:                                            break;
    case Undefined: m_string = "undefined";                 break;
    case Boolean:   m_string = m_bool ? "true" : "false";   break;
    case Number:    if( isnan( m_number ) ) {
                        // @@ Moock says if value is a NAN, then result is "NaN"
                        // INF goes to "Infinity"
                        // -INF goes to "-Infinity"
                        m_string = "NaN";
                    } else {
                        char buffer[50];
                        snprintf( buffer, 50, "%.14g", m_number );
                        m_string = buffer;
                    }
                    break;

    case Object:    // Moock says, "the value that results from
                    // calling toString() on the object".
                    //
                    // The default toString() returns "[object
                    // Object]" but may be customized.
                    m_string = m_object == NULL ? "null" : m_object->to_string();
                    break;

    case Property:  assert(false);  break;
    default:        assert(0);
    }

    return m_string;
}

// ** Value::asNumber
double Value::asNumber( void ) const
{
    switch( m_type ) {
    case String:    // @@ Moock says the rule here is: if the
                    // string is a valid float literal, then it
                    // gets converted; otherwise it is set to NaN.
                    //
                    // Also, "Infinity", "-Infinity", and "NaN"
                    // are recognized.
                    double val;
                    if( !string_to_number( &val, m_string.c_str() ) ) {
                        val = get_nan();
                    }
                    return val;

    case Number:    return m_number;
    case Boolean:   return m_bool ? 1 : 0;
    case Object:    return m_object != NULL ? m_object->to_number() : 0;

    case Property:  assert(false); return get_nan();
    case Undefined: return get_nan();
    default:        return 0.0;
    }

    return 0.0f;
}

// ** Value::asBool
bool Value::asBool( void ) const
{
    switch (m_type) {
    case String:    return m_string.size() > 0 ? true : false;
    case Object:    return m_object != NULL ? m_object->to_bool() : false;
    case Property:  assert(false); return false;
    case Number:    return m_number != 0;
    case Boolean:   return m_bool;
    case Undefined: return false;
    default:        assert(0);
    }

    return false;
}

// ** Value::asObject
Object*	Value::asObject( void ) const
{
    switch( m_type ) {
    case Object:    return m_object.get();
//    case Property:  assert( false ); return NULL;
    default:        break;
    }

    return NULL;
}

// ** Value::asFunction
Function* Value::asFunction( void ) const
{
    switch( m_type ) {
    case Object:    return cast_to<Function>( m_object );
    case Property:  assert(false); return NULL;
    default:        break;
    }

    return NULL;
}

// ** Value::asFunction
Array* Value::asArray( void ) const
{
    switch( m_type ) {
    case Object:    return cast_to<Array>( m_object );
    case Property:  assert(false); return NULL;
    default:        break;
    }
    
    return NULL;
}

// ** Value::asIntegerArray
IntegerArray Value::asIntegerArray( void ) const
{
    IntegerArray result;

    if( Array* array = asArray() ) {
        const ValueArray& items = array->items();

        for( int i = 0, n = ( int )items.size(); i < n; i++ ) {
            result.push_back( items[i].asInt() );
        }
    } else {
        result.push_back( asInt() );
    }

    return result;
}

// ** Value::asStrArray
StrArray Value::asStrArray( void ) const
{
    StrArray result;

    if( Array* array = asArray() ) {
        const ValueArray& items = array->items();

        for( int i = 0, n = ( int )items.size(); i < n; i++ ) {
            result.push_back( items[i].asString() );
        }
    } else {
        result.push_back( asString() );
    }

    return result;
}

// ** Value::setObject
void Value::setObject( ::gameswf::Object* value )
{
    if( m_type == Object && m_object.get() == value ) {
        return;
    }

    dispose();
    m_type   = Object;
    m_object = value;
}

// ** Value::type
const char* Value::type( void ) const
{
    switch( m_type ) {
    case Object:    if( m_object ) {
                        return m_object->m_class != NULL ? m_object->m_class->name().c_str() : "object";
                    } else {
                        return "null";
                    }
                    break;

    case Undefined: return "undefined";
	case Boolean:   return "Boolean";
	case Number:    return "Number";
	case String:    return "String";
    default:        break;
    }

    return "unknown";
}

// ** Value::is
bool Value::is( const Class* type ) const
{
    if( !type ) {
        return false;
    }

    switch( m_type ) {
    case Object:    if( m_object && m_object->m_class != NULL ) {
                        return m_object->m_class->is( type );
                    }
                    break;
    case Boolean:   return type->name() == "Boolean";
    case Number:    return type->name() == "Number" || type->name() == "int" || type->name() == "uint";
    default:        break;
    }

    return false;
}

// ** Value::isFunction
bool Value::isFunction( void ) const
{
    return m_type == Object && cast_to<Function>( m_object.get() ) != NULL;
}

// ** Value::isArray
bool Value::isArray( void ) const
{
    return m_type == Object && cast_to<Array>( m_object.get() ) != NULL;
}

// ** Value::isStringObject
bool Value::isStringObject( void ) const
{
    typedef ::gameswf::String StringType;
    return m_type == Object && cast_to<StringType>( m_object.get() ) != NULL;
}

// ** Value::operator =
void Value::operator = ( const Value& other )
{
    switch( other.m_type ) {
    case Undefined: setUndefined();                 break;
    case Number:    setNumber( other.m_number );    break;
    case Boolean:   setBool( other.m_bool );        break;
    case String:    setString( other.m_string );    break;
    case Object:    setObject( other.m_object );    break;
    case Property:  dispose();
                    m_type      = Property;
                    m_property  = other.m_property;
                    break;

    default:        assert(0);
    }
}

// ** Value::operator ==
bool Value::operator == ( const Value& other ) const
{
    if( m_type != Property && other.m_type != Property && m_type != other.m_type)
    {
        return (isUndefined() && other.isNull()) || (isNull() && other.isUndefined());
    }

    switch( m_type ) {
    case Undefined: return other.m_type   == Undefined;
    case String:    return m_string       == other.asString();
    case Number:    return m_number       == other.asNumber();
    case Boolean:   return m_bool         == other.asBool();
    case Object:    return m_object.get() == other.asObject();
    case Property:  assert( false ); return false;
    default:        assert( false ); return false;
    }
}

// ** Value::operator !=
bool Value::operator != ( const Value& other ) const
{
    return !(*this == other);
}

// ** Value::dispose
void Value::dispose( void )
{
    m_object = NULL;
    m_property = NULL;
}

// ** Value::asProperty
Property* Value::asProperty( void ) const
{
    return isProperty() ? m_property : NULL;
}

// ** Value::setNumber
void Value::setNumber( double value )
{
    dispose();
    m_type   = Number;
    m_number = value;
}

// ** Value::setBool
void Value::setBool( bool value )
{
    dispose();
    m_type = Boolean;
    m_bool = value;
}

// ** Value::setFunction
void Value::setFunction( FunctionNative* func )
{
    setObject( func );
}

// ** Value::setString
void Value::setString( const Str& value )
{
    dispose();
    m_type   = String;
    m_string = value;
}

// ** Value::setString
void Value::setString( const char* value )
{
    dispose();
    m_type   = String;
    m_string = value;
}

/*
bool Value::abstract_equality_comparison( const Value & first, const Value & second )
{
    if (first.type_of() == second.type_of())
    {
        if( first.is_undefined() ) return true;
        if( first.is_null() ) return true;

        if( first.is_number() )
        {
            double first_number = first.to_number();
            double second_number = second.to_number();
            if( first_number == get_nan() || second_number == get_nan() )
            {
                return false;
            }

            return first_number == second_number;
        }
        else if( first.is_string() )
        {
            return first.to_tu_string() == second.to_tu_string();
        }
        else if( first.is_bool() )
        {
            return first.to_bool() == second.to_bool();
        }

        //13.Return true if x and y refer to the same object or if they refer to objects joined to each other (see
        //13.1.2). Otherwise, return false.
        // TODO: treat joined object

        return first.to_object() == second.to_object();
    }
    else
    {

        if( first.is_null() && second.is_undefined() ) return true;
        if( second.is_null() && first.is_undefined() ) return true;

        if( ( first.is_number() && second.is_string() ) 
            || (second.is_number() && first.is_string() ) )
        {
            return first.to_number() == second.to_number();
        }

        if( first.is_bool() || second.is_bool() ) return first.to_number() == second.to_number();

        // TODO:20.If Type(x) is either String or Number and Type(y) is Object,
        //return the result of the comparison x == ToPrimitive(y).
        //21.If Type(x) is Object and Type(y) is either String or Number,
        //return the result of the comparison ToPrimitive(x) == y.

        return false;
    }
}

//ECMA-262 11.8.5
Value Value::abstract_relational_comparison( const Value & first, const Value & second )
{
    return Value( first.to_number()  < second.to_number() );
    // todo
}

//
//	as_property
//
*/
as_property::as_property(const Value& getter, const Value& setter)
{
    m_getter = cast_to<Function>(getter.asObject());
    m_setter = cast_to<Function>(setter.asObject());
}

as_property::~as_property()
{
}

void as_property::setSetter( Function* value )
{
    m_setter = value;
}

void as_property::setGetter( Function* value )
{
    m_getter = value;
}

void as_property::set(Object* target, const Value& val)
{
    if (target != NULL)
    {
        if (m_setter != NULL)
        {
            Value args[] = { val };
            m_setter->call( target, args, 1 );
        }
    }
}

void as_property::get(Object* target, Value* val) const
{
    if (target == NULL)
    {
        val->setUndefined();
        return;
    }

    // env is used when m_getter->m_env is NULL
    as_environment env(target->get_player());
    if (m_getter != NULL)
    {
        *val = m_getter->call( target );
    }
}

// call static method
void as_property::get(const Value& primitive, Value* val) const
{
    if (m_getter != NULL)
    {
        *val = m_getter->call( primitive );
    }
}

}
