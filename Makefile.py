import os, sys

sys.path.append( os.path.join( Makefile.getSourceDir(), 'tools' ) )

from SwfTag import SwfTag

if os.name == 'nt':
    pass
else:
	Makefile.set( 'MACOS_SDK', 'macosx10.8' )
	Makefile.set( 'IOS_SDK', 'iphoneos7.1' )

# Project
project = Makefile.getProject()

project.include( '.' )
project.registerExtension( 'SwfTag', SwfTag )
project.extensions( 'SwfTag' )

# Dependecies
zlib = Target( 'zlib', project )
zlib.dirs( 'zlib' )
zlib.staticLibrary()

# Library
library = Target( 'swf', project )

if platform == 'iOS':
	library.define( 'TU_USE_OGLES2' )

library.include( 'zlib' )
library.dirs( 'base', 'gameswf', 'gameswf/*', 'gameswf/flash/*', 'gameswf/tags' )
library.link( 'zlib' )
library.staticLibrary()

# Executable
executable = Target( 'player', project )
executable.files( 'player/main.cpp' )
executable.link( 'swf' )
executable.executable()