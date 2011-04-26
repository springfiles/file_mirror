<?php

function _array_tostr($arr){
	$res="";
	foreach ($arr as $key)
		if ($res=="")
			$res=$key;
		else
			$res.=" ,".$key;
	return $res;
}

/**
	xml-rpc interface to notify springfiles.com about an new file
	required parameters:
	username
	password
	category = games | maps
	filename
	url
*/
function file_mirror_uploadfile($req){
	$extensions = array("sdz", "sd7");

	if (!isset($req['username']))
		return "No username";
	if (!isset($req['password']))
		return "No password";
	$res=user_authenticate(array( "name" => $req['username'], "pass" => $req['password']));
	if (!is_object($res))
		return "ERROR: Authentication failed";
	$userid=$res->uid;
	if (!(valid_url($req['url']) && (substr($req['url'],0,7)=="http://")))
		return "ERROR: Invalid url specified";
	$url=$req['url'];
	if (!isset($req['filename']))
		return "No filename specified";
	$filename = preg_replace("/[^0-9a-z.\-_]/i", '_', strtolower($req['filename']));
	$pathinfo=pathinfo($filename);
	if (!in_array($pathinfo['extension'], $extensions))
		return "Invalid file extension '.".$pathinfo['extension']."' for $filename, allowed extensions: ". _array_tostr($extensions);

	if (!isset($req['category']))
		return "ERROR: No category specified";

	$res=db_query("SELECT COUNT(fid) from {files} WHERE filename='%s' OR origname='%s'",array($filename,$filename));
	$count=db_result($res);
	if ($count>0)
		return "ERROR: File $filename already exists";
	$res=db_query("SELECT cid from {springdata_categories} WHERE name='%s'",array($req['category']));
	$category=db_result($res);
	if ($category===false)
		return "ERROR: Invalid category ".$category;

	//all input is valid, insert it into db
	$res=db_query("INSERT INTO {file_mirror_upload} (uid, cid, filename, url) VALUES ( %d, %d, '%s', '%s')", array($userid, $category, $filename, $url));
	if ($res==false)
		return "ERROR: already queued";
	else
		return "OK: queued for download";
}
/** copy a file (either local file or a remote file, for example http)
*/
function _remote_file_copy($url, $tmpfile, $localfile){
	if (file_exists($localfile)){
		watchdog("file_mirror", "local file $localfile already exists!");
		return false;
	}
	$file=fopen($tmpfile,"wb");
	$rfile=fopen($res['url'],"rb");
	if ($rfile===false)
		return false;
	while(!feof($rfile)){
		$data=fread($rfile, 4096);
		if (fwrite($tmp, $data)===false){
			fclose($tmp);
			fclose($rfile);
			unlink($tmpfile);
			return false;
		}
	}
	fclose($file);
	fclose($rfile);
	return rename($tmpfile, $localfile);
}
/**
* returns path for given category
*/
function _get_file_path($category){
	if ($category == "games")
		return MIRROR_PATH."/games";
	if ($category == "maps")
		return MIRROR_PATH."/games";
	return false;
}

/**
* download all files that are known in the table file_mirror_upload + make them known to drupal
*/
function file_mirror_cronupload(){
	$res=db_query("SELECT * FROM {file_mirror_upload} up
		LEFT JOIN {springdata_categories} as cat ON cat.cid=up.cid");
	while($row=db_fetch_array($res)){
		$path=_get_file_path($row['category']);
		if ($path===false){
			watchdog("file_mirror", "Unknown category, couldn't get path for upload");
			continue;
		}
		$absname=$path.'/'.$row['filename'];
		if (_remote_file_copy($row['url'], $absname)===false){
			watchdog("file_mirror", "Couldn't download file ".$row['url']." to ".$absname);
			continue;
		}
		if (!file_copy($absfile, $path.'/.'.$row['filename'].".partial" , $absfile, FILE_EXISTS_ERROR)) //make file known to drupal
			continue;
		db_query("DELETE FROM {file_mirror_upload} WHERE uploadid=%d", array($row['uploadid']));
		_create_touch_file($absfile);
	}
}
