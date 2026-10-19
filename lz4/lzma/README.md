LZMA encoder from the LZMA SDK (public domain), 7-Zip 26.04:
https://github.com/ip7z/7zip/tree/26.04/C

Only the files needed by `LzmaEncode` are copied, compiled with `Z7_ST`
(single threaded encoder, no `LzFindMt`).
