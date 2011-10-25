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
function _springdata_add_node($sid, $nodeid, $user, $metadata){
	$cat = $metadata['category'];
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
//		return -1;  //FIXME: re-enable
	}
	if($nodeid>0){
		$node=node_load($nodeid);
		if ($node===FALSE){
			watchdog("file_mirror", "loading node failed, creating new one");
			$node = new stdClass();
		}else {
			watchdog("file_mirror", "loaded old node ".$node->nid);
			$node->nid=$nodeid;
		}
	}else{
		$node = new stdClass();
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
	watchdog("file_mirror", $node->nid.": ".print_r($node, true));
	return $node->nid;
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
			watchdog("file_mirror", "old node id: $nodeid");
			$newnodeid=_springdata_add_node($r['fid'], $nodeid,$user, $r['metadata']);
			$res[$i]['result']="updated";
			if ($nodeid>0){ //node already exists
				db_query("UPDATE {file_mirror_files} SET metadata='%s', nid=%d WHERE sid=%d", array(json_encode($r['metadata']), $newnodeid, $r['fid']));
			}else if ($newnodeid>0){ //new node created / old one updated
				db_query("INSERT INTO {file_mirror_files} (sid, nid, metadata) VALUES (%d, %d,'%s')", array($r['fid'],$newnodeid, json_encode($r['metadata'])));
				$nodeid=$newnodeid;
			} else {
				watchdog("file_mirror", "error creating node");
			}
			$res[$i]['url']=_file_mirror_getlink($nodeid);
		}else{
			$res[]['result']="Invalid command";
		}
		//TODO: update/set last sid here
		$i++;
	}
	return $res;
}
