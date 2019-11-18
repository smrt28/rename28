
File patterns:
==============
%n = filename
%e = ext
%% = '%' character
%N = fileorder
%. = '.' character if it's not the last in the string

Prefixes:
=========
u - uppercase
l - lowercase
[0-9]+ - number padding

Example:
========
"filename.ext", "%un.%ue => "FILENAME.EXT"
"filename.EXT", "%un.%lu => "FILENAME.ext"
"filename.ext", "%n-%3N.%u => "filename-001.ext"
