# Keeps the first "ITextParsers *textparsers = NULL;" in SourceMod 1.5's extension glue.
/^[ \t]*ITextParsers[ \t]*\*[ \t]*textparsers[ \t]*=[ \t]*NULL;/ { if (seen++) next }
{ print }
