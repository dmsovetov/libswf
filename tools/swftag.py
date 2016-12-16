import tokenize
from collections import namedtuple

field = namedtuple('field', 'type name size condition')
tag = namedtuple('tag', 'name fields')


def tokenize_file(file_name, filter):
    """Performs a text file tokenization"""

    tokens = []

    def handle_token(type, token, (srow, scol), (erow, ecol), line):
        if tokenize.tok_name[type] == 'NL':
            return

        tokens.append(token)

    with open(file_name) as fh:
        tokenize.tokenize(fh.readline, handle_token)

    return [token for token in tokens if token not in filter]


def read_token(stream):
    """Reads a single token from an input stream"""

    name = stream[0]
    stream.pop(0)
    return name


def expect_token(stream, text):
    if len(stream) == 0:
        raise StandardError("unexpected end of file")

    if stream[0] != text:
        raise StandardError("unexpected token '%s'" % stream[0])

    read_token(stream)


def check_token(stream, text):
    if stream[0] == text:
        return True

    return False


def parse_token(stream, text):
    if check_token(stream, text):
        read_token(stream)
        return True

    return False


def parse_size(stream):
    if not parse_token(stream, ':'):
        return None

    return read_token(stream)


def parse_optional_fields(stream):
    condition = ''

    while not check_token(stream, '{'):
        condition += read_token(stream)
    expect_token(stream, '{')

    fields = []
    while not check_token(stream, '}'):
        fields.append(parse_field(stream, condition))
    expect_token(stream, '}')

    return fields


def parse_field(stream, condition=None):
    type = read_token(stream)
    size = parse_size(stream)
    name = read_token(stream)
    return field(type, name, size, condition)


def parse_tag(stream):
    """Parses a single tag from a token stream"""

    name = read_token(stream)
    fields = []

    expect_token(stream, '{')
    while not check_token(stream, '}'):
        if parse_token(stream, 'if'):
            fields += parse_optional_fields(stream)
        else:
            fields.append(parse_field(stream))
    parse_token(stream, '}')

    return tag(name, fields)


def parse_tags(stream):
    """Parses all tags defined by a list of tokens"""

    tags = []

    while len(stream) > 0:
        tags.append(parse_tag(stream))

    return tags


def compile_template(template, **kwargs):
    for k, v in kwargs.items():
        if v is not None:
            template = str.replace(template, '{%s}' % k, v)
    return template

TAG_STRUCT = """
struct {name}
{
    virtual void read(stream& stream)
    {
        {reader}
    }

    {fields}
};
"""

FIELD = "{type} {name};"

READ_FIELD = "stream.read({name});"
OPTIONAL_FIELD = "if ({condition}) stream.read({name});"

# Entry point
if __name__ == "__main__":
    tokens = tokenize_file('Argb.tag', ['', '\n', '\t', '\r\n'])

    for tag in parse_tags(tokens):
        fields = '\n\t'.join([compile_template(FIELD, type=field.type, name=field.name) for field in tag.fields])
        reader = '\n\t\t'.join([compile_template(OPTIONAL_FIELD if field.condition else READ_FIELD, name=field.name, condition=field.condition) for field in tag.fields])
        print compile_template(TAG_STRUCT, name=tag.name, fields=fields, reader=reader)
        #print str.replace(TAG_STRUCT, '{name}', tag.name)
        '''
        print tag.name

        for field in tag.fields:
            if field.condition is not None:
                print '\tif ', field.condition, field.type, (field.size if field.size else ''), field.name
            else:
                print '\t', field.type, (field.size if field.size else ''), field.name
        '''
