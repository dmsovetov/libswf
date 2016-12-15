//
//  EventDispatcher.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 17.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "EventDispatcher.h"

#include "gameswf/avm/Domain.h"
#include "gameswf/avm/Class.h"

namespace gameswf
{

// -------------------------------------------------------- EventDispatcher -------------------------------------------------------- //

// ** EventDispatcher::EventDispatcher
EventDispatcher::EventDispatcher( player* player ) : Object( player )
{
    m_class = player->get_domain()->findClassQualified( "flash.events.EventDispatcher" );
    assert( m_class != NULL );
}

// ** EventDispatcher::addEventListener
void EventDispatcher::addEventListener( const Str& type, Function* listener, bool useCapture, unsigned int priority, bool useWeakReference )
{
    Listener l;
    l.m_type     = type;
    l.m_function = listener;

    m_listeners.push_back( l );
}

// ** EventDispatcher::dispatchEvent
bool EventDispatcher::dispatchEvent( Event* event )
{
    const Str& type   = event->type();
    Value      args[] = { event };

    for( int i = 0, n = ( int )m_listeners.size(); i < n; i++ ) {
        if( m_listeners[i].m_type == type ) {
            m_listeners[i].m_function->call( args, 1 );
        }
    }

    return true;
}

// ** EventDispatcher::hasEventListener
bool EventDispatcher::hasEventListener( const Str& type )
{
    for( int i = 0; i < ( int )m_listeners.size(); ) {
        if( m_listeners[i].m_type == type ) {
            return true;
        }
    }

    return false;
}

// ** EventDispatcher::removeEventListener
void EventDispatcher::removeEventListener( const Str& type, Function* listener, bool useCapture )
{
    for( int i = 0; i < ( int )m_listeners.size(); ) {
        if( m_listeners[i].m_type == type && m_listeners[i].m_function.get() == listener ) {
            m_listeners.remove( i );
        } else {
            i++;
        }
    }
}

// ** EventDispatcher::willTrigger
bool EventDispatcher::willTrigger( const Str& type )
{
    return hasEventListener( type );
}

// ** EventDispatcher::alive
void EventDispatcher::alive( void )
{
    Object::alive();
}

// ** EventDispatcher::this_alive
void EventDispatcher::this_alive( void )
{
    Object::this_alive();

    for( int i = 0, n = ( int )m_listeners.size(); i < n; i++ ) {
        m_listeners[i].m_function->this_alive();
    }
}

// ---------------------------------------------------------------- Event -------------------------------------------------------------- //

// ** Event::Event
Event::Event( player* player, const Str& type ) : Object( player ), m_type( type )
{
    m_class = player->get_domain()->findClassQualified( "flash.events.Event" );
    assert( m_class != NULL );
}

// ** Event::type
const Str& Event::type( void ) const
{
    return m_type;
}

// --------------------------------------------------------- EventDispatcherClosure ----------------------------------------------------- //


void EventDispatcherClosure::addEventListener( Frame* frame )
{
    EventDispatcher* e = cast_to<EventDispatcher>( frame->instance() );
    assert(e);

    Str         type     = frame->arg(0).asString();
    Function*   listener = frame->arg(1).asFunction();

    e->addEventListener( type, listener );
}

void EventDispatcherClosure::dispatchEvent( Frame* frame )
{
    EventDispatcher* e = cast_to<EventDispatcher>( frame->instance() );
    assert(e);

    Event* event = cast_to<Event>( frame->arg(0).asObject() );
    e->dispatchEvent( event );
}

void EventDispatcherClosure::hasEventListener( Frame* frame )
{
    EventDispatcher* e = cast_to<EventDispatcher>( frame->instance() );
    assert(e);

    Str type = frame->arg(0).asString();

    e->hasEventListener( type );
}

void EventDispatcherClosure::removeEventListener( Frame* frame )
{
    EventDispatcher* e = cast_to<EventDispatcher>( frame->instance() );
    assert(e);

    Str         type     = frame->arg(0).asString();
    Function*   listener = frame->arg(1).asFunction();

    e->removeEventListener( type, listener );
}

void EventDispatcherClosure::willTrigger( Frame* frame )
{
    EventDispatcher* e = cast_to<EventDispatcher>( frame->instance() );
    assert(e);

    Str type = frame->arg(0).asString();

    e->willTrigger( type );
}

}