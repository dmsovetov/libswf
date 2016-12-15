//
//  Exception.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 18.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Exception.h"

#include "Class.h"

namespace gameswf
{

// ** Exception::Exception
Exception::Exception( const Str& name ) : m_type( NULL ), m_name( name ), m_from( -1 ), m_to( -1 ), m_target( -1 )
{
    m_isFinally = (name == "");
}

// ** Exception::type
Class* Exception::type( void ) const
{
    return m_type.get();
}

// ** Exception::setType
void Exception::setType( Class* value )
{
    m_type = value;
}

// ** Exception::from
int Exception::from( void ) const
{
    return m_from;
}

// ** Exception::setFrom
void Exception::setFrom( int value )
{
    m_from = value;
}

// ** Exception::to
int Exception::to( void ) const
{
    return m_to;
}

// ** Exception::setTo
void Exception::setTo( int value )
{
    m_to = value;
}

// ** Exception::target
int Exception::target( void ) const
{
    return m_target;
}

// ** Exception::setTarget
void Exception::setTarget( int value )
{
    m_target = value;
}

// ** Exception::isFinally
bool Exception::isFinally( void ) const
{
    return m_isFinally;
}

}