//
//  Multiname.h
//  GameSWF
//
//  Created by Советов Дмитрий on 09.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Multiname__
#define __GameSWF__Multiname__

#include "gameswf/gameswf.h"

namespace gameswf {

    // ** class Namespace
    class Namespace : public ref_counted {
    public:

        // enum Kind
        enum Kind {
            Ns,
			Package,
			PackageInternal,
            Private,
			Protected,
            StaticProtected,
			Explicit
        };

                        Namespace( Kind kind, const Str& name, class Package* package );

        const Str&      name( void ) const;
        Kind            kind( void ) const;
        class Package*  package( void ) const;

    private:

        Str             m_name;
        Kind            m_kind;
        PackageWeak     m_package;
    };

    // ** class Multiname
    class Multiname : public ref_counted {
    public:

        // ** enum Kind
        enum Kind {
            Unknown,
            Qualified,
            RuntimeQualified,
            RuntimeQualifiedLate,
            MultipleNamespace,
            MultipleNamespaceLate
        };

    public:

                            Multiname( Kind kind, const Str& name, const Namespaces& namespaces );

        const Str&          name( void ) const;
        void                setName( const Str& value );
        const Namespaces&   namespaces( void ) const;
        Kind                kind( void ) const;
        bool                hasRuntimeNamespace( void ) const;
        bool                hasRuntimeName( void ) const;

    private:

        Kind                m_kind;
        Str                 m_name;
        Namespaces          m_namespaces;
    };

} // namespace gameswf

#endif /* defined(__GameSWF__Multiname__) */
