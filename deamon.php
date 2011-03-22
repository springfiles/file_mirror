<?php
$path = $_GET['p'];
if (is_readable($path)){
	echo hash_file('md5', $path);
}else{
	echo "File '$path' doesn't exist!\n";
}

