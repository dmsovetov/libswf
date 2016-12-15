//
//  ScopeStack.h
//  GameSWF
//
//  Created by Советов Дмитрий on 07.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__ScopeStack__
#define __GameSWF__ScopeStack__

#include "gameswf/gameswf.h"
#include "Value.h"

namespace gameswf {

    class AbcInfo;

    class Stack : private array<Value> {
    public:

                                Stack( int size = 0 ) { /*if( size ) resize( size );*/ }

        void                    push( const Value& v, const char* pushedBy = "" ) { push_back( v ); m_pushedBy.push_back( pushedBy ); }
        Value                   pop( void ) { Value v = back(); pop_back(); m_pushedBy.pop_back(); return v; }
        void                    arguments( Arguments& args, int count );
        const Value&            top( int index = 0 ) const { return (*this)[size() - index - 1]; }
        int                     size( void ) const { return array<Value>::size(); }
        const Value&            at( int index ) const { return (*this)[index]; }
        void                    swap( void );
        void                    drop( int count );
        void                    clear( void ) { array<Value>::clear(); m_pushedBy.clear(); }
        Str                     pushedBy( int index ) const { return m_pushedBy[index]; }

    private:

        array<Str>              m_pushedBy;
    };

    // ** class ScopeStack
    class ScopeStack {
    public:

                                ScopeStack( int size = 0 );
                                ScopeStack( const ScopeStack& other );
                                ~ScopeStack( void );

        Object*                 operator [] ( int index ) const;

        Object*                 at( int index ) const;
        int                     size( void ) const;
        void                    push( Object* value, const char* pushedBy = "" );
        Object*                 find( const Multiname* name, Value* value, const Class* accessScope ) const;
        void                    clear( void ) { m_stack.clear(); m_pushedBy.clear(); }

        Object*                 top( int index = 0 ) const { return (*this)[size() - index - 1]; }
        void                    pop( void );
        Str                     pushedBy( int index ) const { return m_pushedBy[index]; }
        void                    setOuter( const ScopeStack* value );

    private:

        ScopeStack*             m_outer;
        array<gc_ptr<Object> >  m_stack;
        array<Str>              m_pushedBy;
    };

} // namespace gameswf

#endif /* defined(__GameSWF__ScopeStack__) */
