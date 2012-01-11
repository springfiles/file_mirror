<?php

function _file_mirror_getlink($nid){
	global $base_url;
	if ($nid<=0)
		return "";
	$res=db_query("SELECT dst FROM {url_alias} WHERE src='%s'",array("node/$nid"));
	$url=db_result($res);
	if (strlen($url)>0)
		return "$base_url/$url";
	return "$base_url/node/$nid";
}

/**
*	implementation of the xml-rpc call
*/

function _file_mirror_xmlsearch($req){
	return "Moved to http://api.springfiles.com/xmlrpc.php";
}

/*
	@param xml xml object for file
	@param sid sync id of the file
	@return nid of the created node, <0 on error
*/
function _springdata_add_node($nodeid, $user, $metadata){
	$cat = $metadata['category'];
	if($cat=="Game")
		$cat_sel="type='category' and title='Games'";
	else if ($cat=="Map")
		$cat_sel="type='category' and title='Spring Maps'";
	else{
		watchdog("file_mirror","category not detected %cat",array('%cat'=>$cat), WATCHDOG_ERROR);
		return -1;
	}
	$res=db_query("SELECT nid FROM {node} WHERE ".$cat_sel);
	$cat=intval(db_result($res));
	if ($cat<=0){
		watchdog("file_mirror","category %cat not found in db",array('%cat'=>$cat), WATCHDOG_ERROR);
//		return -1;  //FIXME: re-enable
	}
	if($nodeid>0){
		$node=node_load($nodeid);
		if ($node===FALSE){
			watchdog("file_mirror", "loading node failed, creating new one", WATCHDOG_WARNING);
			$node = new stdClass();
		}else {
			watchdog("file_mirror", "loaded old node %nid", array('%nid'=>$node->nid), WATCHDOG_DEBUG);
			$node->nid=$nodeid;
		}
	}else{
		$node = new stdClass();
		watchdog("file_mirror","Creating new node",array(), WATCHDOG_DEBUG);
	}
	$node->uid = $user->uid;
	$node->name = $metadata['name']." ".$metadata['version'];
	$node->title = $node->name;
	$node->type = 'file';
	$timestamp=$metadata['timestamp'];
	$node->created = mktime($timestamp->hour,$timestamp->minute,$timestamp->second,$timestamp->month,$timestamp->day,$timestamp->year);
	$node->field_category[0]['nid']=$cat;
	$node->body = $metadata['Description'];
	$node->field_version[0]['value']=$metadata['version'];
	$node->field_file[0]=$file;
	$node->language='en';
	if ($cat=="Game"){ //add game specific data
	}else if ($cat=="Map"){ //add map specific data
		//add map size
		$node->field_mapsizex[0]['value']=$metadata['Width'];
		$node->field_mapsizey[0]['value']=$metadata['Height'];
//		$node->field_image[0]['value']=$xml->Height;
	}
	node_save($node);
	watchdog("file_mirror", "%nid %data", array('%nid'=>$node->nid, '%data'=>print_r($node, true)), WATCHDOG_DEBUG);
	return $node->nid;
}

function _file_mirror_getlastsyncid($username, $password){
	$params = array( 'name' => $username,
			'pass' => $password);
	$user=user_authenticate($params);
	if (!is_object($user))
		return "Invalid username/password";
	return intval(variable_get("file_mirror_lastsyncid"));
}

function _file_mirror_sync($username, $password, $req){
	$params = array( 'name' => $username,
			'pass' => $password);
	$user=user_authenticate($params);
	if (!is_object($user)){
		$err="Invalid username/password";
		watchdog("file_mirror", $err,array(), WATCHDOG_ERROR);
		return $err;
	}
	if(!is_array($req)){
		$err="invalid request, excepted an array";
		watchdog("file_mirror", $err,array(), WATCHDOG_ERROR);
		return $err;
	}
	watchdog("file_mirror", "got %req requests",array('%req'=>sizeof($req)), WATCHDOG_DEBUG);
	$res=array();
	$i=0;
	foreach($req as $r) {
		if (!array_key_exists("command", $r)){
			$err="Key command doesn't exist";
			watchdog("file_mirror", $err,array(), WATCHDOG_ERROR);
			return $err;
		}
		if (!array_key_exists("fid", $r)){
			$err="Key fid doesn't exist";
			watchdog("file_mirror", $err, array(), WATCHDOG_ERROR);
			return $err;
		}
		$res[$i]['fid']=$r['fid']; //fid is the remote fid, not the fid locally!!!! it is used as "sid" here
		if ($r['command']=="delete"){
			$fileres=db_query("SELECT nid FROM {file_mirror_files} WHERE sid=%d", array($r['fid']));
			$nodeid=intval(db_result($fileres));
			if ($nodeid>0){
				node_delete($nodeid);
				db_query("DELETE FROM {file_mirror_files} WHERE sid=%d", array($r['fid']));
				$res[$i]['result']="deleted $nodeid";
			}else{
				$res[$i]['result']="nodeid $nodeid not found for sid ".$r['fid'];
			}
		}else if ($r['command']=="update"){
			$fileres=db_query("SELECT nid FROM {file_mirror_files} WHERE sid=%d", array($r['fid']));
			$nodeid=intval(db_result($fileres));
			watchdog("file_mirror", "nid: %nid  sid: %sid", array('%nid'=>$nodeid, '%sid'=>$r['fid']), WATCHDOG_DEBUG);
			$newnodeid=_springdata_add_node($nodeid,$user, $r['metadata']);
			$res[$i]['result']="updated";
			if ($nodeid>0){ //node already exists
				db_query("UPDATE {file_mirror_files} SET metadata='%s', nid=%d WHERE sid=%d", array(json_encode($r['metadata']), $newnodeid, $r['fid']));
			}else if ($newnodeid>0){ //new node created / old one updated
				db_query("INSERT INTO {file_mirror_files} (sid, nid, metadata) VALUES (%d, %d,'%s')", array($r['fid'],$newnodeid, json_encode($r['metadata'])));
				$nodeid=$newnodeid;
			} else {
				watchdog("file_mirror", "error creating node",array(), WATCHDOG_ERROR);
			}
			$res[$i]['url']=_file_mirror_getlink($nodeid);
		}else{
			watchdog("file_mirror", "invalid command %command", array('%command'=>$r['command']), WATCHDOG_ERROR);
			$res[]['result']="Invalid command";
		}
		variable_set("file_mirror_lastsyncid", $r['sid'] );
		$i++;
	}
	//logout (clean sessions table)
	sess_destroy_uid($user->uid);
	return $res;
}
