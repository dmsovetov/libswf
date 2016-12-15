//
//  Trait.cpp
//  GameSWF
//
//  Created by Советов Дмитрий on 08.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#include "Trait.h"

#include "Object.h"
#include "Multiname.h"
#include "Class.h"

namespace gameswf
{

// ---------------------------------------------- Traits ----------------------------------------------- //

// ** Traits::addTrait
void Traits::addTrait( Multiname* name, const Value& value, Type type, int slot, Uint8 attr )
{
    assert( name->kind() == Multiname::Qualified );
    assert( name->namespaces().size() == 1 );

    Namespace* ns     = name->namespaces()[0].get();
    Access     access = Public;

    switch( ns->kind() ) {
    case Namespace::PackageInternal:    access = PackagePrivate;    break;
    case Namespace::Private:            access = Private;           break;
    case Namespace::Protected:          access = Protected;         break;
    default:                            access = Public;            break;
    }

    if( type == Setter || type == Getter ) {
        addProperty( name, value, type, slot, attr, access );
        return;
    }

    Trait trait;
    trait.m_value  = value;
    trait.m_type   = type;
    trait.m_slot   = slot;
    trait.m_attr   = attr;
    trait.m_access = access;

    m_traits.set( name->name(), trait );
}

// ** Traits::addProperty
void Traits::addProperty( Multiname* name, const Value& value, Type type, int slot, Uint8 attr, Access access )
{
    Trait trait;

    if( !m_traits.get( name->name(), &trait ) ) {
        trait.m_value  = Value( Value::undefined, Value::undefined );
        trait.m_type   = Property;
        trait.m_slot   = slot;
        trait.m_attr   = attr;
        trait.m_access = access;
        m_traits.set( name->name(), trait );
    }

    switch( type ) {
    case Setter: trait.m_value.asProperty()->setSetter( value.asFunction() ); break;
    case Getter: trait.m_value.asProperty()->setGetter( value.asFunction() ); break;
    default:     assert( false );
    }
}

// ** Traits::setSlots
void Traits::setSlots( ValueArray& slots ) const
{
    assert( slots.size() > slotCount() );

    if( m_super != NULL ) {
        m_super->setSlots( slots );
    }

    for( TraitRegistry::const_iterator i = m_traits.begin(), end = m_traits.end(); i != end; ++i ) {
        const Trait& trait = i->second;
        assert( trait.m_slot > 0 );
        assert( slots[trait.m_slot].isUndefined() );
        slots[trait.m_slot] = i->second.m_value;
    }
}

// ** Traits::slotCount
int Traits::slotCount( void ) const
{
    return ( int )m_traits.size() + (m_super != NULL ? m_super->slotCount() : 0);
}

// ** Traits::setSuper
void Traits::setSuper( Traits* value )
{
    assert( m_super == NULL );
    m_super = value;

    if( m_super == NULL ) {
        return;
    }

    for( TraitRegistry::const_iterator i = m_traits.begin(), end = m_traits.end(); i != end; ++i ) {
        Trait        super;
        const Trait& trait = i->second;

        switch( trait.m_type ) {
        case Method:    {
                            FunctionScript* function = cast_to<FunctionScript>( trait.m_value.asFunction() );
                            if( function && m_super->findTrait( i->first, trait.m_type, super ) ) {
                                function->setSuper( super.m_value.asFunction() );
                            }
                        }
                        break;

        default:        break;
        }
    }
}

// ** Traits::setOwner
void Traits::setOwner( class Class* value )
{
    if( m_owner == value ) {
        return;
    }

    assert( m_owner == NULL );
    m_owner = value;

    for( TraitRegistry::iterator i = m_traits.begin(), end = m_traits.end(); i != end; ++i ) {
        switch( i->second.m_type ) {
        case Function:
        case Method:        if( FunctionScript* function = cast_to<FunctionScript>( i->second.m_value.asFunction() ) ) {
                                function->setAccessScope( value );
                            }
                            break;

        case Property:      if( class as_property* property = i->second.m_value.asProperty() ) {
                                if( FunctionScript* function = cast_to<FunctionScript>( property->m_setter.get() ) ) function->setAccessScope( value );
                                if( FunctionScript* function = cast_to<FunctionScript>( property->m_getter.get() ) ) function->setAccessScope( value );
                            }
                            break;

        default:            break;
        }
    }
}

// ** Traits::assignSlots
void Traits::assignSlots( void )
{
    int idx = m_super != NULL ? (m_super->slotCount() + 1) : 1;

    for( TraitRegistry::iterator i = m_traits.begin(), end = m_traits.end(); i != end; ++i ) {
        Trait& trait = i->second;

        if( trait.m_slot != 0 ) {
            assert( trait.m_slot >= idx );
            idx = trait.m_slot;
        }

        trait.m_slot = idx++;
    }
}

// ** Traits::resolveSlot
TraitResolution Traits::resolveSlot( const Str& name, TraitAccess access, int& slot, const class Class* accessScope ) const
{
    Trait trait;

    // ** Find trait by name
    if( m_traits.get( name, &trait ) == false ) {
        return m_super != NULL ? m_super->resolveSlot( name, access, slot, accessScope ) : TraitUnresolved;
    }

    // ** Trait found - check an access
    slot = trait.m_slot;

    if( trait.m_access == Public || m_owner.get() == accessScope ) {
        return TraitResolved;
    }

    if( trait.m_access == Protected && accessScope->is( m_owner.get() ) ) {
        return TraitResolved;
    }

    if( trait.m_access == PackagePrivate && accessScope && accessScope->package() == m_owner->package() ) {
        return TraitResolved;
    }

    return m_super != NULL ? m_super->resolveSlot( name, access, slot, accessScope ) : TraitAccessDenied;
}

// ** Traits::findTrait
bool Traits::findTrait( const Str& name, Type type, Trait& trait ) const
{
    if( m_traits.get( name, &trait ) && trait.m_type == type ) {
        return true;
    }

    return m_super != NULL ? m_super->findTrait( name, type, trait ) : false;
}

} // namespace gameswf