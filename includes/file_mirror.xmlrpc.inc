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
	if (is_array($data)){
		foreach($data as $val)
			array_push($vars, $val);
	}else
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
	_file_mirror_createquery($query,$vars, $logical,"CONCAT(a.name,' ',a.version) LIKE BINARY '%s' OR a.name LIKE BINARY '%s'", array($req['springname'],$req['springname']));

	if($query!="")
		$query=" AND (".$query.")";
	$result=db_query("SELECT
		f.fid as fid,
		f.filename as filename,
		f.filepath as filepath,
		h.md5 as md5,
		a.name as name,
		a.version as version,
		LOWER(c.name) as category,
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
		$result=db_query("SELECT CONCAT(archives.name,' ',archives.version) as springname, depends_string
			FROM {springdata_depends} AS depends
			LEFT JOIN {springdata_archives} AS archives ON depends.depends=archives.fid
			WHERE depends.fid=%d",$res[$i]['fid']);
		while($row = db_fetch_array($result)){
			if(!is_array($res[$i]['depends']))
				$res[$i]['depends']=array();
			if(strlen($row['springname'])<=0) // 0 means it wasn't resolveable, return the string
				$res[$i]['depends'][]=$row['depends_string'];
			else{ //is resolveable, return string from archive
				$res[$i]['depends'][]=$row['springname'];
			}
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
		if(array_key_exists('torrent', $req))
			$res[$i]['torrent']=_file_mirror_gettorrent($res[$i]['filename']);
		$res[$i]['description']=_file_mirror_getlink($res[$i]['fid']);
		//randomize order of result
		if (count($res[$i]['mirrors'])>2) //remove main mirror to reduce load if enough alternatives are avaiable
			array_shift($res[$i]['mirrors']);
		shuffle($res[$i]['mirrors']);
		unset($res[$i]['fid']);
		if ($res[$i]['version']=="")
			$res[$i]['springname']=$res[$i]['name'];
		else
			$res[$i]['springname']=$res[$i]['name']." ".$res[$i]['version'];
	}
	$count=count($res);
	if ($count<>1)
		watchdog("searchapi", "results: ".$count." request: ".$req['category']." ".$req['filename']." ".$req['springname']." ".$req['logical'].file_mirror_backtrace());
	return $res;
}

function file_mirror_uploadfile($req){
	watchdog("filemirror", "upload request: ".$req['url']);
/*	$params = array( 'name' => $req['username'],
			'pass' => $req['password']);
	$user=user_authenticate($params);
	if (!is_object($user))
		return "Invalid username/password";
	$filename=$req['filename'];
	$sdp=$req['sdp']; # optional
	$tag=$req['tag']; # optional
	$url=$req['url'];
	$uid=$user->uid;
	$res=_file_mirror_run_upq("download url:$url sdp:$sdp filename:$filename uid:$uid");

	return $res;
*/
	return "";
}

/*
	@param xml xml object for file
	@param fid file id of added file
*/
function _springdata_add_node($fid, $name,$version, $cat, $description){
	if($cat=="Game")
		$cat_sel="type='category' and title='Games'";
	else if ($cat=="Map")
		$cat_sel="type='category' and title='Spring Maps'";
	else{
		return "category not detected $cat";
	}
	$res=db_query("SELECT nid FROM {node} WHERE ".$cat_sel);
	$cat=intval(db_result($res));
	if ($cat<=0){
		return "category not found $cat";
	}
	$file=field_file_load($fid);
	if (!is_array($file)){
		return "couldn't load file $fid";
	}
	$res=db_query("SELECT nid FROM {content_field_file} WHERE field_file_fid=%d",array($fid));
	$nid=intval(db_result($res));
	if($nid>0){
		return "node already exists: $nid";
	}
	$node = new stdClass();
	$node->name = $title;
	$node->title = $title;
	$node->type = 'file';
	$node->created = time();
	$node->field_category[0]['nid']=$cat;
	$node->body = $description;
	$node->field_version[0]['value']=$version;
	$node->field_file[0]=$file;
	$node->language='en';
	if ($cat=="Game"){ //add game specific data
	}else if ($cat=="Map"){ //add map specific data
		//add map size
		$node->field_mapsizex[0]['value']=$xml->Width;
		$node->field_mapsizey[0]['value']=$xml->Height;
//		$node->field_image[0]['value']=$xml->Height;
	}
	node_save($node);
	return "success";
}

function file_mirror_createnode($req){
        $params = array( 'name' => $req['username'],
                        'pass' => $req['password']);
        $user=user_authenticate($params);
        if (!is_object($user))
                return "Invalid username/password";
	$fid=intval($req['fid']);
	$name=$req['name'];
	$version=$req['version'];
	$cat=$req['cat'];
	$description=$req['description'];
	return _springdata_add_node($fid, $name,$version, $cat, $description);
}


/*
allow request by:

	fid
	greater than fid
	springname

	returns: array of:
	<array>
		<struct>
			<fid>5</fid>
			<xml>
			<minimap>
			<mirror>

	</array>
*/
function file_mirror_fileinfo($req){
	$fid=intval($req['fid']);
	if(array_key_exists($req, "list"))
		$cmp='>';
	else
		$cmp='=';
	$springname=$req['springname'];
	$vars=array();
	$cond="";
	if ($fid>0){
		$vars[]=$fid;
		$cond="fid$cmd%d";
	}else if (strlen($springname)>0){
		$vars[]=$springname;
		$cond="springname='%s'";
	}else{
		return array();
	}
	$result=db_query("SELECT * FROM {files} f LEFT JOIN {file_mirror} m ON f.fmid=m.fmid WHERE $cond LIMIT 0,50", $vars);
	$res=array();
        while($row = db_fetch_array($result)){
		$res[]=$row;
	}
	return $res;
}

