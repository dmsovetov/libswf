//
//  Error.h
//  GameSWF
//
//  Created by Советов Дмитрий on 18.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Error__
#define __GameSWF__Error__

#include "Object.h"

namespace gameswf {

    // ** class Error
    class Error : public Object {
    public:

                            AvmDeclareType( AS_ERROR, Object )

                            Error( player* player, String* message = NULL, int errorId = 0 );

        // ** Object
        virtual const char* to_string( void );

        // ** Error
        String*             message( void ) const;
        void                setMessage( String* value );
        String*             name( void ) const;
        void                setName( String* value );
        int                 errorId( void ) const;
        void                setErrorId( int value );
        String*             stackTrace( void ) const;
        void                setStackTrace( String* value );

        static Error*       create( player* player, const char* message, ... );

    private:

        StringPtr           m_message;
        StringPtr           m_name;
        StringPtr           m_stackTrace;
        int                 m_errorId;
    };

    AvmBeginClass( Error )
        AvmDeclareMethod( init )
        AvmDeclareMethod( getStackTrace )
        AvmDeclareProperty( message, setMessage )
        AvmDeclareProperty( name, setName )
        AvmDeclareReadonly( errorId )
    AvmEndClass

}

#endif /* defined(__GameSWF__Error__) */
