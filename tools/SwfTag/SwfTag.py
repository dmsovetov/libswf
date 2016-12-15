import os

# class SwfTag
class SwfTag:
	# ctor
	def __init__( self, target, makefile ):
		self.makefile = makefile

	# wrap
	def wrap( self, target, generator, fileName, extension ):
		if not os.path.exists( fileName ) or extension != '.tag':
			return False

		# Add command
		name, ext 	 	= os.path.splitext( os.path.basename( fileName ) )
		outputFolder	= generator.getPathForTarget( target )
		outputFileName	= os.path.join( outputFolder, name )

		cmd = 'python ' + os.path.join( self.makefile.getSourceDir(), 'tools', 'swftag.py' ) + ' ' + fileName + ' ' + outputFileName + '.h'

		target.command( [ fileName ], outputFileName, cmd, 'Generating {0}.h...'.format( name ), ['.h'] )

		return True