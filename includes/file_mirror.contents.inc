<?php

/**
 * upload files to mirror and add the uploaded files to the mirror file index in db
 */
function file_mirror_upload_contents() {
	$i=0;
	//get all dynamic and active mirrors
	$q = db_query("SELECT * FROM {file_mirror} WHERE dynamic = 1 AND active = 1 ORDER BY lastupload");
	while ($mirror = db_fetch_object($q)){
		$start=time();
		$bytes=0;
		db_query("UPDATE {file_mirror} SET lastupload=NOW() WHERE fmid=%d", array($mirror->fmid));
		$files=get_files_to_upload($mirror->fmid,UPLOADSPERCRON);
		if (count($files)<=0){ //use next mirror if no files are to upload
			watchdog('filemirror',"Is up to date: ".$mirror->title);
			continue; //skip to next mirror
		}
		watchdog('filemirror','Connecting to '.$mirror->title);
		$con=file_mirror_ftp_connect($mirror->ftp_url, $mirror->ftp_port, $mirror->ftp_user, $mirror->ftp_pass, $mirror->ftp_passive, $mirror->ftp_ssl, $mirror->ftp_dir);
		if ($con===false)
			continue;//couldn't connect to ftp, next mirror
		$pos=1;
		foreach ($files as $missing ){
			$dstfile=substr($missing['filepath'],strlen(file_directory_path())+1); //remove local prefix (usally sites/default/files
			$srcfile=$missing['filepath'];
			$filesize=filesize($missing['filepath']);
			$bytes+=$filesize;
			watchdog('filemirror',$pos.'/'.count($files).' '.basename($srcfile)." ".round($filesize/(1024*1024),1)." MB ($srcfile->$dstfile)");
			if (file_mirror_ftp_put($con, $srcfile, $dstfile)){
				//insert into database, set date to 0 -> will be checked immidetly
				db_query("INSERT INTO {file_mirror_files}
					(fmid, fid, active, path, changed, size )
					VALUES(%d, %d, %d, '%s', 0, %d )",
					array($mirror->fmid, $missing['fid'], 1, $dstfile, $filesize));
				//verify file
				file_mirror_check_md5(10);
			}else{
				//upload failed, next mirror
				break;
			}
			$pos++;
		}
		ftp_close($con);
		unset($con);
		$timediff=time()-$start;
		if ($timediff>0)
			watchdog('filemirror', "Transfered ".round($bytes/(1024*1024),1)." MB at ".round($bytes/($timediff*1024),1)."KB/s");
	}
}

/**
 * Searches the files, that are not on the given mirror
 * @param id of the mirror
 * @param maxfiles maximum files to return
 * @returns the files that are not on a mirror, limit to maxfiles
 */

function get_files_to_upload($mirrorid,$maxfiles){
	$paths=array();
	//get mirror paths and get all files uploaded 
	$q=db_query("SELECT path from {file_mirror_paths}
		WHERE fmid = %d",array($mirrorid));
	while($record = db_fetch_array($q)){
		$paths[]=$record['path'];
	}
	if (count($paths)==0){ //no path found, use default one
		$q=db_query("SELECT path from {file_mirror_paths}
				WHERE fmid IS NULL");
		while($record = db_fetch_array($q)){
			$paths[]=$record['path'];
		}
	}
	$res=array();
	foreach($paths as $path){
//		watchdog("filemirror", "$path");
		$q=db_query("SELECT fid, filename, filepath FROM files AS f
			WHERE f.fid not in ( SELECT fid FROM
			file_mirror_files WHERE fmid=%d)
			AND filepath LIKE '%s'
			ORDER BY TIMESTAMP DESC
			LIMIT 0,%d",
				array($mirrorid,$path."%",$maxfiles));
		while ($record = db_fetch_array($q)){
			$res[]=$record;
		}
 	}
	return $res;
}
