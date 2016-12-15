//
//  Traits.h
//  GameSWF
//
//  Created by Советов Дмитрий on 08.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Trait__
#define __GameSWF__Trait__

#include "gameswf/gameswf.h"
#include "Value.h"

namespace gameswf {

    // ** class Traits
    class Traits : public ref_counted {
    public:

        // ** enum Type
        enum Type {
            Slot      = 0,
            Method    = 1,
            Property  = 2,
            Getter    = 3,
            Setter    = 4,
            Class     = 5,
            Function  = 6,
            Const     = 7
        };

        // ** enum Attribute
        enum Attribute {
            Final       = 0x1,
            Override    = 0x2,
            Metadata    = 0x4,
        };

        // ** enum Access
        enum Access {
            Private,
            PackagePrivate,
            Protected,
            Public,
        };

        // ** struct Trait
        struct Trait {
            Value       m_value;
            Type        m_type;
            int         m_slot;
            Uint8       m_attr;
            Access      m_access;
        };

    public:

        void            addTrait( Multiname* name, const Value& value, Type type, int slot, Uint8 attr );
        void            assignSlots( void );
        void            setOwner( class Class* value );
        void            setSuper( Traits* value );
        void            setSlots( ValueArray& slots ) const;
        TraitResolution resolveSlot( const Str& name, TraitAccess access, int& slot, const class Class* accessScope ) const;
        int             slotCount( void ) const;

    private:

        void            addProperty( Multiname* name, const Value& value, Type type, int slot, Uint8 attr, Access access );
        bool            findTrait( const Str& name, Type type, Trait& trait ) const;

    private:

        typedef string_hash<Trait>  TraitRegistry;
        ClassWeak       m_owner;
        TraitRegistry   m_traits;
        TraitsWeak      m_super;
    };

} // namespace gameswf

#endif /* defined(__GameSWF__Trait__) */
