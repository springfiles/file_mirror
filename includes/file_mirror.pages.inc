<?php
/**
* Theme function for theming links to mirror download locations.
*
* @param $title
* The Mirror title
* @param $link
* The path to the file
* @return
* An HTML themed string.
*/
function theme_file_mirror_link($title = null, $link = null)
{
$output = '<div class="mirror-link-title">'.l($title, $link).'</div>';
return $output;
}