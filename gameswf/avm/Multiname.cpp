//
//  Multiname.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 09.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Multiname.h"

#include "Package.h"

namespace gameswf
{

// ------------------------------------------------ Namespace ------------------------------------------------ //

// ** Namespace::Namespace
Namespace::Namespace( Kind kind, const Str& name, class Package* package ) : m_kind( kind ), m_name( name ), m_package( package )
{

}

// ** Namespace::name
const Str& Namespace::name( void ) const
{
    return m_name;
}

// ** Namespace::kind
Namespace::Kind Namespace::kind( void ) const
{
    return m_kind;
}

// ** Namespace::package
Package* Namespace::package( void ) const
{
    return m_package.get();
}

// ------------------------------------------------ Multiname ------------------------------------------------ //

// ** Multiname::Multiname
Multiname::Multiname( Kind kind, const Str& name, const Namespaces& namespaces ) : m_kind( kind ), m_name( name ), m_namespaces( namespaces )
{

}

// ** Multiname::setName
void Multiname::setName( const Str& value )
{
    m_name = value;
}

// ** Multiname::name
const Str& Multiname::name( void ) const
{
    return m_name;
}

// ** Multiname::namespaces
const Namespaces& Multiname::namespaces( void ) const
{
    return m_namespaces;
}

// ** Multiname::kind
Multiname::Kind Multiname::kind( void ) const
{
    return m_kind;
}

// ** Multiname::hasRuntimeName
bool Multiname::hasRuntimeName( void ) const
{
    return (m_kind == RuntimeQualifiedLate) || (m_kind == MultipleNamespaceLate);
}

// ** Multiname::hasRuntimeNamespace
bool Multiname::hasRuntimeNamespace( void ) const
{
    return (m_kind == RuntimeQualifiedLate) || (m_kind == RuntimeQualified);
}

} // namespace gameswf