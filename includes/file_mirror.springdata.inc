<?php


function _springdata_import_archive($fid,$xml,$catid){
	for($i=0;$i<count($xml->Depends[0]);$i++){
		$res=db_query("SELECT fid FROM {springdata_archives} WHERE springname='%s'",array($xml->Depends[0]->Depend[$i]));
		$row=db_fetch_array($res);
		$depends=intval($row['fid']);
		if ($depends>0){ //insert with id
			db_query("INSERT INTO {springdata_depends} (fid, depends, depends_string) VALUES (%d,%d,'%s')",array($fid, $depends, $xml->Depends[0]->Depend[$i]));
		}else{ //id not found, insert raw string
			db_query("INSERT INTO {springdata_depends} (fid, depends_string) VALUES (%d,'%s')",array($fid, $xml->Depends[0]->Depend[$i]));
		}
	}
	if ($xml->getName=='Map'){
		for($i=0;$i<count($xml->Positions[0]);$i++){
			db_query("INSERT INTO {springdata_startpos} (fid, id, x, z) VALUES (%d,%d, %d, %d)",array($fid, $i, $xml->Positions[0]->StartPos[$i]->X, $xml->Positions[0]->StartPos[$i]->Z));
		}
	}
	if (($fid>0) && ($catid>0)){ //don't insert invalid data
		//now insert the archive as last, if this commit is missing, all steps will be repeated on next run for this fid
		db_query("INSERT INTO {springdata_archives} (fid,springname, cid) VALUES (%d,'%s', %d)",array($row['fid'],$xml->Name[0], $catid));
	}else{
		watchdog("filemirror", "invalid data to insert:". $row['fid'],$xml->Name[0], $catid);
	}
	return true;
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
WHERE archives.springname IS NULL
AND LEFT(files.filepath,%d)='%s'",array(strlen($springfiles),$springfiles));

	while($row=db_fetch_array($result)){
		$xmlfile=file_directory_path()."/springdata/".basename($row['filepath'].'.metadata.xml');
		if (!file_exists($xmlfile)){
			continue;
		}
		$xml = @simplexml_load_file($xmlfile);
		if(!$xml){
			watchdog("springdata", "Invalid xml file $xmlfile", array(),WATCHDOG_WARNING);
			continue;
		}
		$catid=0;
		if ($xml->getName()=='Game'){
			$catid=$cat['games'];
		}else if ($xml->getName()=='Map'){
			$catid=$cat['maps'];
		}
		if($catid<=0){
			watchdog("springdata", "Invalid catid: $catid read from $xmlfile cat :" . $xml->getName().$cat['games'].$cat['maps']);
			continue;
		}
		_springdata_import_archive($row['fid'],$xml,$catid);
	}
}

