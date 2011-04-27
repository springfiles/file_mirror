#!/usr/bin/env python

# Adds an url for queueing of springfiles.com upload/mirroring service

from xmlrpclib import ServerProxy 
import pprint
import sys

if len(sys.argv) < 6:
	print "Usage: "+sys.argv[0]+" user pass category filename url"
	exit(0)

proxy = ServerProxy('http://springfiles.com/xmlrpc.php')
data = {
	"username" : sys.argv[1],
	"password" : sys.argv[2],
	"category" : sys.argv[3],
	"filename" : sys.argv[4],
	"url" : sys.argv[5],
}

pp = pprint.PrettyPrinter(depth=6)
pp.pprint(proxy.springfiles.upload(data))

