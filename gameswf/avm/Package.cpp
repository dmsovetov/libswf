//
//  Package.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 07.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Package.h"

#include "Class.h"

namespace gameswf
{

// ** Package::Package
Package::Package( Domain* domain, player* player, const Str& name ) : Object( player ), m_domain( domain ), m_name( name )
{

}

// ** Package::to_string
const char* Package::to_string( void )
{
    if( m_name == "" ) {
        return "[Package toplevel]";
    }

    static Str str;
    str = Str( "[Package " ) + m_name + "]";
    return str.c_str();
}

// ** Package::get_member
bool Package::get_member( const Str& name, Value* val )
{
    Class* cls = findClass( name );

    if( cls ) {
        if( val ) *val = cls;
        return true;
    }

    if( m_functions.get( name, val ) ) {
        return true;
    }

    return Object::get_member( name, val );
}

// ** Package::findClass
Class* Package::findClass( const Str& name, bool initialize )
{
    gc_ptr<Class> result;

    if( !m_classes.get( name, &result ) ) {
        return NULL;
    }

    if( initialize ) {
        result->initialize();
    }

    return result.get();
}

// ** Package::registerClass
void Package::registerClass( Class* cls )
{
    cls->m_package = this;
    m_classes[cls->m_name] = cls;
    AVM2_VERBOSE( "Class '%s' registered inside package '%s'\n", cls->m_name.c_str(), m_name.c_str() );
}

// ** Package::registerFunction
void Package::registerFunction( const Str& name, const Value& function )
{
    m_functions[name] = function;
    AVM2_VERBOSE( "Function '%s' registered inside package '%s'\n", name.c_str(), m_name.c_str() );
}

// ** Package::name
const Str& Package::name( void ) const
{
    return m_name;
}

// ** Package::domain
Domain* Package::domain( void ) const
{
    return m_domain;
}

} // namespace gameswf