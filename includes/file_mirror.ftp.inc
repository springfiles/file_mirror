<?php

/*
 * connect to a remote ftp site
 * @param $host host to connect to
 * @param $port
 * @param $username
 * @param $password
 * @param $passive
 * @param $dir root directory to chdir to
 * @return connection handle, false on error
 */

function file_mirror_ftp_connect($host,$port, $username, $password, $passive, $dir){
	if($port==0)
		$port=21;
	if($passive==0)
		$passive=false;
	if(strlen(username)<=0)
		$username="anonymous";
	$timeout=15;
	// build the ftp connection
	$con = ftp_connect($host, $port, $timeout);
	if ($con===false){
		watchdog("filemirror", "Error connecting to $host");
		return false;
	}
	if (!ftp_login($con, $username, $password)){
		watchdog("filemirror", "Error login on $host, password/username wrong?");
		return false;
	}
	if ($passive) {
		if(!ftp_pasv($con, true)){
			watchdog("filemirror", "Error setting passive mode on $host");
			return false;
		}

	}
	if(strlen($dir)>0){
		if(!ftp_chdir($con, $dir)){
			watchdog("filemirror", "Error chdir() to $dir");
			return false;
		}

	}
	return $con;
}

/**
 * Takes an FTP file result and returns an array of data
 * @param $ftp_file_listing
 *   string, the FTP file listing string
 * @param $configuration
 *   array, configuration being run
 * @param $directory
 *   string, directory to harvest from
 * @param $recursive
 *   int, should harvest recursively, set the depth of the search as well
 * @param $strict
 *   boolean, if set, any files currently being uploaded will stop the process
 * @return array
 */
function file_mirror_ftp_get_files(&$connection, $file_mirror, $directory, $recursive  = 25) {
	
	$files = array();
	// get the list of files
	// if we have reached our maxium depth, stop
	if ($recursive === 0) {
		return;
	}
	$ftp_files = file_mirror_ftp_completed_file_list($connection, $directory);
	foreach ($ftp_files as $ftp_file_listing) {
		$ftp_file = array();
		// we need to parse the listing of a file. This listing looks like:
		// -rw-r--r--   1 USER     GROUP     652594 Mar  5 20:33 tnt.wmv.95ccde
		// however it can also look like:
		// -rw-r--r--  1 arthur  staff  9124159 Feb 23  2008 pinback.flv

		// this pattern breaks out each of these items
		$pattern = "/^([0-9dlrwx-]*)[\s]*([0-9]{1})\s*([a-zA-z0-9]*)\s*([a-zA-z0-9]*)\s*([0-9]*)\s*([a-zA-z0-9\s]*\:[0-9]{2}|[a-zA-Z].*)\s(.*)$/";
		if (preg_match($pattern, $ftp_file_listing, $matches)) {
			// do not find files that start with .
			if ($matches[7][0] != '.' && $matches[7] != $file_mirror->url_deamon) {
				// build the file data
                                $ftp_file['filename'] = $matches[7];
				$ftp_file['filesize'] = $matches[5];
				$ftp_file['created'] = strtotime($matches[6]);
				$ftp_file['filepath'] = $directory .'/';
				// check for file token
			  
				// set a possible user name from the directory
				if (preg_match("/.*\/(.*)$/", dirname($ftp_file['filepath']), $user)) {
					$ftp_file['user_name'] = $user[1];
				}
				// is this a directory?
				if ($matches[2] && $matches[2] != 1 && $recursive) {
					$ftp_file['dir'] = file_mirror_ftp_get_files($connection, $file_mirror, $directory .'/'. $ftp_file['filename'], $recursive - 1);
				}
			}
			// if we have a file listing, append it
			if ($ftp_file) {
				$files[] = $ftp_file;
			}
		}
	}
	return $files;
}

/**
 * A wrapper for the ftp_rawlist() php function.
 *
 * @param resource $stream
 * @param string $dir
 *  If strict is TRUE the if any files are still uploading then false is returned
 * @return array
 */
function file_mirror_ftp_completed_file_list(&$connection, $directory) {
	$results = array();
  // If there are no results, exit
  if (! $results = ftp_rawlist($connection, $directory) ) {
  	return array();
  }
  // return the file listing as array
  return $results;
}

/*
* Create directory + upload file on remote server
* @param conn handle to ftp connection
* @param src_file local relative path to file to upload
* @param dst_dir directory on remote ftp server
* @param filename file to upload
*/
function file_mirror_ftp_put($con, $src_file, $dst_file) {
	$localfile=$_SERVER['DOCUMENT_ROOT'].'/'.$src_file;
	if (!is_readable($localfile)){
		$localfile='/home/jj/web/www'.$_SERVER['DOCUMENT_ROOT'].'/'.$src_file;
	}
	if (!is_readable($localfile)){
		watchdog('filemirror', "couldn't read file for upload: ".$localfile);
		return false;
	}
	@ftp_delete($con, $dst_file); //delete possible existing file / directory
	@ftp_rmdir($con, $dst_file);
	@ftp_mkdir($con, dirname($dst_file));
	if (!ftp_put($con, $dst_file, $localfile, FTP_BINARY)){
	    watchdog('filemirror', "upload ". $localfile." failed");
	}
	return true;
}

