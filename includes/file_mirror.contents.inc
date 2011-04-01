<?php

function file_mirror_check_contents($file_mirror){
	global $connection;
	return "file_mirror_check_contents disabled";
    $match = file_mirror_check_filelist($file_mirror, 'intersect');
    $output .= t('Known files found on mirror: ').count($match);
    $output .= '<br />'.t('Unknown files found on mirror: ').count(file_mirror_check_filelist($file_mirror, 'diff'));
    if(count(file_mirror_check_filelist($file_mirror, 'diff'))) $output .= ' '.l('Delete unknown files','admin/settings/file-mirror/'.$file_mirror->fmid.'/clean');  
    $output .= '<br />'.t('Indexed files found on mirror: ').count(file_mirror_check_index($match));
    $output .= '<br />'.t('Unindexed files found on mirror: ').count(file_mirror_array_diff($match, file_mirror_check_index($match)));
    if(count(file_mirror_array_diff($match, file_mirror_check_index($match)))) $output .= ' '.l('Index found files','admin/settings/file-mirror/'.$file_mirror->fmid.'/index');
    //faulty indexes
    $output .= '<br />'.t('Faulty indexes found: ').count(file_mirror_check_faultyIndex($file_mirror));
    if(count(file_mirror_check_faultyIndex($file_mirror))) $output .= ' '.l('Clean faulty indexes','admin/settings/file-mirror/'.$file_mirror->fmid.'/cleanindex');
    $output .= '<br />'.t('Missing files on mirror: ').count(file_mirror_check_filelist($file_mirror, 'diff2'));
    return $output;

}

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

function file_mirror_clean_index_page($file_mirror)
{
    foreach (file_mirror_check_faultyIndex($file_mirror) as $faulty)
    {
       db_query ("DELETE FROM {file_mirror_files} WHERE fmfid = %d", $faulty['fmfid']);
    }
    //also reindex the mirror (in case of moved files)
    file_mirror_index_files_page($file_mirror);
}

function file_mirror_index_files_page($file_mirror)
{
    $match = file_mirror_check_filelist($file_mirror, 'intersect');
    file_mirror_index_files($file_mirror, $match);
    drupal_goto($path = '/admin/settings/file-mirror/'.$file_mirror->fmid.'/check-contents');
}

function file_mirror_index_files($file_mirror, $files)
{
    $i = 0;
    foreach ($files as $value)
    {
       $path = ltrim(str_replace($file_mirror->ftp_dir, "", $value['filepath'].$value['filename']),'/');
       $count = db_fetch_object(db_query("SELECT COUNT(fmfid) as count FROM {file_mirror_files} WHERE fmid = %d AND path = '%s'", array($file_mirror->fmid, $path)));
       //when current file isnt indexed yet, check if it exists in the drupal files table
       if($count->count == 0)
       {
            $fid = db_fetch_object(db_query("SELECT fid FROM {files} WHERE filename = '%s' AND filesize = '%s'", array($value['filename'], $value['filesize'])));
            //file is also found in the drupal files table, we can add it to our mirror files table
            if ($fid->fid)
            {
               $record->fmid = $file_mirror->fmid;
               $record->fid = $fid->fid;
               $record->path = $path;
               $record->active = 0;
               $changed->changed = date("Y-m-d H:i:s");
               drupal_write_record('file_mirror_files',$record);
               $i++;
            }
       }
    }
    drupal_set_message(t('Files indexed: ').$i);
    
}

function file_mirror_check_delete_unknown($file_mirror) {
    
    $i=0;

    $files = file_mirror_check_filelist($file_mirror, 'diff');
    if ($files) {
        $remote = file_mirror_ftp_connect($file_mirror, true);
        foreach ($files as $unknown)
        {
            if (file_mirror_ftp_delete_file($unknown['filepath'].$unknown['filename'], $remote['ftp_connection'], true)) $i++;   
            
        }
    }
    //reindex the mirror
    $match = file_mirror_check_filelist($file_mirror, 'intersect');
    file_mirror_index_files($file_mirror, $match);
    //refresh the filelist
    $remote = file_mirror_check_filelist($file_mirror);
    //get the sum of the filesizes and save it to db
    file_mirror_used_diskspace(file_mirror_flatten_remote_array($remote), $file_mirror);
    drupal_set_message (t('Unknown files found on mirror: ').count($files).' ('.$i.' removed)');
    return file_mirror_check_contents($file_mirror);
}

/**
 * compares the local and remote list of files and merges the arrays with $mode
 * @param file_mirror object file_mirror
 * @param mode= "intersect"|"diff"|"diff2"
 *  @return array with list of files
 */
function file_mirror_check_filelist($file_mirror, $mode = null){
      //connect and get filelist
    $remote =file_mirror_ftp_connect($file_mirror, true);
    $remote = $remote['ftp_files'];
    $local = array();
    $q = db_query("SELECT * from {file_mirror_paths} WHERE fmid = %d", $file_mirror->fmid);
    while ($settings = db_fetch_object($q))
    {
        $local = array_merge($local, file_mirror_local_files(file_directory_path().'/'.$settings->path));
    }
    
    if (count($local) == 0) {
        $q = db_query("SELECT * from {file_mirror_paths} WHERE fmid IS NULL OR fmid = 0");
        while ($settings = db_fetch_object($q))
        {
          $local = array_merge($local, file_mirror_local_files(file_directory_path().'/'.$settings->path));
        }
    }
    
    //if no (default) paths are set, add all drupal files to $local
    if (count($local) == 0) $local = file_mirror_local_files(file_directory_path());
    if ($mode == 'intersect') $return = file_mirror_array_intersect($remote, $local);
    if ($mode == 'diff') $return = file_mirror_array_diff(file_mirror_flatten_remote_array($remote), $local); 
    if ($mode == 'diff2') $return = file_mirror_array_missing($remote, $local);
    if (! $mode) $return = $remote;
    return $return;  
}

function file_mirror_local_files($dir) {
     $local = array();
     $query = db_query("SELECT fid, filename, filesize, filepath from {files} WHERE filepath LIKE '%s%' ORDER BY fid DESC", $dir);
     while ($output = db_fetch_object($query))
      {
            $path = pathinfo($output->filepath);
            $local[] = array('fid' => $output->fid, 'filename' => $output->filename, 'filesize' => $output->filesize, 'filepath' => $path['dirname'], 'localname' => $output->filepath);
      }
      return $local;
}

function file_mirror_check_index($filelist) {
    $indexed = array();
    
    foreach ($filelist as $value)
    {
        $q = db_fetch_object(db_query("SELECT count({file_mirror_files}.fmfid) as count FROM {file_mirror_files}
                                      INNER JOIN {files} ON {file_mirror_files}.fid = {files}.fid
                                      WHERE {files}.filename = '%s' AND {files}.filesize = %d", $value['filename'], $value['filesize']));
        if ($q->count > 0) $indexed[] = $value;
    }
    return $indexed;
}

function file_mirror_check_faultyIndex($file_mirror) {
    global $connection;
    $i = 0;
    //check if there is already a connection
    if (! $connection['ftp_connection']) file_mirror_ftp_connect($file_mirror, true);
    //end function if there is still no connection
    if (! $connection['ftp_connection']) return false;
    $q = db_query("SELECT {files}.filename, {files}.filesize, {file_mirror_files}.path, {file_mirror_files}.fmfid FROM {file_mirror_files} INNER JOIN {files} on {file_mirror_files}.fid = {files}.fid WHERE fmid = %d", $file_mirror->fmid); 
    while ($query = db_fetch_array($q))
    {
        
        $index[$i]['filesize'] = $query['filesize'];
        $index[$i]['filepath'] = $file_mirror->ftp_dir.'/'.$query['path'];
        $index[$i]['fmfid'] = $query['fmfid'];
        $i++;
    }
    $remote = file_mirror_flatten_remote_array($connection['ftp_files']);
    
    $faulty = file_mirror_array_faulty($remote, $index);
    return $faulty;
}

function file_mirror_array_intersect($remote, $local)
{
    $match = array();
    foreach ($remote as $value)
     {
        if (isset($value['dir']))
        {
            $match = array_merge($match,file_mirror_array_intersect($value['dir'], $local));
        }
        else
        {
            foreach ($local as $value2)
            {
                if ($value2['filename'] == $value['filename'] && $value2['filesize'] == $value['filesize']) {
                    $match[] = array('filename' => $value['filename'], 'filesize' => $value['filesize'], 'filepath' => $value['filepath']);
                }
            }
        }
     }
     return $match;
}

function file_mirror_array_diff($remote, $local) {
    $unknown = array();
    foreach ($remote as $value)
     {
        if (isset($value['dir']))
        {
            $unknown = array_merge($unknown, file_mirror_array_diff($value['dir'], $local));
        }
        else
        {
            $hit = false;
            foreach ($local as $value2)
            {
                if ($value2['filename'] == $value['filename'] && $value2['filesize'] == $value['filesize']) {
                    $hit = true;
                }
            }
            if (! $hit) $unknown[] = $value; 
        }
        
     }
     return $unknown;
}

function file_mirror_array_missing($remote, $local, $hits = array()) {
    $missing = array();
    //flatten remote
    $remote = file_mirror_flatten_remote_array($remote);
    foreach ($local as $value)
     {
            $hit = false;
            foreach ($remote as $value2)
            {
                if ($value2['filename'] == $value['filename'] && $value2['filesize'] == $value['filesize']) {
                        $hit = true;
                }
                
            }
            // not found,add it as missing
            if ($hit == false) $missing[] = $value;
     }
     return $missing;
}

function file_mirror_array_faulty($remote, $local, $hits = array()) {
    $missing = array();
    //flatten remote
    $remote = file_mirror_flatten_remote_array($remote);
    if (count($local) > 0)
    foreach ($local as $value)
     {
            $hit = false;
            foreach ($remote as $value2)
            {
                if ($value2['filesize'] == $value['filesize'] && $value2['filepath'].$value2['filename'] == $value['filepath']) {
                        $hit = true;
                }
                
            }
            // not found,add it as missing
            if ($hit == false) $missing[] = $value;
     }
     return $missing;
}

//remove the dir structure in array
function file_mirror_flatten_remote_array($remote)
{
    $flat = array();
    foreach ($remote as $value)
    {
        if (isset($value['dir']))
        {
                $flat = array_merge($flat, file_mirror_flatten_remote_array($value['dir']));
        }
        else {
            $flat[] = $value;
        }
    }
    return $flat;
}

//get the sum of the filesizes in $files array and save it to db
function file_mirror_used_diskspace($files, $file_mirror)
{
    $space = 0;
    
    foreach ($files as $file)
    {
        $space += $file['filesize'];
        
    }
    //save to mirror table
    db_query("UPDATE {file_mirror SET fill = %d WHERE fmid = %d", array($space, $file_mirror->fmid));
}

//check files for http availability
function file_mirror_http_availability()
{
    $q = db_query("SELECT * FROM {file_mirror_files} mf INNER JOIN {file_mirror} m ON mf.fmid = m.fmid WHERE (mf.active = 0 AND mf.changed IS NULL) OR (mf.changed < NOW() - INTERVAL 7 DAY) LIMIT 0,500");
    while ($query = db_fetch_object($q))
    {
        $result[] = $query;
    }
    if ($result)
    {
    foreach ($result as $m)
        {
            if(file_mirror_url_exists($m->url_prefix.'/'.$m->path))
            {
                db_query("UPDATE {file_mirror_files} SET active = 1, changed = NOW() WHERE fmfid = %d", array($m->fmfid));
            }
        }
    }
}



