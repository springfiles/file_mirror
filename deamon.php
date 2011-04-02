<?php
/*
example (optional) config file:
<?php

define('PASSWORD','');
define('ALLOWED_IP','94.23.170.70');
define('LOCK_FILE','sys_get_temp_dir()./springfiles_daemon.lock');
define('MAX_LOCK_FILE_AGE_S', 10*60); //10 minutes

*/

if (file_exists('config.php'))
	include ('config.php');

if (defined('ALLOWED_IP') && ($_SERVER['REMOTE_IP'] != constant('ALLOWED_IP')))
	exit("disallowed ip!");

if (defined ('PASSWORD') && ($_GET['pw'] != constant('PASSWORD')))
	exit("disallowed password!");

$path = @$_SERVER[DOCUMENT_ROOT] . '/' . @$_GET['p'];

if (is_dir($path))
	exit("File '$path' is a directory!\n");
if (!is_readable($path))
	exit("File '$path' doesn't exist!\n");

if (defined('LOCK_FILE') && file_exists(constant('LOCK_FILE'))){
	//lock file exists. check if its older than x. could be from a terminated deamon.php
	$filelastmodified = filemtime(constant('LOCK_FILE'));
	$age = time() - $filelastmodified;
	if(($age) > constant('MAX_LOCK_FILE_AGE_S'))
		unlink(constant('LOCK_FILE')); //delete the lock and keep running
	else
		exit('Already running since: ' . $age . ' s');
}

if (defined('LOCK_FILE'))
	touch(constant('LOCK_FILE'));
echo hash_file('md5', $path);
if (defined('LOCK_FILE'))
	unlink(constant('LOCK_FILE'));
?>
