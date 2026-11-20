BEGIN {
    print "#ifndef " prefix "_H"
    print "#define " prefix "_H"
    print ""
}

$2 == "A" && $1 ~ ("^" prefix "_[A-Z0-9_]+_OFFSET$") {
    printf "#define %s %d\n", $1, $3
    count++
}

END {
    if (!count)
        exit 1
    print ""
    print "extern unsigned char " name "[];"
    print "extern unsigned int " name "_len;"
    print ""
    print "#endif"
}
