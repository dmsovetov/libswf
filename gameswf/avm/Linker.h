//
//  Linker.h
//  GameSWF
//
//  Created by Советов Дмитрий on 09.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Linker__
#define __GameSWF__Linker__

#include "Abc.h"

#include "Multiname.h"
#include "Function.h"
#include "Class.h"

namespace gameswf {

    // ** class Linker
    class Linker {
    public:

                                Linker( Domain* domain, const AbcInfo* abc, player* player );

        bool                    link( void );

    private:

        void                    linkStrings( void );
        void                    linkNamespaces( void );
        Multiname*              linkMultiname( const MultinameInfo* multinameInfo );
        Script*                 linkScript( const ScriptInfo* scriptInfo );
        Class*                  linkClass( const InstanceInfo* instanceInfo, const ClassInfo* classInfo );
        bool                    linkClassTraits( Class* cls, int index );
        Traits*                 linkTraits( const TraitsArray& traitsInfo );
        FunctionScript*         linkFunction( const MethodInfo* methodInfo );
        Exception*              linkException( const ExceptInfo* exceptionInfo );
        Instructions            linkInstructions( const FunctionScript* function, const BodyInfo* bodyInfo );
        Value                   linkConstant( ConstantKind kind, int index ) const;

        Class*                  resolveClass( int index ) const;
        void                    resolveMultiName( int index, Str& package, Str& name ) const;
        const Str&              resolveNamespace( int index ) const;
        const Str&              resolveMultiNamespace( int index ) const;

        static Multiname::Kind  multinameKind( MultinameInfo::Kind kind );
        static Namespace::Kind  namespaceKind( NamespaceInfo::Kind kind );

    public:

        weak_ptr<player>        m_player;
        const AbcInfo*          m_abc;
        Domain*                 m_domain;

        Classes                 m_classes;
        FunctionScripts         m_functions;
        Multinames              m_multinames;
        Strings                 m_strings;
        Namespaces              m_namespaces;
    };

} // namespace gameswf

#endif /* defined(__GameSWF__Linker__) */
