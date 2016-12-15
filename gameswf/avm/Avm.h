
#ifndef __avm2__AVM__
#define __avm2__AVM__

#include "Stack.h"
#include "Instructions.h"
#include "Exception.h"

namespace gameswf
{
    // ** class Avm
    class Avm {
    public:

        // ** enum ErrorId
        enum ErrorId {
            InternalError,
            TypeError,
            ReferenceError,
            ArgumentError,
            VerifyError,

            TotalErrors
        };

    public:
        
                                Avm( player* player, const FunctionScript* function, const Domain* domain );
                                ~Avm( void );

        void                    execute( const FunctionScript* function, Frame* frame );

        static void             dumpStack( const char* id, const Stack& stack );
        static void             dumpScopeStack( const char* id, const ScopeStack& stack );

    private:

        TraitResolution         resolveProperty( Value& object, Value& value, const Multiname* identifier, Frame* frame, bool needsClosure = false ) const;
        void                    createClosure( Object* instance, Value* value ) const;
        bool                    setProperty( const Multiname* identifier, Frame* frame, const Value& value );
        Object*                 findProperty( const Multiname* identifier, Frame* frame, Value* value = NULL, bool needsClosure = false ) const;
        bool                    isType( const Value& value, const Class* type ) const;

        bool                    handleException( const Exceptions& exceptions, Frame* frame, int& index, const char* opCode ) const;
        void                    throwError( Frame* frame, ErrorId errorId, const char* message, ... ) const;

    private:

        player*                 m_player;
        const FunctionScript*   m_function;
        const Domain*           m_domain;
    };
}

#endif /* defined(__avm2__AVM__) */
