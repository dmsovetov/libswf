//
//  Script.h
//  GameSWF
//
//  Created by Советов Дмитрий on 08.07.14.
//  Copyright (c) 2014 Советов Дмитрий. All rights reserved.
//

#ifndef __GameSWF__Script__
#define __GameSWF__Script__

#include "Function.h"
#include "Trait.h"
#include "Package.h"

namespace gameswf {

    // ** class Script
    class Script : public ref_counted {
    public:

						//! Constructs a new script.
                        Script( Function* init, Traits* traits, Package* package );

		//! Returns a parent Package.
        Package*        package( void ) const;
		//! Returns an initializer Function.
        Function*       init( void ) const;
		//! Returns the script traits.
        Traits*         traits( void ) const;

    private:

		//! Parent package.
        PackagePtr      m_package;
		//! Initializer function.
        FunctionPtr     m_init;
		//! Script traits.
        TraitsPtr       m_traits;
    };

    typedef gc_ptr<Script>   ScriptPtr;
    typedef array<ScriptPtr> Scripts;

} // namespace gameswf

#endif /* defined(__GameSWF__Script__) */
