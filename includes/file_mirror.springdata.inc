<?php

/*
	@param xml xml object for file
	@param fid file id of added file
*/
function _springdata_add_node($xml, $fid){
	$cat=(string)$xml->Type[0];
	if($cat=="Game")
		$cat_sel="type='category' and title='Games'";
	else if ($cat=="Map")
		$cat_sel="type='category' and title='Spring Maps'";
	else{
		watchdog("file_mirror", "category not detected".(string)$xml->Type[0]);
		return;
	}
	$res=db_query("SELECT nid FROM {node} WHERE ".$cat_sel);
	$cat=intval(db_result($res));
	if ($cat<=0){
		watchdog("file_mirror", "category not found $cat");
		return;
	}
	$file=field_file_load($fid);
	if (!is_array($file)){
		watchdog("file_mirror", "couldn't load file $fid");
		return;
	}


	$res=db_query("SELECT nid FROM {content_field_file} WHERE field_file_fid=%d",array($fid));
	$nid=intval(db_result($res));
	if($nid>0){
		watchdog("file_mirror", "node already exists: $nid");
		return;
	}
	$node = new stdClass();
	$node->name = (string)$xml->Name[0];
	$node->title = (string)$xml->Name[0];
	$node->type = 'file';
	$node->created = time();
	$node->field_category[0]['nid']=$cat;
	$node->body = (string)$xml->Description[0];
	$node->field_version[0]['value']=(string)$xml->Version[0];
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
}

function file_mirror_import_xml(){
	//path where files from spring persists
	$springfiles="sites/default/files/downloads/spring/"; //FIXME: hardcoded filepath
	//fetch categories into array
	$result=db_query("SELECT cid, name FROM {springdata_categories}");
	$cat=array();
	while($row=db_fetch_array($result)){
		$cat[$row['name']]=$row['cid'];
	}
	$result=db_query("SELECT files.fid as fid,filepath as filepath FROM {files}
LEFT JOIN {springdata_archives} AS archives ON archives.fid=files.fid
WHERE archives.name IS NULL
AND LEFT(files.filepath,%d)='%s'",array(strlen($springfiles),$springfiles));

	while($row=db_fetch_array($result)){
		$xmlfile=file_directory_path()."/springdata/".basename($row['filepath'].'.metadata.xml.gz');
		if (!file_exists($xmlfile)){
			continue;
		}
		
		$f=gzopen($xmlfile);
		if($f===FALSE){
			watchdog("springdata", "Couldn't open $xmlfile", array(),WATCHDOG_WARNING);
			continue;
		}
		$string="";
		while (!gzeof($gz)) { //uncompress gziped file
			$string+=fread($f,4096);
		}
		$xml=@simplexml_load_string($string); //load uncompressed string
		unset($string);
		if(!$xml){
			watchdog("springdata", "Invalid xml file $xmlfile", array(),WATCHDOG_WARNING);
			continue;
		}
		$catid=0;
		if ($xml->Type[0]=='Game'){
			$catid=$cat['games'];
		}else if ($xml->Type[0]=='Map'){
			$catid=$cat['maps'];
		}
		if($catid<=0){
			watchdog("springdata", "Invalid catid: $catid read from $xmlfile cat :" . $xml->Type[0].$cat['games'].$cat['maps']);
			continue;
		}
		_springdata_add_node($xml, $row['fid']);
	}
}

