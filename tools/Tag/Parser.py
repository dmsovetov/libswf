import tokenize

from Type  import Type
from Field import Field

# class Parser
class Parser:
    # ctor
    def __init__( self, name ):
        self.name   = name
        self.fields = []
        self.tokens = []
        self.header = None

    # parse
    def parse( self, fileName ):
        # Tokenize string
        self.tokenize( fileName )

        # Nothing to parse
        if not self.hasTokens():
            return

        # Parse tokens
        self.expectToken( '{' )
        while not self.checkToken( '}' ):
            if self.parseToken( 'if' ):
                self.parseCondition()
            elif self.parseToken( 'header' ):
                self.expectToken( '=' )
                self.header = self.readToken()
            else:
                self.parseField()

        self.expectToken( '}' )

    # parseCondition
    def parseCondition( self ):
        condition = []

        # Read expression
        while not self.checkToken( '{' ):
            condition.append( self.readToken() )
        self.expectToken( '{' )

        condition = ' '.join( condition )

        # Read fields
        while not self.checkToken( '}' ):
            self.parseField( condition )
        self.expectToken( '}' )

    # parseField
    def parseField( self, condition = None ):
        type = self.parseType()
        name = self.readToken()

        self.fields.append( Field( type, name, condition ) )

    # parseType
    def parseType( self ):
        name  = self.readToken()
        count = 0

        if self.parseToken( '[' ):
            if self.parseToken( ']' ):
                count = -1
            else:
                count = self.readToken()
                self.expectToken( ']' )

        return Type( name, count )

    # tokenize
    def tokenize( self, fileName ):
        # handleToken
        def handleToken( type, token, (srow, scol), (erow, ecol), line ):
            if tokenize.tok_name[type] == 'NL':
                return

            self.tokens.append( token )

        file = open( fileName )
        tokenize.tokenize( file.readline, handleToken )

    # hasTokens
    def hasTokens( self ):
        return len( self.tokens ) > 0

    # readToken
    def readToken( self ):
        assert self.hasTokens(), 'no tokens to read'
        token = self.tokens[0]
        self.tokens = self.tokens[1:]
        return token

    # checkToken
    def checkToken( self, token ):
        return self.tokens[0] == token if self.hasTokens() else False

    # parseToken
    def parseToken( self, token ):
        if self.checkToken( token ):
            self.readToken()
            return True

        return False

    # expectToken
    def expectToken( self, expected ):
        # Check end of file
        if not self.hasTokens():
            print '{0} expected, got end of file'.format( expected )
            return

        # Read token
        token = self.readToken()

        # Compare tokens
        if token != expected:
            print '{0} expected, got {1}'.format( expected, token )
            self.tokens = []