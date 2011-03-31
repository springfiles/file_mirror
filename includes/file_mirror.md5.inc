<?php

//TODO: add parameter fmid

function file_mirror_check_md5($amount=10){
	$q = db_query("SELECT * FROM {file_mirror_files} f  
		LEFT JOIN {file_mirror} m ON f.fmid = m.fmid 
		LEFT JOIN {filehash} h ON f.fid = h.fid
		WHERE (f.md5check IS NULL OR (f.md5check < NOW() - INTERVAL 1 YEAR))
		AND m.url_deamon IS NOT NULL 
		AND h.md5 IS NOT NULL
		ORDER BY CHANGED
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
			db_query("UPDATE {file_mirror_files} SET changed = NOW() WHERE fmfid = %d", array($query->fmfid));
			break;
		}
		if ($md5 == $query->md5){
			//md5sum valid
			db_query("UPDATE {file_mirror_files} SET md5check = NOW(), active = 1, changed = NOW() WHERE fmfid = %d", array($query->fmfid));
			$valid++;
		}else {
			watchdog('filemirror', "Invalid file ".$query->path." md5:  ".$md5.", deleted from db on site: ".$query->url_prefix);
			db_query("DELETE FROM {file_mirror_files} WHERE fmfid = %d", array($query->fmfid));
			$deleted++;
		}
		$count++;
	}
}

