<?php
//file handling functions on local server


function file_mirror_listdir($dir='.') {
    if (!is_dir($dir)) {
        return false;
    }
   
    $files = array();
    file_mirror_listdiraux($dir, $files);
    
    return $files;
}

function file_mirror_listdiraux($dir, &$files) {
    $handle = opendir($dir);
    while (($file = readdir($handle)) !== false) {
        if ($file == '.' || $file == '..') {
            continue;
        }
        $filepath = $dir == '.' ? $file : $dir . '/' . $file;
        if (is_link($filepath))
            continue;
        if (is_file($filepath)) {
            $file_['filename'] = basename($filepath);
            $file_['filesize'] = file_mirror_get_filesize($filepath);
            $file_['filepath'] = substr($filepath, 0, 0 - strlen($file_['filename']));
            $files[]= array('filename' => $file_['filename'],'filesize' => $file_['filesize'],'filepath' => $file_['filepath']);
            $i++;
        }
        else if (is_dir($filepath))
            file_mirror_listdiraux($filepath, $files);
    }
    closedir($handle);
}

function file_mirror_get_filesize($file) {
        $size = filesize($file);
        if ($size < 0)
            if (!(strtoupper(substr(PHP_OS, 0, 3)) == 'WIN'))
                $size = trim(`stat -c%s $file`);
            else{
                $fsobj = new COM("Scripting.FileSystemObject");
                $f = $fsobj->GetFile($file);
                $size = $file->Size;
            }
        return $size;
    }
 
//check if a file at given url exists
function file_mirror_url_exists($url) {
    // Version 4.x supported
    $handle   = curl_init(str_replace(" ", "%20",$url));
    if (false === $handle)
    {
        return false;
    }
    curl_setopt($handle, CURLOPT_HEADER, false);
    curl_setopt($handle, CURLOPT_FAILONERROR, true);  // this works
    curl_setopt($handle, CURLOPT_HTTPHEADER, Array("User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.15) Gecko/20080623 Firefox/2.0.0.15") ); // request as if Firefox   
    curl_setopt($handle, CURLOPT_NOBODY, true);
    curl_setopt($handle, CURLOPT_RETURNTRANSFER, false);
    curl_setopt($handle, CURLOPT_TIMEOUT,1);
    $connectable = curl_exec($handle);
    curl_close($handle);
    return $connectable;
}

function file_mirror_check_dumb($amount = 10)
{
    $number_added = 0;
    $last = variable_get("last_dumb_file",0);
    //select all dumb file mirrors
    $q = db_query("SELECT * FROM {file_mirror} WHERE dynamic = 0 AND active = 1");
    while ($file_mirror = db_fetch_object($q))
    {
        //select 10 files from file_mirror_files
        $x = db_query("SELECT * FROM {files} f 
                      WHERE f.fid > %d ORDER BY f.fid LIMIT 0,%d", array($last, $amount));
        while ($files = db_fetch_object($x))
        {
            $count = db_fetch_object(db_query("SELECT count(fid) as c FROM file_mirror_files2
                                              WHERE fid = %d and fmid = %d", array($files->fid, $file_mirror->fmid)));
            if ($count->c == 0)
            {
                $dirs = explode(";", $file_mirror->description);
                foreach ($dirs as $dir)
                {
                    if ($found == false)
                    {
                        if (file_mirror_url_exists($file_mirror->url_prefix.'/'.$dir.'/'.$files->filename))
                        {
                            $number_added += 1;
                            db_query("INSERT INTO {file_mirror_files2} (fmid,fid, path, changed) VALUES(%d, %d, '%s', NOW())",array($file_mirror->fmid, $files->fid, $dir.'/'.$files->filename));
                            $found = true;
                        }
                    }
                }
            }
            $found = false;
            $last = $files->fid;
        }
    }
    dsm($number_added." mirror files added to the dumb mirror table by cron");
    variable_set("last_dumb_file",$last);
    return "gf";
}

function file_mirror_recheck_dumb()
{
   variable_del("last_dumb_file");
   return "All files get rechecked at dumb mirrors";
}