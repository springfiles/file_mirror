<?php

function file_mirror_check_md5($amount=10){
	$q = db_query("SELECT * FROM {file_mirror_files} f  
		LEFT JOIN {file_mirror} m ON f.fmid = m.fmid 
		LEFT JOIN {filehash} h ON f.fid = h.fid
		WHERE (f.md5check IS NULL OR (f.md5check < NOW() - INTERVAL 1 YEAR))
		AND m.url_deamon IS NOT NULL 
		AND h.md5 IS NOT NULL
		limit 0, %d", array($amount));
	$valid=0;
	$deleted=0;
	$count=0;
	while ($query = db_fetch_object($q)){
		$checklist[] = $query;
		$url=$query->url_prefix.'/'.urlencode($query->url_deamon).'?p='.urlencode($query->path).'&h='.$query->md5;
		$md5 = file_get_contents($url);
		if ($md5===false){
			watchdog('filemirror', $url." returned '".$md5."' is deamon.php missing/defect on mirror?");
			break;
		}
		if ($md5 == $query->md5){
			//md5sum valid
			db_query("UPDATE {file_mirror_files} SET md5check = NOW(), active = 1, changed = NOW() WHERE fmfid = %d", array($query->fmfid));
			$valid += 1;
		}else {
			watchdog('filemirror', "Invalid file ".$query->path." md5:  ".$md5." on site: ".$query->url_prefix);
			db_query("DELETE FROM {file_mirror_files} WHERE fmfid = %d", array($query->fmfid));
// TODO:set status -> delete files with status 2 by ftp + remove then database entry
//		db_query("UPDATE FROM {file_mirror_files} WHERE fmfid = %d SET (status) VALUES(2)");
			$deleted +=1;
		}
		$count++;
	}
	watchdog('filemirror', $valid."/".$count.' validated,  md5s, '. $deleted.' deleted from db as mirror because of md5 mismatch');
}

