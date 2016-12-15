//
//  Error.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 18.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Error.h"
#include "Function.h"

namespace gameswf
{

// ------------------------------------------------------ Error ------------------------------------------------------ //

// ** Error::Error
Error::Error( player* player, String* message, int errorId ) : Object( player ), m_message( message ), m_errorId( errorId )
{

}

// ** Error::toString
const char* Error::to_string( void )
{
    if( m_message == NULL || m_message->length() == 0 ) {
        return "Error";
    }

    static char buf[64];
    sprintf( buf, "Error: %s", m_message->toCString() );

    return buf;
}

// ** Error::message
String* Error::message( void ) const
{
    return m_message.get();
}

// ** Error::setMessage
void Error::setMessage( String* value )
{
    m_message = value;
}

// ** Error::name
String* Error::name( void ) const
{
    return m_name.get();
}

// ** Error::setName
void Error::setName( String* value )
{
    m_name = value;
}

// ** Error::errorId
int Error::errorId( void ) const
{
    return m_errorId;
}

// ** Error::setErrorId
void Error::setErrorId( int value )
{
    m_errorId = value;
}

// ** Error::stackTrace
String* Error::stackTrace( void ) const
{
    return m_stackTrace.get();
}

// ** Error::setStackTrace
void Error::setStackTrace( String* value )
{
    m_stackTrace = value;
}

// ** Error::create
Error* Error::create( player* player, const char* message, ... )
{
    const int kFormatBufferSize = 4096;

    char szBuf[kFormatBufferSize];

    va_list ap;
    va_start( ap, message );
    vsnprintf( szBuf, kFormatBufferSize, message, ap );
    va_end( ap );

    return new Error( player, new String( player, szBuf ) );
}

// --------------------------------------------------- ErrorClosure -------------------------------------------------- //

void ErrorClosure::init( Frame* frame )
{
    Error* e = cast_to<Error>( frame->instance() );
    assert(e);

    String* message = cast_to<String>( frame->arg(0).asObject() );
    int     errorId = frame->arg(1).asInt();

    e->setMessage( message );
    e->setErrorId( errorId );

    if( const Frame* parent = frame->parent() ) {
        e->setStackTrace( new String( parent->player(), Str( e->to_string() ) + "\n" + parent->captureStackTrace() ) );
    }
}

void ErrorClosure::getStackTrace( Frame *frame )
{
    Error* e = cast_to<Error>( frame->instance() );
    assert(e);

    String* stackTrace = e->stackTrace();
    frame->setResult( stackTrace );
}

void ErrorClosure::message( Frame* frame )
{
    
}

void ErrorClosure::setMessage( Frame* frame )
{
    
}

void ErrorClosure::name( Frame* frame )
{
    
}

void ErrorClosure::setName( Frame* frame )
{
    
}

void ErrorClosure::errorId( Frame* frame )
{
    
}

}