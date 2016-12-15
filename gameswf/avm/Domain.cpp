//
//  TopLevel.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 07.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Domain.h"

#include "Package.h"
#include "Class.h"
#include "Array.h"
#include "Error.h"
#include "Linker.h"

#include "gameswf/gameswf_as_classes/as_string.h"
#include "gameswf/flash/events/EventDispatcher.h"
#include "gameswf/gameswf_stream.h"
#include "base/tu_file.h"

#define NativeClosure( function )   new FunctionNative( m_player.get_ptr(), function )
#define Readonly( function )        Value( function, Value() )
#define Property( getter, setter )  Value( getter, setter )

namespace gameswf {

void as_global_trace( Frame* frame );

// ------------------------------------------------ Domain ------------------------------------------------ //

// ** Domain::Domain
Domain::Domain( player *player ) : m_player( player )
{

}

// ** Domain::multiname
Multiname* Domain::multiname( int index ) const
{
    return m_multinames[index].get();
}

// ** Domain::setMultinames
void Domain::setMultinames( const Multinames& value )
{
    m_multinames = value;
}

// ** Domain::setStrings
void Domain::setStrings( const Strings& value )
{
    m_strings = value;
}

// ** Domain::setFunctions
void Domain::setFunctions( const FunctionScripts& value )
{
    m_functions = value;
}

// ** Domain::registerClass
Class* Domain::registerClass( Package* package, const Str& name, const Str& superClass, CreateInstanceThunk createInstance, FunctionNative* init ) const
{
    Class* sup = superClass != "" ? findClassQualified( superClass ) : NULL;
    Class* cls = new Class( m_player.get_ptr(), sup, name, 0, createInstance, init );
    package->registerClass( cls );

    return cls;
}

// ** Domain::registerPackages
void Domain::registerPackages( void )
{
#if !AVM2_INTERNAL_BUILTIN
    AbcInfo* builtin = new AbcInfo( m_player.get_ptr() );
    stream*  in      = new stream( new tu_file( "builtin.abc", "rb" ) );
    builtin->read( in, NULL );

    Linker linker( this, builtin, m_player.get_ptr() );
    linker.link();
#else
    Package* package = resolvePackage( "" );

    package->registerFunction( "trace", as_global_trace );
    package->set_member( "undefined", Value::undefined );

    {
        Class* cls = registerClass( package, "Object", "", ObjectClosure::newOp );
        cls->addBuiltIn( "hasOwnProperty", ObjectClosure::hasOwnProperty );
        cls->addBuiltIn( "toString", ObjectClosure::toString );
    }

    {
        Class* cls = registerClass( package, "Number", "Object", ObjectClosure::newOp );
    }

    {
        Class* cls = registerClass( package, "Boolean", "Object", ObjectClosure::newOp );
    }

    {
        Class* cls = registerClass( package, "int", "Object", ObjectClosure::newOp );
    }

    {
        Class* cls = registerClass( package, "uint", "Object", ObjectClosure::newOp );
    }


    {
        Class* cls = registerClass( package, "Math", "" );
        cls->set_member( "PI", M_PI );
        cls->set_member( "pow", NativeClosure( MathClosure::pow ) );
    }

    {
        Class* cls = registerClass( package, "String", "Object", StringClosure::newOp, NativeClosure( StringClosure::init ) );
        cls->addBuiltIn( "split", NativeClosure( StringClosure::split ) );
        cls->addBuiltIn( "slice", NativeClosure( StringClosure::slice ) );
        cls->addBuiltIn( "substr", NativeClosure( StringClosure::substr ) );
        cls->addBuiltIn( "substring", NativeClosure( StringClosure::substring ) );
        cls->addBuiltIn( "concat", NativeClosure( StringClosure::concat ) );
        cls->addBuiltIn( "charAt", NativeClosure( StringClosure::charAt ) );
        cls->addBuiltIn( "charCodeAt", NativeClosure( StringClosure::charCodeAt ) );
        cls->addBuiltIn( "indexOf", NativeClosure( StringClosure::indexOf ) );
        cls->addBuiltIn( "lastIndexOf", NativeClosure( StringClosure::lastIndexOf ) );
        cls->addBuiltIn( "length", Readonly( StringClosure::length ) );
        cls->addBuiltIn( "toUpperCase", NativeClosure( StringClosure::toUpperCase ) );
        cls->addBuiltIn( "toLowerCase", NativeClosure( StringClosure::toLowerCase ) );
        cls->addBuiltIn( "lastIndexOf", NativeClosure( StringClosure::lastIndexOf ) );
        cls->set_member( "fromCharCode", NativeClosure( StringClosure::fromCharCode ) );
    }

    {
        Class* cls = registerClass( package, "Array", "Object", ArrayClosure::newOp, NativeClosure( ArrayClosure::init ) );
        cls->addBuiltIn( "push", ArrayClosure::push );
        cls->addBuiltIn( "pop", ArrayClosure::pop );
        cls->addBuiltIn( "splice", ArrayClosure::splice );
        cls->addBuiltIn( "slice", ArrayClosure::slice );
        cls->addBuiltIn( "shift", ArrayClosure::shift );
        cls->addBuiltIn( "concat", ArrayClosure::concat );
        cls->addBuiltIn( "every", ArrayClosure::every );
        cls->addBuiltIn( "some", ArrayClosure::some );
        cls->addBuiltIn( "sort", ArrayClosure::sort );
        cls->addBuiltIn( "sortOn", ArrayClosure::sortOn );
        cls->addBuiltIn( "forEach", ArrayClosure::forEach );
        cls->addBuiltIn( "filter", ArrayClosure::filter );
        cls->addBuiltIn( "map", ArrayClosure::map );
        cls->addBuiltIn( "indexOf", ArrayClosure::indexOf );
        cls->addBuiltIn( "lastIndexOf", ArrayClosure::lastIndexOf );
        cls->addBuiltIn( "join", ArrayClosure::join );
        cls->addBuiltIn( "reverse", ArrayClosure::reverse );
        cls->addBuiltIn( "length", Readonly( ArrayClosure::length ) );
        cls->set_member( "CASEINSENSITIVE", Array::CaseInsensitive );
        cls->set_member( "DESCENDING", Array::Descending );
        cls->set_member( "UNIQUESORT", Array::UniqueSort );
        cls->set_member( "RETURNINDEXEDARRAY", Array::ReturnIndexedArray );
        cls->set_member( "NUMERIC", Array::Numeric );
    }

    {
        Class* cls = registerClass( package, "Error", "Object", ErrorClosure::newOp, NativeClosure( ErrorClosure::init ) );
        cls->addBuiltIn( "getStackTrace", ErrorClosure::getStackTrace );
        cls->addBuiltIn( "errorID", Readonly( ErrorClosure::errorId ) );
        cls->addBuiltIn( "message", Property( ErrorClosure::message, ErrorClosure::setMessage ) );
        cls->addBuiltIn( "name", Property( ErrorClosure::name, ErrorClosure::setName ) );
    }
#endif
}

// ** Domain::registerPackage
void Domain::registerPackage( Package *package )
{
    m_packages[package->name()] = package;
    AVM2_VERBOSE( "Toplevel package '%s' registered\n", package->name().c_str() );
}

// ** Domain::resolvePackage
Package* Domain::resolvePackage( const Str& name )
{
    if( Package* result = findPackage( name ) ) {
        return result;
    }

    Package* result = new Package( this, m_player.get_ptr(), name );
    registerPackage( result );

    return result;
}

// ** Domain::findPackage
Package* Domain::findPackage( const Str& name ) const
{
    gc_ptr<Package> result;
    m_packages.get( name, &result );

    return result.get();
}

// ** Domain::findClassQualified
Class* Domain::findClassQualified( const Str& name, bool initialize ) const
{
    const char* sep = strrchr( name.c_str(), '.' );

    Str packageName = Str( name.c_str(), sep ? (sep - name.c_str()) : 0 );
    Str className   = sep ? (sep + 1) : name.c_str();

    return findClass( packageName, className, initialize );
}

// ** Domain::findClass
Class* Domain::findClass( const Str& package, const Str& name, bool initialize ) const
{
    if( Package* pack = findPackage( package ) ) {
        return pack->findClass( name, initialize );
    }

    return NULL;
}

// ------------------------------------------- FlashDomain ------------------------------------------------ //

// ** FlashDomain::FlashDomain
FlashDomain::FlashDomain( player* player ) : Domain( player )
{

}

// ** FlashDomain::registerPackages
void FlashDomain::registerPackages( void )
{
    Domain::registerPackages();

    registerEvents();
    registerDisplay();
}

// ** FlashDomain::registerEvents
void FlashDomain::registerEvents( void )
{
    Package* package = resolvePackage( "flash.events" );

    {
        Class* cls = registerClass( package, "EventDispatcher", "Object", EventDispatcherClosure::newOp );
        cls->addBuiltIn( "addEventListener", NativeClosure( EventDispatcherClosure::addEventListener ) );
        cls->addBuiltIn( "dispatchEvent", NativeClosure( EventDispatcherClosure::dispatchEvent ) );
        cls->addBuiltIn( "hasEventListener", NativeClosure( EventDispatcherClosure::hasEventListener ) );
        cls->addBuiltIn( "removeEventListener", NativeClosure( EventDispatcherClosure::removeEventListener ) );
        cls->addBuiltIn( "willTrigger", NativeClosure( EventDispatcherClosure::willTrigger ) );
    }

    {
        Class* cls = registerClass( package, "Event", "Object" );
        cls->set_member( "ADDED", "ADDED" );
        cls->set_member( "ADDED_TO_STAGE", "ADDED_TO_STAGE" );
        cls->set_member( "ENTER_FRAME", "ENTER_FRAME" );
    }
}

// ** FlashDomain::registerDisplay
void FlashDomain::registerDisplay( void )
{
    Package* package = resolvePackage( "flash.display" );

    {
        Class* cls = registerClass( package, "DisplayObject", "flash.events.EventDispatcher" );
    }

    {
        Class* cls = registerClass( package, "InteractiveObject", "flash.display.DisplayObject" );
    }

    {
        Class* cls = registerClass( package, "DisplayObjectContainer", "flash.display.InteractiveObject" );
    }

    {
        Class* cls = registerClass( package, "Sprite", "flash.display.DisplayObjectContainer" );
    }

    {
        Class* cls = registerClass( package, "MovieClip", "flash.display.Sprite" );
    }
}

}