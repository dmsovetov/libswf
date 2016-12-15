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
