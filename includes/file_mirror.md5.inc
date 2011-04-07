<?php

//TODO: add parameter fmid

function file_mirror_check_md5($amount=10){
	$q = db_query("SELECT * FROM {file_mirror_files} f  
		LEFT JOIN {file_mirror} m ON f.fmid = m.fmid 
		LEFT JOIN {filehash} h ON f.fid = h.fid
		WHERE (f.md5check IS NULL OR (f.md5check < NOW() - INTERVAL 1 YEAR))
		AND m.url_deamon IS NOT NULL 
		AND h.md5 IS NOT NULL
		AND m.active=1
		ORDER BY f.changed ASC
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
			//mark all files on this mirror as changed to skip md5check, if other files are waiting for validating
			db_query("UPDATE {file_mirror_files} SET changed = NOW() WHERE fmid = %d AND md5check IS NULL ", array($query->fmid));
			break;
		}
		if ($md5 == $query->md5){
			//md5sum valid
			db_query("UPDATE {file_mirror_files}
					SET md5check = NOW(), active = 1, changed = NOW(), md5='%s'
					WHERE fmfid = %d", 
					array($query->md5,$query->fmfid));
			$valid++;
		}else if (strlen($md5)==32) { //md5 invalid
			watchdog('filemirror', "Invalid file ".$query->path." md5:  ".$md5.", deleted from db on site: ".$query->url_prefix);
			db_query("DELETE FROM {file_mirror_files} WHERE fmfid = %d", array($query->fmfid));
			$deleted++;
		}else{ //file doesn't exist, password/ip error, ...
			watchdog('filemirror', "Invalid result: ".$md5."on site: ".$query->url_prefix);
                        db_query("DELETE FROM {file_mirror_files} WHERE fmfid = %d", array($query->fmfid));
		}
		$count++;
	}
}

