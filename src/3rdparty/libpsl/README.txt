The file psl_data.cpp is generated from the Public Suffix
List (see [1] and [2]), by the program residing at
src/psl-make-dafsa.

To regenerate the file, run the following command from qtbase tree:

    src/3rdparty/libpsl/src/psl-make-dafsa public_suffix_list.dat src/3rdparty/libpsl/psl_data.cpp
    src/3rdparty/libpsl/src/psl-make-dafsa --output-format=binary public_suffix_list.dat \
        tests/auto/network/access/qnetworkcookiejar/testdata/publicsuffix/public_suffix_list.dafsa

Those arrays in psl_data.cpp are derived from the Public
Suffix List ([2]), which was originally provided by
Jo Hermans <jo.hermans@gmail.com>.

----
[1] list: https://publicsuffix.org/list/public_suffix_list.dat
[2] homepage: https://publicsuffix.org/
