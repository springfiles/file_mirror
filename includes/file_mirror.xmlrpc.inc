<?php

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

function _file_mirror_xmlsearch($req){
	return "Moved to http://upq.springfiles.com/xmlrpc.php";
}

/*
	@param xml xml object for file
	@param sid sync id of the file
	@return nid of the created node, <0 on error
*/
function _springdata_add_node($sid, $name,$version, $cat, $description){
	if($cat=="Game")
		$cat_sel="type='category' and title='Games'";
	else if ($cat=="Map")
		$cat_sel="type='category' and title='Spring Maps'";
	else{
		watchdog("file_mirror","category not detected $cat");
		return -1;
	}
	$res=db_query("SELECT nid FROM {node} WHERE ".$cat_sel);
	$cat=intval(db_result($res));
	if ($cat<=0){
		watchdog("file_mirror","category $cat not found in db");
		return -1;
	}
	$file=field_file_load($fid);
	if (!is_array($file)){
		watchdog("file_mirror","couldn't load file $fid");
		return -1;
	}
	$res=db_query("SELECT nid FROM {content_field_file} WHERE field_file_fid=%d",array($fid));
	$nid=intval(db_result($res));
	if($nid>0){
		watchdog("file_mirror", "node already exists: $nid");
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
	return $node->nid;
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
function _file_mirror_fileinfo($req){
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

function _file_mirror_getlastsyncid($username, $password){
	$params = array( 'name' => $username,
			'pass' => $password);
	$user=user_authenticate($params);
	if (!is_object($user))
		return "Invalid username/password";
	//FIXME: implement me
	return 0;
}

function _file_mirror_sync($username, $password, $req){
	$params = array( 'name' => $username,
			'pass' => $password);
	$user=user_authenticate($params);
	if (!is_object($user))
		return "Invalid username/password";

	//FIXME: implement me
	//$nid=_springdata_add_node
	//insert into file_mirror_nodes or delete
	if(!is_array($req))
		return "invalid request, excepted an array";
	$res=array();
	$i=0;
	foreach($req as $r) {
		if (!array_key_exists("command", $r))
			return "Key command doesn't exist";
		if (!array_key_exists("fid", $r))
			return "Key fid doesn't exist";
		$res[$i]['fid']=$r['fid'];
		if ($r['command']=="delete"){
			//delete
			$res[$i]['result']="deleted";
		}else if ($r['command']=="update"){
			//$node=_springdata_add_node();
			$node="implement me";
			$res[$i]['result']="updated";
			$res[$i]['url']="http://springfiles.com/node/$node";
		}else{
			$res[]['result']="Invalid command";
		}
		//TODO: update/set last sid here
		$i++;
	}
	return $res;
}
