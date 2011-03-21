<?php

/*
 * connect to a remote ftp site
 */

function file_mirror_ftp_connect($file_mirror, $display = false, $timeout = 15) {
  global $connection;
  // build the ftp connection
  $connection['ftp_connection'] = ftp_connect($file_mirror->ftp_url, $file_mirror->ftp_port, $timeout);
  $connection['ftp_login'] = ftp_login($connection['ftp_connection'], $file_mirror->ftp_user, $file_mirror->ftp_pass);
  $connection['ftp_files'] = file_mirror_ftp_get_files($connection['ftp_connection'], $file_mirror, $file_mirror->ftp_dir);
  $connection['ftp_passive'] = true;
  if ($file_mirror->ftp_passive == 1) {
    $connection['ftp_passive'] = ftp_pasv($connection['ftp_connection'],true);
  }

  // check the connection to the server
  if (! $connection['ftp_connection']) {
    $message = 'Unable to connect to server %server';
    $params = array('%server' => $file_mirror->title);
  }
  // can we login?
  elseif (! $connection['ftp_login']) {
    $message = 'Unable to login to server %server with username %user';
    $params =  array('%server' => $file_mirror->ftp_url, '%user' => $file_mirror->ftp_user);
  }
  elseif (! $connection['ftp_passive']) {
    $message = 'Unable to setup a passive connection to %server with username %user';
    $params =  array('%server' => $file_mirror->ftp_url, '%user' => $file_mirror->ftp_user);
  }
  // can we get a list of files?
  elseif (! $connection['ftp_files']) {
    $message = 'Unable to get file list from server %server with username %user';
    $params = array('%server' => $file_mirror->ftp_url, '%user' => $file_mirror->ftp_user);
  }

  // did we have an error condition?
  if ($message) {
    if ($display) { drupal_set_message(t($message, $params), 'error'); }
    else { watchdog(MM_FTP_WATCHDOG_TYPE, $message, $params, WATCHDOG_ERROR); }
    return;
  }
  // no error, return the connection
  return $connection;
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

/**
 * deletes file on remote server
 * @param file_name full filename to delete on remote server
 * @param display show message in drupal if true
 */
function file_mirror_ftp_delete_file($file_name, &$connection, $display = false) {
  // Remove the specified file from the FTP server
  if (!@connection) {
    return FALSE;
  }

  $delete_result = @ftp_delete($connection, $file_name);

  if ($delete_result == TRUE) {
    // The file/folder was deleted successfully
    return TRUE;
  }
  else {
    $message = "Couldn't delete the selected file";
   // did we have an error condition?
    if ($message) {
      if ($display) { drupal_set_message(t($message, $params), 'error'); }
      else { watchdog(MM_FTP_WATCHDOG_TYPE, $message, $params, WATCHDOG_ERROR); }
      return;
    }
    
  }
}

//upload a file to mirror

/*
* Create a directory + upload file on remote server
* @param conn_id handle to ftp connection
* @param src_file local relative path to file to upload
* @param dst_dir directory on remote ftp server
* @param filename file to upload
*/
function file_mirror_ftp_putAll(&$conn_id, $src_file, $dst_file) {
	$localfile=$_SERVER['DOCUMENT_ROOT'].'/'.$src_file;
	if (!is_readable($localfile)){
		$localfile='/home/jj/web/www'.$_SERVER['DOCUMENT_ROOT'].'/'.$src_file;
	}
	if (!is_readable($localfile)){
		watchdog('filemirror', "couldn't read file for upload: ".$localfile);
		return false;
	}
	@ftp_delete($conn_id, $dst_file); //delete possible existing file / directory
	@ftp_rmdir($conn_id, $dst_file);
	@ftp_mkdir($conn_id, dirname($dst_file));
	if (ftp_put($conn_id, $dst_file, $localfile, FTP_BINARY)){
	}else{
	    watchdog('filemirror', "upload ". $localfile." failed");
	}
	return true;
}

/*
 * recursively creates a directory on remote server
*/
function file_mirror_ftpMkDir($path, $conn_id)
  {
   
   $dir=split("/", $path);
   $path="";
   $ret = true;
  
   for ($i=0;$i<count($dir);$i++)
   {
       $path.="/".$dir[$i];
       //echo "$path\n";
       if(!@ftp_chdir($conn_id,$path)){
         @ftp_chdir($conn_id,"/");
         if(!@ftp_mkdir($conn_id,$path)){
          $ret=false;
          break;
         }
       }
   }
   return $ret;
  }
