# burst

Example:
```
% ls
burst  burst.c  burst.h  example  Makefile  packed.txt.gz  README.md  screenlog.0  words.tar.gz  words.txt
% wc --lines words.txt
1999 words.txt
% ./burst words.txt
% cat words.txt.* | wc --lines
1999
% cat words.txt.* | diff - words.txt
%
```