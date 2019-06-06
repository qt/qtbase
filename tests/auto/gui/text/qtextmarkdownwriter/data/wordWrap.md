[The CommonMark Specification](https://spec.commonmark.org/0.29/) is the
conservative formal specification of the Markdown format, while 
[GitHub Flavored Markdown](https://guides.github.com/features/mastering-markdown/#GitHub-flavored-markdown)
adds extra features such as task lists and tables.

Qt owes thanks to the authors of the [MD4C parser](https://github.com/mity/md4c)
for making markdown import possible. The QTextMarkdownWriter class does not
have such dependencies, and also has not yet been tested as extensively, so we
do not yet guarantee that we are able to rewrite every Markdown document that
you are able to read and display with Text or QTextEdit. But you are free to
write [bugs](https://bugreports.qt.io) about any troublesome cases that you
encounter.

