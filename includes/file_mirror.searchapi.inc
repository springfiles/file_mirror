<?php

/**
* @param query resulting query, for example "AND filename='%s'"
* @param vars data for query, for example "Zero-K v0.51"
* @param logical how the queries are assigned, AND/OR
* @param condition SQL condition, for example "LIKE filename '%s'" or "filename='%s'"
* @param data data from the xml-rpc request, if empty string, then don't change query + vars
*/

function _file_mirror_createquery(&$query, &$vars, $logical,$condition, $data){
	if($query=="") $logical="";
	
	if($data=="")
		return;
	$query.=" ".$logical." ".$condition;
	$data=str_replace("_","\_",$data);
	array_push($vars, $data);
}
/**
*	implementation of the xml-rpc call
*/

function file_mirror_search($req){
global $base_url;
$res="";
$category="%";
$query="";
$vars=array($base_url."/node/");


if(isset($req['logical'])&&($req['logical']=="or")){
	$logical="OR";
}else{
	$logical="AND";
}

_file_mirror_createquery($query,$vars, $logical,'node_type.title LIKE "%s"', $req['category']);
_file_mirror_createquery($query,$vars, $logical,'files.filename LIKE "%s"', $req['filename']);
_file_mirror_createquery($query,$vars, $logical,'archives.springname LIKE BINARY "%s"', $req['springname']);

if ($query!="")
	$query=" AND (".$query.")";
$result=db_query("
SELECT
files.fid as fid,
files.filename as filename,
files.filepath as filepath,
files.origname as origname,
node.title as title,
hash.md5 as md5,
archives.springname as springname,
node_type.title as category,
files.filesize as size,
files.timestamp as timestamp,
CONCAT('%s',CAST(node.nid AS CHAR)) as description
FROM {files}
LEFT JOIN {content_field_file} AS cont ON files.fid= cont.field_file_fid
LEFT JOIN {node} AS node ON cont.nid = node.nid
LEFT JOIN {filehash} AS hash ON files.fid = hash.fid
LEFT JOIN {content_type_file} AS type ON cont.nid=type.nid
LEFT JOIN {node} as node_type ON type.field_category_nid=node_type.nid
LEFT JOIN {springdata_archives} as archives ON files.fid = archives.fid
WHERE node.nid IS NOT NULL
$query
ORDER BY node.nid DESC
LIMIT 0,10
",
$vars
);

$res=array();
while($row = db_fetch_array($result)){
	//add primary server
	$row['mirrors']=array($base_url.'/'.$row['filepath']);
	unset($row['filepath']);
	unset($row['origname']);
	unset($row['title']);
	$res[]=$row;
}
for($i=0;$i<count($res);$i++){
	//search + add depends to file
	$result=db_query("SELECT archives.springname as springname
FROM {springdata_depends} AS depends
LEFT JOIN {springdata_archives} AS archives ON depends.depends=archives.fid
WHERE depends.fid=%d",$res[$i]['fid']);
	while($row = db_fetch_array($result)){
		if(!is_array($res[$i]['depends']))
			$res[$i]['depends']=array();
		$res[$i]['depends'][]=$row['springname'];
	}
	//search + add additional mirrors to file
	$result=db_query('SELECT CONCAT(mirror.url_prefix,"/",files.path) as url
FROM file_mirror_files as files
LEFT JOIN file_mirror as mirror ON files.fmid=mirror.fmid
WHERE files.fid=%d
AND mirror.active=1
AND files.active=1
ORDER BY files.md5check',array($res[$i]['fid']));
	while($row = db_fetch_array($result)){
		$res[$i]['mirrors'][]=$row['url'];
	}
	//randomize order of result
	shuffle($res[$i]['mirrors']);
	unset($res[$i]['fid']);
	if ($res[$i]['springname']=="")
		unset($res[$i]['springname']);
}
$count=count($res);
if ($count<>1)
	watchdog("searchapi", "results: ".$count." request: ".$req['category']." ".$req['filename']." ".$req['springname']." ".$req['logical']);
return $res;
}
