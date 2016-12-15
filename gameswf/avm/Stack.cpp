//
//  ScopeStack.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 07.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Stack.h"
#include "Domain.h"
#include "Abc.h"
#include "Package.h"
#include "Multiname.h"
#include "Object.h"

namespace gameswf
{

// ** Stack::swap
void Stack::swap( void )
{
    assert( size() >= 2 );
    
    int    sz = size();
    Stack& stack = *this;

    Value temp    = stack[sz - 2];
    stack[sz - 2] = stack[sz - 1];
    stack[sz - 1] = temp;
}

// ** Stack::drop
void Stack::drop( int count )
{
    while( count-- ) {
        pop();
    }
}

// ** Stack::arguments
void Stack::arguments( Arguments& args, int count )
{
    args.clear();

    AVM2_VERBOSE( "( " );
    for( int i = 0; i < count; i++ ) {
        args.push( pop() );
        AVM2_VERBOSE( "%s ", args.values().back().asCString() );
    }
    AVM2_VERBOSE( ")\n" );
}

// ------------------------------------------------------ ScopeStack ------------------------------------------------------ //

// ** ScopeStack::ScopeStack
ScopeStack::ScopeStack( int size ) : m_outer( NULL )
{
}

// ** ScopeStack::ScopeStack
ScopeStack::ScopeStack( const ScopeStack& other ) : m_outer( NULL )
{
    for( int i = 0, n = other.size(); i < n; i++ ) {
        push( other.at( i ) );
    }

    setOuter( other.m_outer );
}

ScopeStack::~ScopeStack( void )
{
    delete m_outer;
}

// ** ScopeStack::operator []
Object* ScopeStack::operator [] ( int index ) const
{
    return m_stack[index];
}

// ** ScopeStack::setOuter
void ScopeStack::setOuter( const ScopeStack* value )
{
    if( m_outer ) {
        delete m_outer;
        m_outer = NULL;
    }

    if( value ) {
        m_outer = new ScopeStack( *value );
    }
}

// ** ScopeStack::find
Object* ScopeStack::find( const Multiname* name, Value* value, const Class* accessScope ) const
{
    // ** Search a scope stack
    for( int i = m_stack.size() - 1; i >= 0; i-- )
    {
        Object* object = m_stack[i];

        if( object && object->resolveProperty( name, value, accessScope ) == TraitResolved ) {
            return object;
        }
    }

    return m_outer ? m_outer->find( name, value, accessScope ) : NULL;
}

// ** ScopeStack::size
int ScopeStack::size( void ) const
{
    return m_stack.size();
}

// ** ScopeStack::push
void ScopeStack::push( Object* value, const char* pushedBy )
{
    m_stack.push_back( value );
    m_pushedBy.push_back( pushedBy );
}

// ** ScopeStack::pop
void ScopeStack::pop( void )
{
    m_stack.pop_back();
    m_pushedBy.pop_back();
}

// ** ScopeStack::at
Object* ScopeStack::at( int index ) const
{
    return m_stack[index];
}

}