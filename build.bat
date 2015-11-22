gcc -Os main.c -o convHex.exe
strip -s -R .comment -R .gnu.version --strip-unneeded convHex.exe