//
//  EventDispatcher.h
//  GameSWF
//
//  Created by Советов Дмитрий on 17.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__EventDispatcher__
#define __GameSWF__EventDispatcher__

#include "gameswf/avm/Function.h"

namespace gameswf {

    // ** class Event
    class Event : public Object {
    public:

                                Event( player* player, const Str& type );

        const Str&              type( void ) const;

    private:

        Str                     m_type;
    };

    // ** class EventDispatcher
    class EventDispatcher : public Object {
    public:

                                AvmDeclareType( AS_EVENT_DISPATCHER, Object )

                                EventDispatcher( player* player );

        // ** Object
        virtual void            this_alive( void );
        virtual void            alive( void );

        // ** EventDispatcher
        void                    addEventListener( const Str& type, Function* listener, bool useCapture = false, unsigned int priority = 0, bool useWeakReference = false );
        bool                    dispatchEvent( Event* event );
        bool                    hasEventListener( const Str& type );
        void                    removeEventListener( const Str& type, Function* listener, bool useCapture = false );
        bool                    willTrigger( const Str& type );

    private:

        // ** struct Listener
        struct Listener {
            Str                 m_type;
            FunctionPtr         m_function;
        };

        array<Listener>         m_listeners;
    };

    AvmBeginClass( EventDispatcher )
        AvmDeclareMethod( addEventListener )
        AvmDeclareMethod( dispatchEvent )
        AvmDeclareMethod( hasEventListener )
        AvmDeclareMethod( removeEventListener )
        AvmDeclareMethod( willTrigger )
    AvmEndClass
}

#endif /* defined(__GameSWF__EventDispatcher__) */
