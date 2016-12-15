// gameswf_function.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript function.

#ifndef GAMESWF_FUNCTION_H
#define GAMESWF_FUNCTION_H

#include "Instructions.h"
#include "Stack.h"
#include "Object.h"
#include "Exception.h"

namespace gameswf {

    // ** class Arguments
    class Arguments {
    public:

                                    Arguments( const Value* args = NULL, int count = 0 );

        player*                     player( void ) const;
        void                        clear( void );
        void                        push( const Value& value );
        int                         count( void ) const;
        const ValueArray&           values( void ) const;
        ValueArray                  args( int startIndex ) const;
        const Value&                arg( int index ) const;
        void                        setDefaults( int maxArgs, const ValueArray& values );

    private:

        ValueArray                  m_args;
    };

    // ** class Function
	class Function : public Object {
    public:

                                    AvmDeclareType( AS_FUNCTION, Object );

                                    Function( player* player );

        // ** Object
		virtual const char*         to_string( void );

		// ** Function
        Value                       call( const Value* args = NULL, int count = 0 ) const;
        Value                       call( const Value& instance, const Value* args = NULL, int count = 0 ) const;
        Value                       executeWithInstance( Object* instance, const Arguments& args, Frame* parentFrame );
        virtual Value               execute( Frame* frame ) const = 0;
	};

    // ** class FunctionClosure
    class FunctionClosure : public Function {
    public:

                                    AvmDeclareType( AS_FUNCTION_CLOSURE, Function );

                                    FunctionClosure( player* player, Object* instance, Function* function );

        // ** FunctionClosure
        Object*                     instance( void ) const;
        Function*                   function( void ) const;

    private:

		// ** Function
        virtual Value               execute( Frame* frame ) const;

    private:

        ObjectPtr                   m_instance;
        FunctionPtr                 m_function;
    };

    // ** class FunctionNative
	class FunctionNative : public Function {
    public:

                                    AvmDeclareType( AS_C_FUNCTION, Function );

                                    FunctionNative( player* player, FunctionNativeThunk func, void* userData = NULL );

        // ** Object
        virtual const char*         to_string( void );

        // ** FunctionNative
        FunctionNativeThunk         thunk( void ) const;

    private:

        // ** Function
        virtual Value               execute( Frame* frame ) const;

    private:

		FunctionNativeThunk         m_thunk;
        void*                       m_user_data;
	};

    // ** class FunctionScript
    class FunctionScript : public Function {
    friend class Avm;
    public:

                                    AvmDeclareType( AS_3_FUNCTION, Function );

                                    FunctionScript( player* player, Domain* domain, int maxStack, int maxScope );

        // ** Object
        virtual const char*         to_string( void );

        // ** FunctionScript
        Function*                   super( void ) const;
        void                        setSuper( Function* value );
        Class*                      accessScope( void ) const;
        void                        setAccessScope( Class* value );
        const Instructions&         instructions( void ) const;
        void                        setInstructions( const Instructions& value );
        const Exceptions&           exceptions( void ) const;
        void                        addException( Exception* e );
        void                        setArguments( const ClassesWeak& types, const ValueArray& defaults );
        void                        setName( const Str& value );
        int                         nargs( void ) const;

    private:

        // ** Function
        virtual Value               execute( Frame* frame ) const;

        // ** FunctionScript
        bool                        checkArguments( Frame* frame ) const;

    private:

        Domain*                     m_domain;
        Instructions                m_instructions;
        FunctionWeak                m_super;
        Exceptions                  m_exceptions;
        int                         m_maxStack;
        int                         m_maxScope;
        ClassesWeak                 m_argTypes;
        ValueArray                  m_argDefaults;
        Str                         m_name;
        ClassWeak                   m_accessScope;
    };

    typedef gc_ptr<FunctionScript>      FunctionScriptPtr;
    typedef array<FunctionScriptPtr>    FunctionScripts;

    // ** class FunctionWithScope
    class FunctionWithScope : public Function {
    public:

                                    FunctionWithScope( player* player, Function* function, const ScopeStack& scope );

        // ** Object
        virtual const char*         to_string( void );

    private:

        // ** Function
        virtual Value               execute( Frame* frame ) const;

    private:

        FunctionPtr                 m_function;
        ScopeStack                  m_scope;
    };

    // ** class ActivationScope
    class ActivationScope : public Object {
    public:

                                    AvmDeclareType( AS_ACTIVATION_SCOPE, Object );

                                    ActivationScope( player* player, const Traits* traits ) : Object( player ) { setTraits( traits ); }

        // ** Object
        virtual const char*         to_string( void ) { return "[object ActivationScope]"; }
    };

    // ** class CatchScope
    class CatchScope : public ActivationScope {
    public:

                                    AvmDeclareType( AS_CATCH_SCOPE, ActivationScope );

                                    CatchScope( player* player, const Traits* traits ) : ActivationScope( player, traits ) {}

        // ** Object
        virtual const char*         to_string( void ) { return "[object CatchScope]"; }
    };

    // ** class Frame
    class Frame {
    friend class Avm;
    public:

                                    Frame( const char* name, player* player, const Frame* parent, Object* instance, const Arguments* args, Value* result );

        player*                     player( void ) const;
        const Frame*                parent( void ) const;
        const Value&                arg( int index ) const;
        int                         nargs( void ) const;
        ValueArray                  argv( int startIndex ) const;
        const Arguments*            args( void ) const;
        Object*                     instance( void ) const;
        Class*                      accessScope( void ) const;
        void                        setAccessScope( Class* value );
        const Value&                result( void ) const;
        void                        setResult( const Value& value );
        void                        setInstance( Object* value );
        void                        setOuterScope( const ScopeStack* value );
        const char*                 name( void ) const;
        void                        setName( const char* value );
        void                        setDefaultArgs( int maxArgs, const ValueArray& values );

        void                        throwException( const Value& error );
        bool                        hasUnhandledException( void ) const;
        void                        handleException( void );
        const Value&                exception( void ) const;
        Str                         captureStackTrace( void ) const;

    private:

        void                        fillLocalRegisters( ValueArray& registers, int maxSize );

    private:

        struct player*              m_player;
        const char*                 m_name;
        const Frame*                m_parent;
        const Arguments*            m_arguments;
        ObjectPtr                   m_instance;
        Value                       m_exception;
        bool                        m_hasException;
        Value*                      m_result;
        Stack                       m_stack;
        ScopeStack                  m_scope;
        Class*                      m_accessScope;
    };
}

#endif
