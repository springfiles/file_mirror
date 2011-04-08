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

function file_mirror_ftp_connect($host,$port, $username, $password, $passive,$ssl, $dir){
	if($port==0)
		$port=21;
	if($passive==0)
		$passive=false;
	if(strlen(username)<=0)
		$username="anonymous";
	$timeout=15;
	// build the ftp connection
	if(intval($ssl)==1){
		$con = ftp_ssl_connect($host, $port, $timeout);
	}else{
		$con = ftp_connect($host, $port, $timeout);
	}
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

/*
* Create directory + upload file on remote server
* @param conn handle to ftp connection
* @param src_file local relative path to file to upload
* @param dst_dir directory on remote ftp server
* @param filename file to upload
*/
function file_mirror_ftp_put($con, $src_file, $dst_file) {
	if (!is_readable($src_file)){
		watchdog('filemirror', "couldn't read file for upload: ".$src_file);
		return false;
	}
	@ftp_delete($con, $dst_file); //delete possible existing file / directory
	@ftp_rmdir($con, $dst_file);
	@ftp_mkdir($con, dirname($dst_file));
	if (!ftp_put($con, $dst_file, $src_file, FTP_BINARY)){
	    watchdog('filemirror', "upload $src_file to $dst_file failed");
	}
	return true;
}

