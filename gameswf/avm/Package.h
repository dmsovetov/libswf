//
//  Package.h
//  GameSWF
//
//  Created by Советов Дмитрий on 07.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Package__
#define __GameSWF__Package__

#include "Object.h"

namespace gameswf
{
    // ** class Package
    class Package : public Object {
    public:

                                    Package( Domain* domain, player* player, const Str& name );

        // ** Object
        virtual const char*         to_string( void );
        virtual bool                get_member( const Str& name, Value* val );

        // ** Package
        Class*                      findClass( const Str& name, bool initialize = true );
        void                        registerClass( Class* cls );
        void                        registerFunction( const Str& name, const Value& function );
        const Str&                  name( void ) const;
        Domain*                     domain( void ) const;

    private:

        Domain*                     m_domain;
        string_hash<gc_ptr<Class> > m_classes;
		string_hash<Value>          m_functions;
		Str                         m_name;
    };

} // namespace gameswf

#endif /* defined(__GameSWF__Package__) */
