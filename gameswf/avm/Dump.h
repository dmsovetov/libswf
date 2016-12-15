
#ifndef __avm2__ABCDump__
#define __avm2__ABCDump__

#include "gameswf/gameswf.h"

namespace gameswf {

    // ** class Dump
    class Dump {
    public:
        
                                Dump( void );
        
        void                    dumpABC( const AbcInfo* abc );
        
        static Str              formatMethod( const AbcInfo* abc, int index );
        static Str              formatNamespace( const AbcInfo* abc, int index, bool nameOnly = false );
        static Str              formatMultiname( const AbcInfo* abc, int index, bool nameOnly = false );
        static Str              formatNSSet( const AbcInfo* abc, int index );
        static Str              formatClass( const AbcInfo* abc, int index );
        
        static const char*      formatOpCode( int opcode );
        static const char*      formatMultinameKind( int kind );
        static const char*      formatNamespaceKind( int kind );
        static const char*      formatString( const AbcInfo* abc, int str );
        
    private:
        
        const AbcInfo           *m_abc;
    };

} // namespace gameswf

#endif /* defined(__avm2__ABCDump__) */
