// as_class.cpp	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script 3 Class object

#include "Class.h"

#include "Avm.h"
#include "Function.h"
#include "Package.h"
#include "Trait.h"

namespace gameswf
{

// ** Class::Class
Class::Class( player* player, Class* superClass, const Str& name, Uint8 flags, CreateInstanceThunk createInstance, Function* init, Function* staticInit )
    : Function( player ), m_name( name ), m_flags( flags ), m_superClass( superClass ), m_createInstance( createInstance ), m_init( init ), m_staticInit( staticInit ), m_isInitialized( false )
{
    m_class = this;

    if( FunctionScript* init = cast_to<FunctionScript>( m_init.get() ) ) {
        init->setAccessScope( this );
    }
}

// ** Class::execute
Value Class::execute( Frame* frame ) const
{
    Value newInstance = createInstance();
    construct( newInstance, *frame->args() );
    frame->setResult( newInstance );

    return newInstance;
}

// ** Class::name
const Str& Class::name( void ) const
{
    return m_name;
}

// ** Class::createInstance
Value Class::createInstance( void ) const
{
    assert( m_createInstance );
    Object* instance = m_createInstance( m_player.get_ptr() );
    instance->setType( const_cast<Class*>( this ) );
    instance->setTraits( m_instanceTraits.get() );

    return instance;
}

// ** Class::setClassTraits
void Class::setClassTraits( Traits* value )
{
    assert( m_classTraits == NULL );

    m_classTraits = value;
    m_classTraits->setOwner( this );
}

// ** Class::instanceTraits
Traits* Class::instanceTraits( void ) const
{
    return m_instanceTraits.get();
}

// ** Class::addInstanceTrait
void Class::setInstanceTraits( Traits* value )
{
    assert( m_instanceTraits == NULL );

    m_instanceTraits = value;
    m_instanceTraits->setOwner( this );
}

// ** Class::initialize
void Class::initialize( void )
{
    if( m_isInitialized ) {
        return;
    }

    m_isInitialized = true;

    // ** Initialize super class
    if( m_superClass != NULL ) {
        m_superClass->initialize();
    }

    if( m_createInstance == NULL ) {
        m_createInstance = m_superClass != NULL ? m_superClass->m_createInstance : ObjectClosure::newOp;
    }

    IF_VERBOSE_PARSE(log_msg("Initializing class %s\n", m_name.c_str()));

    // ** Initialize class
    if( m_superClass != NULL && m_init ) {
        if( m_instanceTraits != NULL ) {
            m_instanceTraits->setSuper( m_superClass->m_instanceTraits.get() );
        }

        if( FunctionScript* script = cast_to<FunctionScript>( m_init ) ) {
            script->setSuper( m_superClass.get() );
        }
    }

    if( m_classTraits != NULL ) {
        m_classTraits->assignSlots();
        setTraits( m_classTraits.get() );
    }

    if( m_instanceTraits != NULL ) {
        m_instanceTraits->assignSlots();
    }

    // ** Run static initializer
    if( m_staticInit ) {
        m_staticInit->call( this );
    }
}

// ** Class::construct
void Class::construct( const Value& instance, const Arguments& args, Frame* parentFrame ) const
{
    Object* object = instance.asObject();
    if( !object ) {
        assert( false );
        return;
    }

    // ** Invoke constructor
    if( m_init ) {
        m_init->executeWithInstance( object, args, parentFrame );
    }
}

// ** Class::findBuiltIn
bool Class::findBuiltIn( const Str& name, Value* value ) const
{
    if( m_members.get( name, value ) ) {
        return true;
    }

    return m_superClass != NULL ? m_superClass->findBuiltIn( name, value ) : NULL;
}

// ** Class::addBuiltIn
void Class::addBuiltIn( const Str& name, const Value& value )
{
    m_members[name] = value;
}

// ** Class::package
Package* Class::package( void ) const
{
    return m_package.get();
}

// ** Class::is
bool Class::is( const Class* type ) const
{
    if( type == this ) {
        return true;
    }

    return m_superClass != NULL ? m_superClass->is( type ) : false;
}

// ** Class::isSealed
bool Class::isSealed( void ) const
{
    return m_flags & Sealed ? true : false;
}

}
