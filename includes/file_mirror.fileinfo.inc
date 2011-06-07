<?php

/*
allow request by:

	fid
	greater than fid
	springname

	returns: array of:
	<array>
		<struct>
			<fid>5</fid>
			<xml>
			<minimap>
			<mirror>

	</array>
*/
function file_mirror_fileinfo($req){
	$fid=intval($req['fid']);
	if(array_key_exists($req, "list"))
		$cmp='>';
	else
		$cmp='=';
	$springname=$req['springname'];
	$vars=array();
	$cond="";
	if ($fid>0){
		$vars[]=$fid;
		$cond="fid$cmd%d";
	}else if (strlen($springname)>0){
		$vars[]=$springname;
		$cond="springname='%s'";
	}else{
		return array();
	}
	$result=db_query("SELECT * FROM {files} f LEFT JOIN {file_mirror} m ON f.fmid=m.fmid WHERE $cond LIMIT 0,50", $vars);
	$res=array();
        while($row = db_fetch_array($result)){
		$res[]=$row;
	}
	return $res;
}
