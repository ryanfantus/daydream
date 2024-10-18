import sys

print "#include \"headers.h\""
print ""
print "struct ftscprod_ products[] = {"

while 1:
	buf=sys.stdin.readline()

	if buf == "":
		break

	a = buf.split(',') 

	print "  {0x%s, (char*) \"%s\"}," % (a[0], a[1])

print "  {0xff,(char*)0L}"
print "};"
