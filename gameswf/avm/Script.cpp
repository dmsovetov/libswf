//
//  Script.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 08.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Script.h"

namespace gameswf
{

// ** Script::Script
Script::Script( Function* init, Traits* traits, Package* package ) : m_package( package ), m_init( init ), m_traits( traits )
{

}

// ** Script::package
Package* Script::package( void ) const
{
    return m_package;
}

// ** Script::init
Function* Script::init( void ) const
{
    return m_init;
}

// ** Script::traits
Traits* Script::traits( void ) const
{
    return m_traits.get();
}

} // namespace gameswf