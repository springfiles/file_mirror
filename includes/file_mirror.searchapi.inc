<?php

/**
* @param query resulting query, for example "AND filename='%s'"
* @param vars data for query, for example "Zero-K v0.51"
* @param logical how the queries are assigned, AND/OR
* @param condition SQL condition, for example "LIKE filename '%s'" or "filename='%s'"
* @param data data from the xml-rpc request, if empty string, then don't change query + vars
*/

function _file_mirror_createquery(&$query, &$vars, $logical,$condition, $data){
	if($query=="") $logical="";
	
	if($data=="")
		return;
	$query.=" ".$logical." ".$condition;
	$data=str_replace("_","\_",$data);
	array_push($vars, $data);
}

function _file_mirror_gettorrent($filename){
	$file=file_directory_path()."/springdata/".$filename.".torrent";
	if (is_readable($file)){
		return base64_encode(file_get_contents($file));
	}
	watchdog("file_mirror", "no torrent info for $file");
	return "";
}

function _file_mirror_getlink($fid){
	$res=db_query("SELECT nid FROM {content_field_file} as c WHERE field_file_fid=%d
", array($fid));
	$nid=db_result($res);
	if ($nid<=0)
		return "";
	$res=db_query("SELECT dst FROM {url_alias} WHERE src='%s'",array("node/$nid"));
	$url=db_result($res);
	if (strlen($url)>0)
		return "http://springfiles.com/$url";
	return "http://springfiles.com/node/$nid";
}

/**
*	implementation of the xml-rpc call
*/

function file_mirror_xmlsearch($req){
	global $base_url;
	$res="";
	$category="%";
	$query="";
	$vars=array();

	if(isset($req['logical'])&&($req['logical']=="or")){
		$logical="OR";
	}else{
		$logical="AND";
	}

	_file_mirror_createquery($query,$vars, $logical,'f.filename LIKE BINARY "%s"', $req['filename']);
	_file_mirror_createquery($query,$vars, $logical,'c.name LIKE BINARY "%s"', $req['category']);
	_file_mirror_createquery($query,$vars, $logical,'a.springname LIKE BINARY "%s"', $req['springname']);

	if($query!="")
		$query=" AND (".$query.")";
	$result=db_query("SELECT
		f.fid as fid,
		f.filename as filename,
		f.filepath as filepath,
		h.md5 as md5,
		a.springname as springname,
		c.name as category,
		f.filesize as size,
		f.timestamp as timestamp
		FROM files as f
		LEFT JOIN springdata_archives as a ON f.fid=a.fid
		LEFT JOIN springdata_categories as c ON a.cid=c.cid
		LEFT JOIN filehash as h ON f.fid=h.fid
		WHERE c.cid>0
		$query
		ORDER BY f.fid DESC
		LIMIT 0,10
		",
		$vars
	);
	$res=array();
	while($row = db_fetch_array($result)){
		//add primary server
		$row['mirrors']=array($base_url.'/'.$row['filepath']);
		unset($row['filepath']);
		$res[]=$row;
	}

	for($i=0;$i<count($res);$i++){
		//search + add depends to file
		$result=db_query("SELECT archives.springname as springname, depends_string
			FROM {springdata_depends} AS depends
			LEFT JOIN {springdata_archives} AS archives ON depends.depends=archives.fid
			WHERE depends.fid=%d",$res[$i]['fid']);
		while($row = db_fetch_array($result)){
			if(!is_array($res[$i]['depends']))
				$res[$i]['depends']=array();
			if(strlen($row['springname'])<=0) // 0 means it wasn't resolveable, return the string
				$res[$i]['depends'][]=$row['depends_string'];
			else //is resolveable, return string from archive
				$res[$i]['depends'][]=$row['springname'];
		}
		//search + add additional mirrors to file
		$result=db_query('SELECT CONCAT(mirror.url_prefix,"/",files.path) as url
			FROM file_mirror_files as files
			LEFT JOIN file_mirror as mirror ON files.fmid=mirror.fmid
			WHERE files.fid=%d
			AND mirror.active=1
			AND files.active=1
			ORDER BY files.md5check',array($res[$i]['fid']));
		while($row = db_fetch_array($result)){
			$res[$i]['mirrors'][]=$row['url'];
		}
		if(isset($req['torrent']))
			$res[$i]['torrent']=_file_mirror_gettorrent($res[$i]['filename']);
		$res[$i]['description']=_file_mirror_getlink($res[$i]['fid']);
		//randomize order of result
		shuffle($res[$i]['mirrors']);
		unset($res[$i]['fid']);
		if ($res[$i]['springname']=="")
			unset($res[$i]['springname']);
	}
	$count=count($res);
	if ($count<>1)
		watchdog("searchapi", "results: ".$count." request: ".$req['category']." ".$req['filename']." ".$req['springname']." ".$req['logical'].file_mirror_backtrace());
	return $res;
}

