//
//  Exception.h
//  GameSWF
//
//  Created by Советов Дмитрий on 18.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Exception__
#define __GameSWF__Exception__

#include "gameswf/gameswf.h"

namespace gameswf
{
    // ** class Exception
    class Exception : public ref_counted {
    public:

                                Exception( const Str& name );

        Class*                  type( void ) const;
        void                    setType( Class* value );
        int                     from( void ) const;
        void                    setFrom( int value );
        int                     to( void ) const;
        void                    setTo( int value );
        int                     target( void ) const;
        void                    setTarget( int value );
        bool                    isFinally( void ) const;

    private:

        ClassWeak               m_type;
        Str                     m_name;
        int                     m_from;
        int                     m_to;
        int                     m_target;
        bool                    m_isFinally;
    };

    typedef gc_ptr<Exception>   ExceptionPtr;
    typedef array<ExceptionPtr> Exceptions;
}

#endif /* defined(__GameSWF__Exception__) */
