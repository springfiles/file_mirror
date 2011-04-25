#!/usr/bin/env python
from unitsync import unitsync as unitsyncpkg
import sys
import os
import ctypes
import Image
import shutil
import getopt
import base64

import sys
sys.path.append('metalink')
import metalink

from xml.dom import minidom


springcontent = [ 'bitmaps.sdz', 'springcontent.sdz', 'maphelper.sdz', 'cursors.sdz' ]

def getMapPositions(usync,doc, idx, Map):
	positions = doc.createElement("Positions")
	startpositions = usync.GetMapPosCount(idx)
	for i in range(0, startpositions):
		startpos=doc.createElement("StartPos")
		getXmlData(doc, startpos, "X", str(usync.GetMapPosX(idx, i)))
		getXmlData(doc, startpos, "Z", str(usync.GetMapPosZ(idx, i)))
		positions.appendChild(startpos)
	Map.appendChild(positions)

def getXmlData(doc, parent, element, value):
	node = doc.createElement(element)
	value = str(value)
	value = value.decode('utf-8','replace')
	subnode = doc.createTextNode(value)
	node.appendChild(subnode)
	parent.appendChild(node)

def getMapDepends(usync,doc,idx,Map,maparchivecount):
	for j in range (1, maparchivecount): # get depends for file, idx=0 is filename itself
		deps=os.path.basename(usync.GetMapArchiveName(j))
		node = doc.createElement("Depends")
		if not deps in springcontent:
			getXmlData(doc, node, "Depend", str(deps))
	Map.appendChild(node)

def writeMapXmlData(usync, smap, idx, filename,maparchivecount,archivename):
        if os.path.isfile(filename):
		print filename + " already exists, skipping..."
	else:
		doc = minidom.Document()
		smap = doc.createElement("Map")
		getXmlData(doc, smap, "Name", usync.GetMapName(idx))
		getXmlData(doc, smap, "Author", usync.GetMapAuthor(idx))
		getXmlData(doc, smap, "Description", usync.GetMapDescription(idx))
		#ExtractorRadius
		getXmlData(doc, smap, "Gravity", str(usync.GetMapGravity(idx)))
		#HumanName
		#MaxMetal
		getXmlData(doc, smap, "MaxWind", str(usync.GetMapWindMax(idx)))
		getXmlData(doc, smap, "MinWind", str(usync.GetMapWindMin(idx)))
		getXmlData(doc, smap, "Torrent", get_torrent(archivename))
		#Options
		getMapPositions(usync,doc,idx,smap)
		#TidalStrength
		#Checksum
		#ArchiveName
		getMapDepends(usync,doc,idx,smap,maparchivecount)
		doc.appendChild(smap)
		print "Wrote "+filename
		tmp=".tmp.xml"
		metadata = open(tmp,'w')
		metadata.write(doc.toxml("utf-8"))
		metadata.close()
		shutil.move(tmp,filename)

# remove a file suffix
def rstrip(s):
	for suffix in ('.sd7', '.sdf', '.sdz'):
		if suffix and s.endswith(suffix):
			s = s[:-len(suffix)]
	return s

# extracts minimap from given file
def createMapImage(usync, mapname, outfile, size):
	if os.path.isfile(outfile):
		print outfile + " already exists, skipping..."
	else:
		data=ctypes.string_at(usync.GetMinimap(mapname, 0), 1024*1024*2)
		im = Image.frombuffer("RGB", (1024, 1024), data, "raw", "BGR;16")
		im=im.resize(size)
		tmp=".tmp.jpg" # first create tmp file
		im.save(tmp)
		shutil.move(tmp,outfile) # rename to dest
		print "wrote "+outfile

def createMapInfoImage(usync, mapname, maptype, byteperpx, decoder,decoderparm, outfile, size):
	if os.path.isfile(outfile):
		print outfile + " already exists, skipping..."
	else:
		width = ctypes.pointer(ctypes.c_int())
		height = ctypes.pointer(ctypes.c_int())
		usync.GetInfoMapSize(mapname, maptype, width, height)
		width = width.contents.value
		height = height.contents.value
		data = ctypes.create_string_buffer(int(width*height*byteperpx*2))
		data.restype = ctypes.c_void_p
		ret=usync.GetInfoMap(mapname, maptype, data, byteperpx)
		if (ret<>0):
			im = Image.frombuffer(decoder, (width, height), data, "raw", decoderparm)
			im=im.convert("L")
			im=im.resize(size)
			tmp=".tmp.jpg"
			im.save(tmp)
			shutil.move(tmp,outfile)
			print "wrote "+outfile


def dumpmap(usync, springname, outpath, filename, idx):
	metalmap = outpath + '/' + filename + ".metalmap" + ".jpg"
	heightmap = outpath + '/' + filename + ".heightmap" + ".jpg"
	mapimage = outpath + '/' + filename + ".jpg"
	if os.path.isfile(metalmap) and os.path.isfile(heightmap) and os.path.isfile(mapimage):
		print "Already exported " + mapimage
	else:
		mapwidth=float(usync.GetMapWidth(idx))
		mapheight=float(usync.GetMapHeight(idx))
		if mapwidth>mapheight:
			scaledsize=(1024, int(((mapheight/mapwidth) * 1024)))
		else:
			scaledsize=(int(((mapwidth/mapheight) * 1024)), 1024)
		createMapImage(usync,springname,mapimage, scaledsize)
		createMapInfoImage(usync,springname, "height",2, "RGB","BGR;15", heightmap, scaledsize)
		createMapInfoImage(usync,springname, "metal",1, "L","L;I", metalmap, scaledsize)
	
def getGameDepends(usync, idx, gamearchivecount, doc, game):
	depends = doc.createElement("Depends")
	game.appendChild(depends)
	for i in range (1, gamearchivecount): # get depends for file, idx=0 is filename itself
		deps=os.path.basename(usync.GetPrimaryModArchiveList(i))
		print deps
		if not deps in springcontent and not deps.endswith(".sdp"): #FIXME: .sdp is returned wrong by unitsync
			if deps in springnames:
				depend=springnames[deps]
			else:
				depend=deps
			getXmlData(doc, depends, "Depend", depend)

def writeGameXmlData(usync, springname, idx, filename,gamesarchivecount, archivename):
	if os.path.isfile(filename):
		print filename + " already exists, skipping..."
	else:
		doc = minidom.Document()
		game = doc.createElement("Game")
		doc.appendChild(game)
		getXmlData(doc, game, "Name", springname)
		getXmlData(doc, game, "Description", usync.GetPrimaryModDescription(idx))
		getXmlData(doc, game, "Version", usync.GetPrimaryModVersion(idx))
		getXmlData(doc, game, "Torrent", get_torrent(archivename))
		getGameDepends(usync, idx, gamesarchivecount, doc, game)
		tmp=".tmp.xml"
		f=open(tmp, 'w')
		f.write(doc.toxml("utf-8"))
		f.close()
		shutil.move(tmp,filename)


def usage():
	print "--help \n\
--output <outputdir>\n\
--unitsync <libunitsync.so>\n\
--datadir <maps/gamesdir>\n\
"
springnames={}
def createdict(usync,gamescount, mapcount):
	#create dict with springnames[filename]=springname
	for i in range(0, gamescount):
		springname=usync.GetPrimaryModName(i)
		filename=usync.GetPrimaryModArchive(i)
		springnames[filename]=springname
	for i in range(0, mapcount): 
		maparchivecount = usync.GetMapArchiveCount(usync.GetMapName(i)) # initialization for GetMapArchiveName()
		filename = os.path.basename(usync.GetMapArchiveName(0))
		print "["+str(i) +"/"+ str(mapcount)+ "] extracting data from "+filename
		springname = usync.GetMapName(i)
		springnames[filename]=springname

def get_torrent(filename):
	metalink._opts = { 'overwrite': False }
	filesize=os.path.getsize(filename)
	torrent = metalink.Torrent(filename)
	m = metalink.Metafile()
	m.hashes.filename=filename
	m.scan_file(filename, True, 255, 1)
	m.hashes.get_multiple('ed2k')
	data = {'files':[[metalink.encode_text(filename), int(filesize)]],
		'piece length':int(m.hashes.piecelength),
		'pieces':m.hashes.pieces,
		'encoding':'UTF-8',
		}
	return base64.b64encode(torrent.create(data))

def main():
	try:
		opts, args = getopt.getopt(sys.argv[1:], "ho:u:d:t", ["help", "output=", "unitsync=", "datadir="])
	except getopt.GetoptError, err:
		print str(err)
		sys.exit(2)
	unitsync="/usr/lib/spring/libunitsync.so"
	outputpath="../../../default/files/springdata"
	datadir=outputpath
	for o, a in opts:
		if o == ("-u", "--unitsync"):
			unitsync=a
		elif o in ("-o", "--output"):
			outputpath=a
		elif o in ("-d", "--datadir"):
			datadir=a
		elif o in ("-t", "--test"):
			test(unitsync)
		elif o in ("-h","-?","--help"):
			usage()
			sys.exit()
	
	outputpath = os.path.abspath(outputpath)
	os.environ["SPRING_DATADIR"]=outputpath
	usync = unitsyncpkg.Unitsync(unitsync)

	usync.Init(True,1)
	mapcount = usync.GetMapCount()
	gamescount = usync.GetPrimaryModCount()
	createdict(usync,gamescount, mapcount)
	for i in range(0, mapcount):
		maparchivecount = usync.GetMapArchiveCount(usync.GetMapName(i)) # initialization for GetMapArchiveName()
		filename = os.path.basename(usync.GetMapArchiveName(0))
		print "["+str(i) +"/"+ str(mapcount)+ "] extracting data from "+filename
		springname = usync.GetMapName(i)
		dumpmap(usync, springname, outputpath, filename,i)
		writeMapXmlData(usync, springname, i, outputpath +"/" +filename+".metadata.xml",maparchivecount, usync.GetArchivePath(filename)+filename)
	for i in range (0, gamescount):
		springname=usync.GetPrimaryModName(i)
		filename=usync.GetPrimaryModArchive(i)
		print "["+str(i) +"/"+ str(gamescount)+ "] extracting data from "+filename
		gamearchivecount=usync.GetPrimaryModArchiveCount(i) # initialization for GetPrimaryModArchiveList()
		writeGameXmlData(usync, springname, i, outputpath + "/" + filename + ".metadata.xml", gamearchivecount, usync.GetArchivePath(filename)+filename)
	print "Parsed "+ str(gamescount) + " games, " + str(mapcount) + " maps"

if __name__ == "__main__":
    main()

