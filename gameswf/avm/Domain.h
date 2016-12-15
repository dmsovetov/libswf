//
//  TopLevel.h
//  GameSWF
//
//  Created by Советов Дмитрий on 07.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Domain__
#define __GameSWF__Domain__

#include "gameswf/gameswf_player.h"
#include "Function.h"
#include "Multiname.h"

namespace gameswf {

    // ** class Domain
    class Domain {
    public:

                            Domain( player* player );

        void                setMultinames( const Multinames& value );
        Multiname*          multiname( int index ) const;
        void                setStrings( const Strings& value );
        void                setFunctions( const FunctionScripts& value );

        virtual void        registerPackages( void );
        Class*              registerClass( Package* package, const Str& name, const Str& superClass, CreateInstanceThunk createInstance = NULL, FunctionNative* init = NULL ) const;
        void                registerPackage( Package* package );
        Package*            resolvePackage( const Str& name );
        Package*            findPackage( const Str& name ) const;
        Class*              findClass( const Str& package, const Str& name, bool initialize = true ) const;
        Class*              findClassQualified( const Str& name, bool initialize = true ) const;

    protected:

        weak_ptr<player>    m_player;
        PackageRegistry     m_packages;
        Multinames          m_multinames;
        Strings             m_strings;
        FunctionScripts     m_functions;
    };

    // ** class FlashDomain
    class FlashDomain : public Domain {
    public:

                            FlashDomain( player* player );

        // ** Domain
        virtual void        registerPackages( void );

    private:

        void                registerDisplay( void );
        void                registerEvents( void );
    };

} // namespace gameswf

#endif /* defined(__GameSWF__Domain__) */
