In 1958, Mahatma Gandhi was quoted as follows:

> The Earth provides enough to satisfy every man's need but not for every man's
> greed.

In [The CommonMark Specification](https://spec.commonmark.org/0.29/) John
MacFarlane writes:

> What distinguishes Markdown from many other lightweight markup syntaxes,
> which are often easier to write, is its readability. As Gruber writes:

> > The overriding design goal for Markdown's formatting syntax is to make it
> > as readable as possible. The idea is that a Markdown-formatted document should
> > be publishable as-is, as plain text, without looking like it's been marked up
> > with tags or formatting instructions. ( 
> > [http://daringfireball.net/projects/markdown/](http://daringfireball.net/projects/markdown/)
> > )

> The point can be illustrated by comparing a sample of AsciiDoc with an
> equivalent sample of Markdown. Here is a sample of AsciiDoc from the AsciiDoc
> manual:

> ```AsciiDoc
> 1. List item one.
> +
> List item one continued with a second paragraph followed by an
> Indented block.
> +
> .................
> $ ls *.sh
> $ mv *.sh ~/tmp
> .................
> +
> List item continued with a third paragraph.
> 
> 2. List item two continued with an open block.
> ...
> ```
The quotation includes an embedded quotation and a code quotation and ends with
an ellipsis due to being incomplete.

Now let's have an indented code block:

    #include <stdio.h>
    
    int main(void)
    {
        printf("# hello markdown\n");
        return 0;
    }

and end with a fenced code block:
~~~pseudocode
#include <something.h>
#include <else.h>

a block {
    a statement;
    another statement;
}
~~~

