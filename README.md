#about

file_mirror is a module for drupal on springfiles.com, that mirrors files to other sites, indexes them and allows searcheing through an xml-rpc.


##avaiable functions in on http://springfiles.com/xmlrpc.php

###springfiles.search
See doc/examplequery for more details or try doc/exampleclient.py
optional parameters:
	logical = and | or
	torrent = true

the file_mirror module allows to search through this data, look at exampleclient.py to see, how it works.

###springfiles.upload
notifies springfiles.com about a new file. After a short time, the file will be downloaded and added to the file mirroring system.

required parameters:
	username
	password
	category = games | maps
	filename
	url



