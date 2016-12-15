// gameswf_function.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// ActionScript function.

#include "Function.h"
#include "Avm.h"
#include "Error.h"
#include "Class.h"

namespace gameswf
{

// --------------------------------------------------- Function --------------------------------------------------- //

// ** Function::Function
Function::Function( player* player ) : Object( player )
{

}

// ** Function::to_string
const char* Function::to_string( void )
{
    static char str[64];
    sprintf( str, "[Function %p]", this );

    return str;
}

// ** Function::call
Value Function::call( const Value* args, int count ) const
{
    return call( Value::undefined, args, count );
}

// ** Function::call
Value Function::call( const Value& instance, const Value* args, int count ) const
{
    Value     result;
    Arguments argsuments( args, count );
    Frame     frame( "...", m_player.get(), NULL, instance.asObject(), &argsuments, &result );

    result = execute( &frame );

    if( frame.hasUnhandledException() ) {
        printf( "%s\n", frame.exception().asCString() );
    }

    return result;
}

// ** Function::executeWithInstance
Value Function::executeWithInstance( Object* instance, const Arguments& args, Frame* parentFrame )
{
    Value result;
    Frame frame( "...", m_player.get(), parentFrame, instance, &args, &result );
    execute( &frame );

    if( frame.hasUnhandledException() ) {
        if( parentFrame ) {
            parentFrame->throwException( frame.exception() );
        } else {
            printf( "%s\n", frame.exception().asCString() );
        }
    }

    return result;
}

// ----------------------------------------------- FunctionClosure ------------------------------------------------ //

// ** FunctionClosure::FunctionClosure
FunctionClosure::FunctionClosure( player* player, Object* instance, Function* function ) : Function( player ), m_instance( instance ), m_function( function )
{
    
}

// ** FunctionClosure::execute
Value FunctionClosure::execute( Frame* frame ) const
{
    assert( frame->instance() == 0 || frame->instance() == m_instance.get() );
    frame->setInstance( m_instance.get() );
    return m_function->execute( frame );
}

// ** FunctionClosure::instance
Object* FunctionClosure::instance( void ) const
{
    return m_instance.get();
}

// ** FunctionClosure::function
Function* FunctionClosure::function( void ) const
{
    return m_function.get();
}

// ------------------------------------------------ FunctionNative ------------------------------------------------ //

// ** FunctionNative::FunctionNative
FunctionNative::FunctionNative( player* player, FunctionNativeThunk thunk, void* userData )
    : Function( player ), m_thunk( thunk ), m_user_data( userData )
{

}

// ** FunctionNative::to_string
const char* FunctionNative::to_string( void )
{
    static char str[64];
    sprintf( str, "[FunctionNative %p]", this );
    
    return str;
}

// ** FunctionNative::thunk
FunctionNativeThunk FunctionNative::thunk( void ) const
{
    return m_thunk;
}

// ** FunctionNative::execute
Value FunctionNative::execute( Frame* frame ) const
{
    (*m_thunk)( frame );
    return frame->result();
}

// ------------------------------------------------ FunctionScript ------------------------------------------------ //

// ** FunctionScript::FunctionScript
FunctionScript::FunctionScript( player* player, Domain* domain, int maxStack, int maxScope )
    : Function( player ), m_domain( domain ), m_maxStack( maxStack ), m_maxScope( maxScope )
{

}

// ** FunctionScript::to_string
const char* FunctionScript::to_string( void )
{
    static char str[64];
    sprintf( str, "[FunctionScript %p]", this );

    return str;
}

// ** FunctionScript::super
Function* FunctionScript::super( void ) const
{
    return m_super.get_ptr();
}

// ** FunctionScript::setSuper
void FunctionScript::setSuper( Function* value )
{
    m_super = value;
}

// ** FunctionScript::accessScope
Class* FunctionScript::accessScope( void ) const
{
    return m_accessScope.get();
}

// ** FunctionScript::setAccessScope
void FunctionScript::setAccessScope( Class* value )
{
    assert( m_accessScope == NULL || m_accessScope == value );
    m_accessScope = value;
}

// ** FunctionScript::instructions
const Instructions& FunctionScript::instructions( void ) const
{
    return m_instructions;
}

// ** FunctionScript::setInstructions
void FunctionScript::setInstructions( const Instructions& value )
{
    m_instructions = value;
}

// ** FunctionScript::checkArguments
bool FunctionScript::checkArguments( Frame* frame ) const
{
    // ** Check argument count
    if( frame->nargs() != nargs() ) {
        Value error = Error::create( get_player(), "Argument count mismatch on '%s'. Expected %d, got %d.", frame->name(), m_argTypes.size(), frame->nargs() );
        frame->throwException( error );
        return false;
    }

    // ** Check argument types
    for( int i = 0, n = nargs(); i < n; i++ ) {
        Class*       type = m_argTypes[i].get();
        const Value& arg  = frame->arg( i );

        if( type == NULL || arg.is( type ) ) {
            continue;
        }

        Value error = Error::create( get_player(), "Argument %d type mismatch on '%s'. Expected %s, got %s.", i + 1, frame->name(), type->name().c_str(), arg.type() );
        frame->throwException( error );
        return false;
    }

    return true;
}

// ** FunctionScript::execute
Value FunctionScript::execute( Frame* frame ) const
{
    frame->setAccessScope( m_accessScope.get() );
    frame->setName( m_name.c_str() );
    frame->setDefaultArgs( nargs(), m_argDefaults );

    if( checkArguments( frame ) ) {
        Avm avm( m_player.get_ptr(), this, m_domain );
        avm.execute( this, frame );
    }

    return frame->result();
}

// ** FunctionScript::addException
void FunctionScript::addException( Exception* e )
{
    m_exceptions.push_back( e );
}

// ** FunctionScript::exceptions
const Exceptions& FunctionScript::exceptions( void ) const
{
    return m_exceptions;
}

// ** FunctionScript::setArguments
void FunctionScript::setArguments( const ClassesWeak& types, const ValueArray& defaults )
{
    m_argTypes    = types;
    m_argDefaults = defaults;
}

// ** FunctionScript::setName
void FunctionScript::setName( const Str& value )
{
    m_name = value;
}

// ** FunctionScript::nargs
int FunctionScript::nargs( void ) const
{
    return m_argTypes.size();
}

// ------------------------------------------------ FunctionScope ------------------------------------------------ //

// ** FunctionWithScope::FunctionWithScope
FunctionWithScope::FunctionWithScope( player* player, Function* function, const ScopeStack& scope ) : Function( player ), m_function( function ), m_scope( scope )
{

}

// ** FunctionWithScope::to_string
const char* FunctionWithScope::to_string( void )
{
    static char str[64];
    sprintf( str, "[FunctionWithScope %p]", this );
    
    return str;
}

// ** FunctionWithScope::call
Value FunctionWithScope::execute( Frame* frame ) const
{
    frame->setOuterScope( &m_scope );
    return m_function->execute( frame );
}

// ------------------------------------------------------- Frame ------------------------------------------------------- //

// ** Frame::Frame
Frame::Frame( const char* name, struct player* player, const Frame* parent, Object* instance, const Arguments* args, Value* result )
    : m_player( player ), m_name( name ), m_parent( parent ), m_arguments( args ), m_result( result ), m_instance( instance ), m_hasException( false ), m_accessScope( NULL )
{

}

// ** Frame::player
player* Frame::player( void ) const
{
    return m_player;
}

// ** Frame::parent
const Frame* Frame::parent( void ) const
{
    return m_parent;
}

// ** Frame::arg
const Value& Frame::arg( int index ) const
{
    return m_arguments ? m_arguments->arg( index ) : Value::undefined;
}

// ** Frame::args
const Arguments* Frame::args( void ) const
{
    return m_arguments;
}

// ** Frame::nargs
int Frame::nargs( void ) const
{
    return m_arguments ? m_arguments->count() : 0;
}

// ** Frame::argv
ValueArray Frame::argv( int startIndex ) const
{
    ValueArray result;

    for( int i = startIndex, n = nargs(); i < n; i++ ) {
        result.push_back( arg( i ) );
    }

    return result;
}

// ** Frame::result
const Value& Frame::result( void ) const
{
    return m_result ? *m_result : Value::undefined;
}

// ** Frame::setResult
void Frame::setResult( const Value& value )
{
    if( m_result ) *m_result = value;
}

// ** Frame::instance
Object* Frame::instance( void ) const
{
    return m_instance.get();
}

// ** Frame::accessScope
Class* Frame::accessScope( void ) const
{
    return m_accessScope;
}

// ** Frame::setAccessScope
void Frame::setAccessScope( Class* value )
{
    m_accessScope = value;
}

// ** Frame::setInstance
void Frame::setInstance( Object* value )
{
    m_instance = value;
}

// ** Frame::setOuterScope
void Frame::setOuterScope( const ScopeStack* value )
{
    m_scope.setOuter( value );
}

// ** Frame::name
const char* Frame::name( void ) const
{
    return m_name ? m_name : "...";
}

// ** Frame::setName
void Frame::setName( const char* value )
{
    m_name = value;
}

// ** Frame::setDefaultArgs
void Frame::setDefaultArgs( int maxArgs, const ValueArray& values )
{
    const_cast<Arguments*>( m_arguments )->setDefaults( maxArgs, values );
}

// ** Frame::throwException
void Frame::throwException( const Value& value )
{
    m_exception     = value;
    m_hasException  = true;
}

// ** Frame::handleException
void Frame::handleException( void )
{
    m_exception     = Value::undefined;
    m_hasException  = false;
}

// ** Frame::hasUnhandledException
bool Frame::hasUnhandledException( void ) const
{
    return m_hasException;
}

// ** Frame::exception
const Value& Frame::exception( void ) const
{
    return m_exception;
}

// ** Frame::fillLocalRegisters
void Frame::fillLocalRegisters( ValueArray& registers, int maxSize )
{
    registers.resize( maxSize );

    registers[0] = m_instance.get();

    if( !m_arguments ) {
        return;
    }

    for( int i = 0, n = m_arguments->count(); i < n; i++ ) {
        assert( (i + 1) < maxSize );
        registers[i + 1] = m_arguments->arg( i );
    }
}

// ** Frame::captureStackTrace
Str Frame::captureStackTrace( void ) const
{
    Str str = m_name ? Str( "\t at " ) + m_name : "";
    return m_parent ? str + "\n" + m_parent->captureStackTrace() : str;
}

// --------------------------------------------------- Arguments ---------------------------------------------------- //

// ** Arguments::Arguments
Arguments::Arguments( const Value* args, int count )
{
    for( int i = count - 1; i >= 0; i-- ) {
        m_args.push_back( args[i] );
    }
}

// ** Arguments::clear
void Arguments::clear( void )
{
    m_args.clear();
}

// ** Arguments::setDefaults
void Arguments::setDefaults( int maxArgs, const ValueArray& values )
{
    int nargs  = count();
    int offset = maxArgs - values.size();

    for( int i = nargs; i < maxArgs; i++ ) {
        const Value& value = values[i - offset];
        m_args.insert( m_args.begin(), value );
    }
}

// ** Arguments::push
void Arguments::push( const Value& value )
{
    m_args.push_back( value );
}

// ** Arguments::count
int Arguments::count( void ) const
{
    return ( int )m_args.size();
}

// ** Arguments::values
const ValueArray& Arguments::values( void ) const
{
    return m_args;
}

// ** Arguments::arg
const Value& Arguments::arg( int index ) const
{
    int nargs = count();

    if( index >= 0 && index < nargs ) {
        return m_args[nargs - index - 1];
    }

    return Value::undefined;
}

// ** Arguments::args
ValueArray Arguments::args( int startIndex ) const
{
    ValueArray result;

    for( int i = startIndex; i < count(); i++ ) {
        result.push_back( arg( i ) );
    }

    return result;
}

}
