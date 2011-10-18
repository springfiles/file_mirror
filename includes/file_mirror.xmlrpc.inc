<?php

function file_mirror_xmlsearch($req){
	return "Moved to http://upq.springfiles.com/xmlrpc.php";
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

function file_mirror_uploadfile($req){
	watchdog("filemirror", "upload request: ".$req['url']);
	$params = array( 'name' => $req['username'],
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
}

