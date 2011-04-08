#!/usr/bin/env python

from xmlrpclib import ServerProxy 
import pprint
import sys

if len(sys.argv) < 2:
	print "Usage: "+sys.argv[0]+" FilenameToSearchFor"
	exit(0)

#proxy = ServerProxy('http://localhost/springfiles/xmlrpc.php')
proxy = ServerProxy('http://springfiles.com/xmlrpc.php')
searchstring = {
#	"category" : "Spring Maps",
	"logical" : "or",
	"filename" : sys.argv[1],
	"springname" : sys.argv[1],
}

pp = pprint.PrettyPrinter(depth=6)
pp.pprint(proxy.springfiles.search(searchstring))

