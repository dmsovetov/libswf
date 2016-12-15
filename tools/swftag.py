import glob, os, argparse

from Tag import Parser
from Tag import Generator

# Entry point
if __name__ == "__main__":
	# Parse arguments
	parser = argparse.ArgumentParser( description = 'Dreemchest SWF tag reader generator' )

	parser.add_argument( 'input',  type = str, help = 'Input File' )
	parser.add_argument( 'output', type = str, help = 'Output File' )

	args = parser.parse_args()

	# Generate tag
	name, ext = os.path.splitext( os.path.basename( args.input ) )
	tag = Parser( name )
	tag.parse( args.input )

	gen = Generator()
	gen.generate( args.output, tag )


#	tag = Parser( 'Rgb' )
#	tag.parse( '../gameswf/tags/Rgb.tag' )

#	gen = Generator()
#	gen.generate( 'Rgb.h', tag )

#	tags = []

#	for fileName in glob.glob( '../gameswf/tags/*.tag' ):
#		print 'Parsing', fileName

#		name, ext = os.path.splitext( os.path.basename( fileName ) )

#		tag = Parser( name )
#		tag.parse( fileName )

#		gen = Generator()
#		gen.generate( name + '.h', tag )

#		tags.append( name )

#	Generator.generateTags( 'Tags.h', tags )

