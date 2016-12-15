
#include "Avm.h"

#include "Object.h"
#include "Domain.h"
#include "Dump.h"
#include "Class.h"
#include "Multiname.h"
#include "Instructions.h"
#include "Function.h"
#include "Array.h"
#include "Package.h"
#include "Error.h"

#define AvmHandleException( frame ) if( (frame)->hasUnhandledException() ) {                            \
                                        if( !handleException( exceptions, frame, op, opCode ) ) {       \
                                            return;                                                     \
                                        }                                                               \
                                        continue;                                                       \
                                    }

#define AvmTypeError( ... )         throwError( frame, TypeError, __VA_ARGS__ );                        \
                                    AvmHandleException( frame )

#define AvmReferenceError( ... )    throwError( frame, ReferenceError, __VA_ARGS__ );                   \
                                    AvmHandleException( frame )

namespace gameswf
{

// ** Avm::Avm
Avm::Avm( player* player, const FunctionScript* function, const Domain* domain ) : m_player( player ), m_function( function ), m_domain( domain )
{
}

Avm::~Avm( void )
{
}

// ** Avm::execute
void Avm::execute( const FunctionScript* function, Frame* frame )
{
    Value*      result     = frame->m_result;
    Stack&      stack      = frame->m_stack;
    ScopeStack& scopeStack = frame->m_scope;

    ValueArray registers;
    frame->fillLocalRegisters( registers, 16 );
    
    AVM2_VERBOSE( "Avm::execute : instance=%s\n", registers[0].asCString() );

    const Instructions& code        = function->m_instructions;
    Exceptions          exceptions  = function->exceptions();

    Class*              accessScope = frame->accessScope();
    Arguments           args;
    Value               object;
    Value               value;
    const char*         debugFile  = "";
    int                 debugLine  = 0;

    for( int op = 0, n = ( int )code.size(); op < n; op++ ) {
        const Instruction&  i      = code[op];
        const char*         opCode = Dump::formatOpCode( i.opCode );

        switch( i.opCode ) {
            // --------------------------------------------------- Locals ----------------------------------------------- //

            case GetLocal0:             AVM2_VERBOSE( "%s : local[0] = %s\n", opCode, registers[0].asCString() );
                                        stack.push( registers[0], opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case GetLocal1:             AVM2_VERBOSE( "%s : local[1] = %s\n", opCode, registers[1].asCString() );
                                        stack.push( registers[1], opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case GetLocal2:             AVM2_VERBOSE( "%s : local[2] = %s\n", opCode, registers[2].asCString() );
                                        stack.push( registers[2], opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case GetLocal3:             AVM2_VERBOSE( "%s : local[3] = %s\n", opCode, registers[3].asCString() );
                                        stack.push( registers[3], opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case SetLocal:              AVM2_VERBOSE( "%s : local[%d] = %s\n", opCode, i.Integer, stack.top().asCString() );
                                        registers[i.Integer] = stack.pop();
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case GetLocal:              AVM2_VERBOSE( "%s : local[%d] = %s\n", opCode, i.Integer, registers[i.Integer].asCString() );
                                        stack.push( registers[i.Integer] );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case SetLocal1:             AVM2_VERBOSE( "%s : local[1] = %s\n", opCode, stack.top().asCString() );
                                        registers[1] = stack.pop();
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case SetLocal2:             AVM2_VERBOSE( "%s : local[2] = %s\n", opCode, stack.top().asCString() );
                                        registers[2] = stack.pop();
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case SetLocal3:             AVM2_VERBOSE( "%s : local[3] = %s\n", opCode, stack.top().asCString() );
                                        registers[3] = stack.pop();
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case Kill:                  AVM2_VERBOSE( "%s : %d\n", opCode, i.Integer );
                                        registers[i.Integer] = Value::undefined;
                                        break;

            // -------------------------------------------------- Scope ------------------------------------------------- //

            case PushScope:             AVM2_VERBOSE( "%s : %s\n", opCode, stack.top().asCString() );
                                        scopeStack.push( stack.pop().asObject(), opCode );
                                        AVM2_DEBUG_ONLY( dumpScopeStack( "scope", scopeStack ) );
                                        break;

            case PopScope:              AVM2_VERBOSE( "%s\n", opCode );
                                        scopeStack.pop();
                                        AVM2_DEBUG_ONLY( dumpScopeStack( "scope", scopeStack ) );
                                        break;

            case GetScopeObject:        AVM2_VERBOSE( "%s : %d (pushed %s)\n", opCode, i.Integer, scopeStack.at( i.Integer )->to_string() );
                                        stack.push( scopeStack.at( i.Integer ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case GetGlobalScope:        AVM2_VERBOSE( "%s : %s\n", opCode, scopeStack.at( 0 )->to_string() );
                                        stack.push( scopeStack.at( 0 ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            // ---------------------------------------------- Property access -------------------------------------------- //
                
            case FindProperty:          AVM2_VERBOSE( "%s : '%s' at scope stack - ", opCode, i.Identifier->name().c_str() );
                                        if( Object* object = findProperty( i.Identifier, frame ) ) {
                                            AVM2_VERBOSE( "found at %s\n", object->to_string() );
                                            stack.push( Value( object ), opCode );
                                        } else {
                                            AVM2_VERBOSE( "global object\n" );
                                            stack.push( scopeStack.at( 0 ), opCode );
                                        }
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
            
            case FindPropertyStrict:    AVM2_VERBOSE( "%s : '%s' at scope stack - ", opCode, i.Identifier->name().c_str() );
                                        if( Object* object = findProperty( i.Identifier, frame ) ) {
                                            AVM2_VERBOSE( "found at %s\n", object->to_string() );
                                            stack.push( object, opCode );
                                        } else {
                                            AvmReferenceError( "The property '%s' could not be resolved.", i.Identifier->name().c_str() );
                                        }
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case GetLex:                {
                                            AVM2_VERBOSE( "%s : '%s'\n", opCode, i.Identifier->name().c_str() );
                                            Value value;
                                            if( findProperty( i.Identifier, frame, &value, true ) ) {
                                                stack.push( value, opCode );
                                            } else {
                                                AvmReferenceError( "The property '%s' could not be resolved.", i.Identifier->name().c_str() );
                                            }
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;
                
            case GetProperty:           {
                                            AVM2_VERBOSE( "%s : '%s' at %s\n", opCode, i.Identifier->name().c_str(), stack.top().asCString() );
                                            TraitResolution resolution = resolveProperty( object, value, i.Identifier, frame, true );

                                            if( object.isNull() ) {
                                                AvmTypeError( "Cannot access a property or method of a null object reference." );
                                            }

                                            if( object.isUndefined() ) {
                                                AvmTypeError( "A term is undefined and has no properties." );
                                            }

                                            if( resolution != TraitResolved ) {
                                                AvmReferenceError( "Property %s not found on %s and there is no default value.", i.Identifier->name().c_str(), object.type() );
                                            }

                                            stack.push( value, opCode );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case SetProperty:
            case InitProperty:          {
                                            AVM2_VERBOSE( "%s : %s.%s = %s\n", opCode, stack.top(1).asCString(), i.Identifier->name().c_str(), stack.top().asCString() );
                                            if( !setProperty( i.Identifier, frame, stack.pop() ) ) {
                                                AvmReferenceError( "The property '%s' could not be set.", i.Identifier->name().c_str() );
                                            }
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case SetSlot:               {
                                            AVM2_VERBOSE( "%s : %s[%d] = %s\n", opCode, stack.top( 1 ).asCString(), i.Integer, stack.top( 0 ).asCString() );
                                            Value   value  = stack.pop();
                                            Object* object = stack.pop().asObject();

                                            object->setSlot( i.Integer, value );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case GetSlot:               AVM2_VERBOSE( "%s : %s[%d] = %s\n", opCode, stack.top().asCString(), i.Integer, stack.top().asObject()->slot( i.Integer ).asCString() );
                                        stack.push( stack.pop().asObject()->slot( i.Integer ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            // ---------------------------------------------- Function invokation -------------------------------------------- //

            case Call:                  {
                                            AVM2_VERBOSE( "%s : ", opCode );
                                            stack.arguments( args, i.ArgCount );

                                            Value receiver = stack.pop();
                                            Value value    = stack.pop();

                                            if( !value.isFunction() ) {
                                                AvmTypeError( "Value is not a function." );
                                            }

                                            Value result = value.asFunction()->executeWithInstance( receiver.asObject(), args, frame );
                                            AvmHandleException( frame );

                                            stack.push( result );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case CallSuperVoid:         {
                                            AVM2_VERBOSE( "%s : ", opCode );
                                            stack.arguments( args, i.ArgCount );

                                            assert( !i.Identifier->hasRuntimeName() );
                                            assert( !i.Identifier->hasRuntimeNamespace() );
                                            assert( m_function->m_super != NULL );

                                            object = stack.pop();
                                            if( object.isNullOrUndefined() ) {
                                                AvmTypeError( "Failed to call property '%s', a term is undefined and has no properties.\n", i.Identifier->name().c_str() );
                                            }

                                            m_function->m_super->executeWithInstance( object.asObject(), args, frame );
                                            AvmHandleException( frame );

                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case CallPropVoid:          {
                                            AVM2_VERBOSE( "%s : %s ", opCode, i.Identifier->name().c_str() );

                                            // ** Pop arguments
                                            stack.arguments( args, i.ArgCount );

                                            TraitResolution resolution = resolveProperty( object, value, i.Identifier, frame );

                                            if( object.isNullOrUndefined() ) {
                                                AvmTypeError( "Failed to call property '%s', a term is undefined and has no properties.\n", i.Identifier->name().c_str() );
                                            }

                                            if( resolution != TraitResolved ) {
                                                AvmTypeError( "Property %s not found on %s and there is no default value.\n", i.Identifier->name().c_str(), object.type() );
                                            }

                                            if( !value.isFunction() ) {
                                                AvmTypeError( "%s, value is not a function.", i.Identifier->name().c_str() );
                                            }

                                            value.asFunction()->executeWithInstance( object.asObject(), args, frame );
                                            AvmHandleException( frame );

                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case CallProperty:          {
                                            AVM2_VERBOSE( "%s : %s ", opCode, i.Identifier->name().c_str() );

                                            // ** Pop arguments
                                            stack.arguments( args, i.ArgCount );

                                            // ** Pop object
                                            TraitResolution resolution = resolveProperty( object, value, i.Identifier, frame );

                                            if( object.isNullOrUndefined() ) {
                                                AvmTypeError( "%s, cannot access a property or method of a null object reference.", i.Identifier->name().c_str() );
                                            }

                                            if( resolution != TraitResolved ) {
                                                AvmReferenceError( "Property %s not found on %s and there is no default value.\n", i.Identifier->name().c_str(), object.type() );
                                            }

                                            if( !value.isFunction() ) {
                                                AvmTypeError( "%s, value is not a function.", i.Identifier->name().c_str() );
                                            }

                                            // ** Call property
                                            Value result = value.asFunction()->executeWithInstance( object.asObject(), args, frame );
                                            AvmHandleException( frame );

                                            stack.push( result, opCode );

                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case ReturnVoid:            if( result ) {
                                            *result = Value::Undefined;
                                        }
                                        return;

            case ReturnValue:           if( result ) {
                                            *result = stack.pop();
                                        }
                                        return;

            // ---------------------------------------------- Instance construction -------------------------------------------- //

            case NewObject:             {
                                            AVM2_VERBOSE( "%s : (", opCode );

                                            Object* instance = new Object( m_player );
                                            instance->m_members.set_capacity( i.ArgCount );

                                            for( int j = 0; j < i.ArgCount; j++ ) {
                                                Value value = stack.pop();
                                                Str   name  = stack.pop().asCString();
                                                AVM2_VERBOSE( "%s:%s ", name.c_str(), value.asCString() );
                                                instance->set_member( name, value );
                                            }
                                            AVM2_VERBOSE( ")\n" );

                                            stack.push( instance, opCode );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case NewArray:              {
                                            AVM2_VERBOSE( "%s : ", opCode );

                                            stack.arguments( args, i.ArgCount );

                                            Array* instance = new Array( m_player );
                                            for( int i = 0, n = args.count(); i < n; i++ ) {
                                                instance->push( args.arg( i ) );
                                            }

                                            stack.push( instance, opCode );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case NewClass:              {
                                            AVM2_VERBOSE( "%s : '%s'\n", opCode, i.Class->to_string() );
                                            Class* super = cast_to<Class>( stack.top(0).asObject() );
                                            UNUSED(super);
                                            stack.pop();

                                            Class* cls = i.Class;
                                            cls->initialize();
                                            stack.push( cls, opCode );

                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case NewActivation:         AVM2_VERBOSE( "%s\n", opCode );
                                        const_cast<Traits*>( function->traits() )->setOwner( frame->accessScope() );
                                        stack.push( new ActivationScope( m_player, function->traits() ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case NewCatch:              AVM2_VERBOSE( "%s\n", opCode );
                                        stack.push( new CatchScope( m_player, function->traits() ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case NewFunction:           {
                                            AVM2_VERBOSE( "%s\n", opCode );
                                            AVM2_DEBUG_ONLY( dumpScopeStack( "scope", scopeStack ) );
                                            FunctionScript* function = cast_to<FunctionScript>( const_cast<Function*>( i.Function ) );
                                            function->setAccessScope( frame->accessScope() );
                                            stack.push( ( Object* )new FunctionWithScope( m_player, function, scopeStack ), opCode );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case Construct:             {
                                            AVM2_VERBOSE( "%s : \n", opCode );
                                            stack.arguments( args, i.ArgCount );
                                            Value value = stack.pop();

                                            if( !value.isFunction() ) {
                                                AvmTypeError( "Instantiation attempted on a non-constructor." );
                                            }

                                            Value result = value.asFunction()->executeWithInstance( NULL, args, frame );
                                            AvmHandleException( frame );

                                            stack.push( result );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;
                
            case ConstructProp:         {
                                            AVM2_VERBOSE( "%s : %s at %s ", opCode, i.Identifier->name().c_str(), stack.top( i.ArgCount ).asCString() );

                                            // ** Get the arguments
                                            stack.arguments( args, i.ArgCount );

                                            // ** Resolve property
                                            TraitResolution resolution = resolveProperty( object, value, i.Identifier, frame );

                                            if( object.isNullOrUndefined() ) {
                                                AvmTypeError( "%s, cannot access a property or method of a null object reference.", i.Identifier->name().c_str() );
                                            }

                                            if( value.isNullOrUndefined() ) {
                                                AvmTypeError( "%s, instantiation attempted on a non-constructor.", i.Identifier->name().c_str() );
                                            }

                                            if( resolution != TraitResolved ) {
                                                AvmReferenceError( "Property %s not found on %s and could not be called.\n", i.Identifier->name().c_str(), object.type() );
                                            }

                                            // ** Construct
                                            if( i.Identifier->name() == "Number" ) {
                                                stack.push( args.count() ? args.values()[0].asNumber() : 0 );
                                            } else {
                                                Class* cls      = cast_to<Class>( value.asObject() );
                                                Value  instance = cls->createInstance();

                                                cls->construct( instance, args, frame );

                                                stack.push( instance, opCode );
                                            }

                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;
                
            case ConstructSuper:        {
                                            AVM2_VERBOSE( "%s : %s (args %d)\n", opCode, stack.top( i.ArgCount ).asCString(), i.ArgCount );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                
                                            const Value& obj = stack.top( i.ArgCount );
                                            if( obj.isNullOrUndefined() ) {
                                                AvmTypeError( "cannot access a property or method of a null object reference." );
                                            }

                                            stack.arguments( args, i.ArgCount );

                                            if( Class* superClass = cast_to<Class>( m_function->m_super.get() ) ) {
                                                superClass->construct( obj, args, frame );
                                                AvmHandleException( frame );
                                            }
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            // ----------------------------------------------- Type coercion --------------------------------------------- //
                
            case Coerce:                AVM2_VERBOSE( "%s : to %s\n", opCode, i.Identifier->name().c_str() );
                                        break;
                
            case CoerceToAny:           AVM2_VERBOSE( "%s : to *\n", opCode );
                                        break;

            case ConvertToInt:          AVM2_VERBOSE( "%s : %s = %d\n", opCode, stack.top().asCString(), stack.top().asInt() );
                                        stack.push( Value( (double)stack.pop().asInt() ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case ConvertToBool:         AVM2_VERBOSE( "%s : %s = %d\n", opCode, stack.top().asCString(), stack.top().asBool() );
                                        stack.push( Value( stack.pop().asBool() ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case ConvertToString:       AVM2_VERBOSE( "%s : %s = %s\n", opCode, stack.top().asCString(), stack.top().asCString() );
                                        stack.push( Value( stack.pop().asCString() ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case ConvertToDouble:       AVM2_VERBOSE( "%s : %s = %f\n", opCode, stack.top().asCString(), stack.top().asNumber() );
                                        stack.push( Value( stack.pop().asNumber() ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case IsTypeLate:            {
                                            AVM2_VERBOSE( "%s : %s is %s\n", opCode, stack.top(1).asCString(), stack.top().asCString() );
                                            Class* type  = cast_to<Class>( stack.pop().asObject() );
                                            Value  value = stack.pop();
                                            stack.push( isType( value, type ) );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            // -------------------------------------------- Operand stack ------------------------------------------- //

            case PushNull:              AVM2_VERBOSE( "%s\n", opCode );
                                        stack.push( Value::null );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case PushByte:              AVM2_VERBOSE( "%s : %d\n", opCode, i.Integer );
                                        stack.push( Value( i.Integer ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case PushInt:               AVM2_VERBOSE( "%s : %d\n", opCode, i.Integer );
                                        stack.push( Value( i.Integer ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case PushDouble:            AVM2_VERBOSE( "%s : %f\n", opCode, i.Number );
                                        stack.push( Value( i.Number ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case PushShort:             AVM2_VERBOSE( "%s : %d\n", opCode, i.Integer );
                                        stack.push( Value( i.Integer ), opCode );
                                        break;
                
            case PushString:            AVM2_VERBOSE( "%s : %s\n", opCode, i.Str->toCString() );
                                        stack.push( Value( ( Object* )i.Str ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case PushTrue:              AVM2_VERBOSE( "%s\n", opCode );
                                        stack.push( Value( true ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case PushFalse:             AVM2_VERBOSE( "%s\n", opCode );
                                        stack.push( Value( false ), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case PushUndefined:         AVM2_VERBOSE( "%s\n",opCode );
                                        stack.push( Value::undefined );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;
                
            case Pop:                   AVM2_VERBOSE( "%s\n", opCode );
                                        stack.pop();
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case Dup:                   AVM2_VERBOSE( "%s : %s\n", opCode, stack.top().asCString() );
                                        stack.push( stack.top(), opCode );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case Swap:                  AVM2_VERBOSE( "%s\n", opCode );
                                        stack.swap();
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            // --------------------------------------------- Arithmetic -------------------------------------------- //
                
            case Add:                   {
                                            AVM2_VERBOSE( "%s : %s + %s\n", opCode, stack.top( 1 ).asCString(), stack.top( 0 ).asCString() );
                                            Value b = stack.pop();
                                            Value a = stack.pop();
                
                                            if( a.isStringObject() && b.isStringObject() ) {
                                                stack.push( Value( new String( m_player, a.asString() + b.asString() ) ), opCode );
                                            }
                                            else if( a.isStringObject() || b.isStringObject() ) {
                                                stack.push( Value( new String( m_player, a.asString() + b.asString() ) ), opCode );
                                            } else {
                                                stack.push( Value( a.asNumber() + b.asNumber() ), opCode );
                                            }
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case Multiply:              {
                                            AVM2_VERBOSE( "%s : %s * %s\n", opCode, stack.top( 1 ).asCString(), stack.top( 0 ).asCString() );
                                            double b = stack.pop().asNumber();
                                            double a = stack.pop().asNumber();
                                            stack.push( a * b );
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case IncrementI:            AVM2_VERBOSE( "%s : %s++\n", opCode, stack.top().asCString() );
                                        stack.push( Value( double( stack.pop().asInt() + 1 ) ) );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case Increment:             AVM2_VERBOSE( "%s : %s++\n", opCode, stack.top().asCString() );
                                        stack.push( Value( double( stack.pop().asNumber() + 1 ) ) );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case Equals:                AVM2_VERBOSE( "%s : %s == %s\n", opCode, stack.top(0).asCString(), stack.top(1).asCString() );
                                        stack.push( stack.pop() == stack.pop() );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            case BitOr:                 AVM2_VERBOSE( "%s : %d | %d\n", opCode, stack.top(0).asInt(), stack.top(1).asInt() );
                                        stack.push( stack.pop().asInt() | stack.pop().asInt() );
                                        AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        break;

            // ----------------------------------------------- Object iteration ----------------------------------------------- //

            case HasNext2:              {
                                            AVM2_VERBOSE( "%s : %s %s\n", opCode, registers[i.objectReg].asCString(), registers[i.indexReg].asCString() );

                                            Object* object = registers[i.objectReg].asObject();
                                            Value&  index  = registers[i.indexReg];
                                            bool    first  = index.isNumber();

                                            string_hash<Value>::iterator it = first ? object->m_members.begin() : object->m_members.find( index.asCString() );
                                            if( it != object->m_members.end() ) {
                                                if( !first ) {
                                                    ++it;
                                                }

                                                bool hasNext = it != object->m_members.end();

                                                index.setString( hasNext ? it->first : "" );
                                                stack.push( hasNext );
                                            } else {
                                                stack.push( false );
                                            }
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case NextValue:             {
                                            AVM2_VERBOSE( "%s : %s %s\n", opCode, stack.top( 0 ).asCString(), stack.top( 1 ).asCString() );
                                            Value   index  = stack.pop();
                                            Object* object = stack.pop().asObject();

                                            if( object ) {
                                                string_hash<Value>::iterator it = object->m_members.find( index.asString() );
                                                stack.push( it != object->m_members.end() ? it->second : Value() );
                                            } else {
                                                stack.push( Value() );
                                            }
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            case NextName:              {
                                            AVM2_VERBOSE( "%s : %s %s\n", opCode, stack.top( 0 ).asCString(), stack.top( 1 ).asCString() );

                                            Value   index  = stack.pop();
                                            Object* object = stack.pop().asObject();

                                            if( object ) {
                                                string_hash<Value>::iterator it = object->m_members.find( index.asString() );
                                                stack.push( Value( it != object->m_members.end() ? it->first : "" ) );
                                            } else {
                                                stack.push( Value() );
                                            }
                                            AVM2_DEBUG_ONLY( dumpStack( "operand", stack ) );
                                        }
                                        break;

            // ---------------------------------------------- Branching ----------------------------------------------- //

            case Label: break;

                
            case Jump:                  AVM2_VERBOSE( "%s : %d\n", opCode, i.offset );
                                        op = i.offset;
                                        break;

            case IfTrue:                {
                                            AVM2_VERBOSE( "%s : %s\n", opCode, stack.top( 0 ).asCString() );
                                            if( stack.pop().asBool() ) {
                                                op = i.offset;
                                            }
                                        }
                                        break;

            case IfLess:                {
                                            AVM2_VERBOSE( "%s : %s < %s\n", opCode, stack.top( 0 ).asCString(), stack.top( 1 ).asCString() );
                                            bool le = stack.pop().asNumber() <= stack.pop().asNumber();
                                            if( le == false ) {
                                                op = i.offset;
                                            }
                                        }
                                        break;
                
            case IfFalse:               {
                                            AVM2_VERBOSE( "%s : %s\n", opCode, stack.top( 0 ).asCString() );
                                            if( stack.pop().asBool() == false ) {
                                                op = i.offset;
                                            }
                                        }
                                        break;

            case IfStictNotEqual:
            case IfNotEqual:            {
                                            AVM2_VERBOSE( "%s : %s != %s\n", opCode, stack.top( 0 ).asCString(), stack.top( 1 ).asCString() );
                                            if( stack.pop() != stack.pop() ) {
                                                op = i.offset;
                                            }
                                        }
                                        break;

            case IfNotGreater:          {
                                            AVM2_VERBOSE( "%s : %s <= %s\n", opCode, stack.top( 0 ).asCString(), stack.top( 1 ).asCString() );
                                            double b  = stack.pop().asNumber();
                                            double a  = stack.pop().asNumber();
                                            bool   ng = a <= b;
                                            if( ng ) {
                                                op = i.offset;
                                            }
                                        }
                                        break;

            case IfNotLess:             {
                                            AVM2_VERBOSE( "%s : %s <= %s\n", opCode, stack.top( 0 ).asCString(), stack.top( 1 ).asCString() );
                                            double b  = stack.pop().asNumber();
                                            double a  = stack.pop().asNumber();
                                            bool   nl = a >= b;

                                            if( nl ) {
                                                op = i.offset;
                                            }
                                        }
                                        break;

            case LookupSwitch:          {
                                            AVM2_VERBOSE( "%s : %d\n", opCode, stack.top().asInt() );
                                            int index = stack.pop().asInt();
                
                                            if( index >= 0 && index < i.caseCount ) {
                                                op = i.caseOffsets[index];
                                            } else {
                                                op = i.defaultOffset - 1;
                                            }
                                        }
                                        break;
                
            case DebugFile:             debugFile = i.Str->toCString();
                                        break;
                
            case DebugLine:             debugLine = i.line;
                                        break;

            case Debug:                 break;
                
            case Throw:                 {
                                            AVM2_VERBOSE( "%s : %s\n", opCode, stack.top().asCString() );
                                            frame->throwException( stack.pop() );
                                            AvmHandleException( frame );
                                        }
                                        break;

            default:        printf( "AVM::ExecuteMethod : unhandled instruction %s(0x%x)\n", opCode, i.opCode );
                            assert( false );
        }
        
        AVM2_VERBOSE( "\n" );
    }
}

// ** Avm::throwError
void Avm::throwError( Frame* frame, ErrorId errorId, const char *message, ... ) const
{
    const char* errorName[TotalErrors] = {
        "InternalError", "TypeError", "ReferenceError", "ArgumentError", "VerifyError"
    };

    const int kFormatBufferSize = 4096;

    char szBuf[kFormatBufferSize];

    va_list ap;
    va_start( ap, message );
    vsnprintf( szBuf, kFormatBufferSize, message, ap );
    va_end( ap );

    Value error = Error::create( m_player, "%s: Error #%d: %s", errorName[errorId], errorId * 100, szBuf );
    frame->throwException( error );
}

// ** Avm::handleException
bool Avm::handleException( const Exceptions& exceptions, Frame* frame, int& index, const char* opCode ) const
{
    Stack&       stack              = frame->m_stack;
    ScopeStack&  scope              = frame->m_scope;
    const Value& exception          = frame->m_exception;
    bool         continueExecution  = false;

    for( int i = 0, n = ( int )exceptions.size(); i < n; i++ ) {
        Exception* e = exceptions[i].get();

        // ** Check exception bounds
        if( index < e->from() || index > e->to() ) {
            continue;
        }

        // ** Skip finally blocks
        if( e->isFinally() ) {
            continueExecution = true;
            continue;
        }

        // ** Check exception type
        if( e->type() && !exception.is( e->type() ) ) {
            continue;
        }

        index = e->target() - 1;

        stack.clear();
        scope.clear();
        stack.push( exception, opCode );

        frame->handleException();

        return true;
    }

    return continueExecution;
}

// ** Avm::resolveProperty
TraitResolution Avm::resolveProperty( Value& object, Value& value, const Multiname* identifier, Frame* frame, bool needsClosure ) const
{
    assert( !identifier->hasRuntimeNamespace() );

    Stack&          stack      = frame->m_stack;
    TraitResolution resolution = TraitUnresolved;

    if( identifier->hasRuntimeName() ) {
        const_cast<Multiname*>( identifier )->setName( stack.pop().asString() );
    }

//  const Namespace* ns = identifier->hasRuntimeNamespace() ? stack.pop().to_namespace() : NULL;

    // ** Pop object from stack and ensure it's a valid reference
    object = stack.pop();
    if( object.isNullOrUndefined() ) {
        value = Value::undefined;
        return resolution;
    }

    // ** Resolve property
    if( Object* o = object.asObject() ) {
        resolution = o->resolveProperty( identifier, &value, frame->accessScope() );
    }

    if( Property* property = value.asProperty() ) {
        property->get( object, &value );
    }

    // ** Create closure
    if( needsClosure ) {
        createClosure( object.asObject(), &value );
    }

    return resolution;
}

// ** Avm::isType
bool Avm::isType( const Value& value, const Class* type ) const
{
    if( Object* object = value.asObject() ) {
        return object->m_class->is( type );
    }

    if( value.isNumber() && type->name() == "Number" ) return true;

    return false;
}

// ** Avm::setProperty
bool Avm::setProperty( const Multiname* identifier, Frame* frame, const Value& value )
{
    assert( !identifier->hasRuntimeNamespace() );

    Stack& stack  = frame->m_stack;

    if( identifier->hasRuntimeName() ) {
        const_cast<Multiname*>( identifier )->setName( stack.pop().asString() );
    }

//  const Namespace* ns = identifier->hasRuntimeNamespace() ? stack.pop().to_namespace() : NULL;
    Object* object = stack.pop().asObject();

    if( !object ) {
        return false;
    }

    return object->setProperty( identifier->name(), value, frame->accessScope() ) == TraitResolved;
}

// ** Avm::findProperty
Object* Avm::findProperty( const Multiname* identifier, Frame* frame, Value* value, bool needsClosure ) const
{
    assert( !identifier->hasRuntimeName() );
    assert( !identifier->hasRuntimeNamespace() );

    Stack&           stack = frame->m_stack;
    ScopeStack&      scope = frame->m_scope;
    const Str&       name  = identifier->hasRuntimeName()      ? stack.pop().asString()     : identifier->name();
//  const Namespace* ns    = identifier->hasRuntimeNamespace() ? stack.pop().to_namespace() : NULL;

    // ** Search inside the scope stack
    if( Object* object = scope.find( identifier, value, frame->accessScope() ) ) {
        if( needsClosure ) {
            createClosure( object, value );
        }

        return object;
    }

    // ** Search packages
    const Namespaces& namespaces = identifier->namespaces();
    assert( namespaces.size() );

    for( int i = 0, n = ( int )namespaces.size(); i < n; i++ ) {
        Package* package = namespaces[i]->package();

        if( !package ) {
            continue;
        }

        if( package->get_member( name, value ) ) {
            return package;
        }
    }
    
    return NULL;
}

// ** Avm::createClosure
void Avm::createClosure( Object *instance, Value *value ) const
{
    if( instance == NULL ) {
        return;
    }

    assert( value );

    // ** Ensure the value is a function
    Function* function = value->asFunction();
    if( !function ) {
        return;
    }

    // ** Do nothing if the value is a closure
    if( cast_to<FunctionClosure>( function ) ) {
        return;
    }

    value->setObject( instance->enclose( function ) );
}

// ** Avm::dumpStack
void Avm::dumpStack( const char *id, const Stack& stack )
{
    printf( "\t\t------- %s -------\n", id );
    for( int i = stack.size() - 1; i >= 0; i-- ) {
        printf( "\t\t%s pushed by %s\n", stack.at( i ).asCString(), stack.pushedBy( i ).c_str() );
    }
}

// ** Avm::dumpScopeStack
void Avm::dumpScopeStack( const char *id, const ScopeStack& stack )
{
    printf( "\t\t------- %s -------\n", id );
    for( int i = stack.size() - 1; i >= 0; i-- ) {
        printf( "\t\t%d: %s pushed by %s\n", i, stack.at( i ) ? stack.at( i )->to_string() : "null", stack.pushedBy( i ).c_str() );
    }
}

}