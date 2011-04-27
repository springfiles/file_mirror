#!/usr/bin/env python

from xmlrpclib import ServerProxy 
import pprint
import sys

if len(sys.argv) < 6:
	print "Usage: "+sys.argv[0]+" user pass category filename url"
	exit(0)

user=sys.argv[1]
password=sys.argv[2]
category=sys.argv[3]
filename=sys.argv[4]
url=sys.argv[5]

proxy = ServerProxy('http://springfiles.dev/xmlrpc.php')
data = {
	"username" : user,
	"password" : password,
	"category" : category,
	"filename" : filename,
	"url" : url,
}

pp = pprint.PrettyPrinter(depth=6)
pp.pprint(proxy.springfiles.upload(data))

