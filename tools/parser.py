import tokenize


def tokenize_file(file_name):
    """Performs a text file tokenization"""

    tokens = []

    def handle_token(type, token, (srow, scol), (erow, ecol), line):
        if tokenize.tok_name[type] == 'NL':
            return

        tokens.append(token)

    with open(file_name) as fh:
        tokenize.tokenize(file.readline, handle_token)

    return tokens
