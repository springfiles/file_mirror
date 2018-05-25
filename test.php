<?php

require('file_mirror.module');
function watchdog($a, $b)
{
}

$con = mysqli_connect("localhost","jj","changenow","springfiles");
$res = mysqli_query($con, '
	SELECT filepath FROM files f
	JOIN content_field_file c ON c.field_file_fid = f.fid
	JOIN node n ON n.nid = c.nid
	JOIN filehash h ON h.fid = f.fid
	WHERE f.fid NOT IN (
		SELECT f.fid FROM files f
		JOIN file_mirror_files fm ON fm.fid = f.fid
	)
	AND (
		f.filepath LIKE "%spring-maps%"
	)
	GROUP BY h.md5
');

if ($res === FALSE) {
	die(mysqli_error($con) . "\n");
}


while($row = mysqli_fetch_assoc($res)) {
	$path = $row['filepath'];
	$url = "http://springfiles.com/" . $path;
	print($url. "\n");
	_file_mirror_notify_upq($url);
}

print(mysqli_num_rows($res) . "\n");
