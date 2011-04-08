<?php
//admin page, shows a list of existing ftp mirrors

function file_mirror_page_list($rows_per_page = 20) {
    
     drupal_set_title(t('File Mirror'));    
    // We get the column names from the table and build the header.
    $header = array();
    
  
    $header[] = array('data' => 'Title', 'field' => '`title`');
    $header[] = array('data' => 'File URL', 'field' => '`url_prefix`');
    $header[] = array('data' => 'User', 'field' => '`uid`');
    $header[] = array('data' => 'Size', 'field' => '`mirror_size`');
    $header[] = array('data' => 'MBL', 'field' => '`bandwidth_limit`');
    $header[] = array('data' => 'Created', 'field' => '`created`');
    $header[] = array('data' => 'Changed', 'field' => '`changed`');
    $header[] = array('data' => 'Enabled', 'field' => '`active`');
    $header[] = array('data' => 'Actions', 'field' => '`fmid`');
    
    // Get the data rows from the table.
    $select = "SELECT title, url_prefix, uid, mirror_size, bandwidth_limit, created, changed, active, fmid FROM {file_mirror}";
    // Set it up so that the user can sort on any column, but the primary key will always be the last value to sort on.
    $select .= tablesort_sql($header);
    // Do the query so that we can page the data.
    $result = pager_query($select, $rows_per_page);
  
    while ($row = db_fetch_array($result)) {
      $line = array();
      foreach ($row as $key => $value) {
        // We use check_markup to apply our filters.
        switch ($key)
        {
            case 'title':
                $line[] = check_markup(l($value, 'admin/settings/file-mirror/'.$row['fmid']), FILTER_FORMAT_DEFAULT,TRUE);
            break;
            case 'url_prefix':
                $line[] = check_markup(l($value, $value), FILTER_FORMAT_DEFAULT,TRUE);
            break;
            case 'active':
                if($value == '0') $value = t('No');
                if($value == '1') $value = t('Yes');
                $line[] = check_markup($value, FILTER_FORMAT_DEFAULT,TRUE);
            break;
            case 'uid':
               $mirror_user = user_load($value);
               $line[] = $mirror_user->name;
            break;
            case 'created':
            case 'changed':
                if($value == '0000-00-00 00:00:00') $value = t('Never');
                $line[] = check_markup($value, FILTER_FORMAT_DEFAULT,TRUE);
            break;
            case 'fmid':
                $value = l('Edit', 'admin/settings/file-mirror/'.$value.'/edit');
                $line[] = check_markup($value, FILTER_FORMAT_DEFAULT,TRUE);
            break;
            default:
                $line[] = check_markup($value, FILTER_FORMAT_DEFAULT,TRUE);
            break;
        }
        
      }
      
      $rows[] = $line;
    }
  
    // Build the displayable table.
    $output .= theme('table', $header, $rows);
    $output .= theme('pager', $rows_per_page);
    return $output;
}

function file_mirror_settings_form($form_state = NULL) {
   //default value;
   
   $q = db_query("SELECT * from {file_mirror_paths}");
   while ($settings = db_fetch_object($q))
   {
     $arr[] = $settings->path;
   }
   $count = count($arr);
   
   $form['paths'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default mirror enabled file paths (relative to the drupal files directory) '),
    '#prefix' => '<div id="default_paths-wrapper">',
    '#suffix' => '</div>',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('If leaved empty, all files found in the Drupal files directory will get mirrored.<br />Don\'t enter a trailing slash / begin with a slash. All given directories are recursive.'),
  );
  for ($i = 0; $i < 6 + $count; $i++)
   $form['paths'][$i] = array(
    '#type' => 'textfield',
    '#title' => t('Relative path'),
    '#size' => 36,
    '#default_value' => $arr[$i],
  );
   $form['submit'] = array(
    '#type' => 'submit',
     '#value' => t('Submit'),
     );
  return $form;
}

function file_mirror_settings_form_submit($form_id, $form_values) {
  db_query("DELETE FROM {file_mirror_paths} WHERE fmid IS NULL");
  $record = (object) NULL;
  foreach ($form_values['values'] as $key => $value)
  {
     if (is_numeric($key) && $value) {
     $record->path = $value;
     drupal_write_record('file_mirror_paths', $record);
     }
  }
  
}

function file_mirror_folders_form($form_state = NULL, $file_mirror) {
//default value;
  
   $q = db_query("SELECT * from {file_mirror_paths} ");
   while ($settings = db_fetch_object($q))
   {
     $default[] = $settings->path;
   }
   $q = db_query("SELECT * from {file_mirror_paths} WHERE fmid = %d", $file_mirror->fmid);
   while ($settings = db_fetch_object($q))
   {
     $folders[] = $settings->path;
   }
   if (count($folders) == 0) $folders = $default;
   $count = count($folders);
   //dsm($folders);
   $form['fmid'] = array(
      '#type' => 'value',
      '#default_value' => $file_mirror->fmid,
    );
   $form['paths'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the folders (relative to the drupal files directory) to sync with this mirror'),
    '#prefix' => '<div id="default_paths-wrapper">',
    '#suffix' => '</div>',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('If leaved empty, all files found in the Drupal files directory will get mirrored.<br />Don\'t enter a trailing slash or begin with a slash. All given directories are recursive.'),
  );
  for ($i = 0; $i < 6 + $count; $i++)
   $form['paths'][$i] = array(
    '#type' => 'textfield',
    '#title' => t('Relative path'),
    '#size' => 36,
    '#default_value' => $folders[$i],
  );
   $form['submit'] = array(
    '#type' => 'submit',
     '#value' => t('Submit'),
     );
  return $form;
}

function file_mirror_folders_form_submit($form_id, $form_values) {
  db_query("DELETE FROM {file_mirror_paths} where fmid = %d", $form_values['values']['fmid']);
  $record = (object) NULL;
  foreach ($form_values['values'] as $key => $value)
  {
     if (is_numeric($key) && $value) {
     $record->path = $value;
     $record->fmid = $form_values['values']['fmid'];
     drupal_write_record('file_mirror_paths', $record);
     }
  }  
}

function file_mirror_edit_form($form_state = NULL, $file_mirror = NULL) {
   global $user;
   
   if (! isset($file_mirror->uid)) {
     $file_mirror->uid = $user->uid;
     $file_mirror->active = 1;
     $file_mirror->dynamic = 1;
   }
   $q = db_fetch_object(db_query("SELECT * FROM {file_mirror} WHERE fmid = %d", arg(4)));
    
  $form['fmid'] = array(
      '#type' => 'value',
      '#default_value' => $file_mirror->fmid,
    );
  
  $form['uid'] = array(
      '#type' => 'value',
      '#default_value' => $file_mirror->uid,
    );
  
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#required' => true,
    '#default_value' => $file_mirror->title,
  );
  
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('dumb paths (split by ;)'),
    '#default_value' => $file_mirror->description,
  );
  
  $form['country'] = array(
    '#type' => 'textfield',
    '#title' => t('geographic location'),
    '#default_value' => $file_mirror->country,
  );
  
  $form['ftp_url'] = array(
    '#type' => 'textfield',
    '#title' => t('FTP address'),
    '#description' => t('Example: ftp.site.com'),
    '#default_value' => $file_mirror->ftp_url,
  );
  
  $form['ftp_user'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#description' => t('Leave empty for anonymous access'),
    '#default_value' => $file_mirror->ftp_user,
  );
  
  $form['ftp_pass'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
    '#maxlength' => 64,
    '#size' => 15,
    '#default_value' => $file_mirror->ftp_pass,
  );
  
  $form['ftp_passive'] = array(
    '#type' => 'checkbox',
    '#title' => t('FTP passive mode'),
    '#description' => t('In passive mode, data connections are initiated by the client, rather than by the server. It may be needed if the client is behind firewall.'),
    '#default_value' => $file_mirror->ftp_passive,
  );
  
  $form['ftp_port'] = array(
    '#type' => 'textfield',
    '#title' => t('FTP port'),
    '#description' => t('The port to connect to. Default is 21'),
    '#maxlength' => 6,
    '#size' => 4,
  );
  
  $form['ftp_dir'] = array(
    '#type' => 'textfield',
    '#title' => t('FTP Directory'),
    '#description' => t('Optional map to use in the ftp site, don\'t add a trailing slash.'),
    '#default_value' => $file_mirror->ftp_dir,
  );
  
  $form['flat'] = array(
    '#type' => 'checkbox',
    '#title' => t('Flatten directories'),
    '#description' => t('Put all files in a single map'),
    '#default_value' => $file_mirror->flat,
  );
  
  $form['url_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('URL prefix'),
    '#required' => true,
    '#description' => t('When storing files on a FTP server you can set a web accessible path to your files. Use http://yourserver.com to create the path. If your files are not web accessible, use and the system will use ftp://path/to/dir/filename.Do not use a trailing slash (/) on your URL'),
    '#default_value' => $file_mirror->url_prefix,
  );
  
  $form['url_deamon'] = array(
    '#type' => 'textfield',
    '#title' => t('PHP Deamon script'),
    '#description' => t('deamon script name (example: deamon.php)'),
    '#default_value' => $file_mirror->url_deamon,
  );
  
  $form['mirror_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Mirror size'),
    '#size' => 10,
    '#description' => t('Maximum size (MB), leave empty for unlimited size'),
    '#default_value' => $file_mirror->mirror_size,
  );
  
  $form['bandwidth_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('MBL'),
    '#size' => 10,
    '#description' => t('Monthly bandwidth limit (MB), leave empty for unlimited bandwidth usage'),
    '#default_value' => $file_mirror->bandwidth_limit,
  );
  
  $form['dynamic'] = array(
    '#type' => 'checkbox',
    '#title' => t('Dynamic'),
    '#description' => t('Sync with main server content'),
    '#default_value' => $file_mirror->dynamic,
  );
  
  $form['active'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#description' => t('Enable or disable mirror'),
    '#default_value' => $file_mirror->active,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}
 
function file_mirror_edit_form_submit($form_id, $form_values) {
  $record = (object) NULL;
  $record->fmid = $form_values['values']['fmid']; 
  $record->title = $form_values['values']['title'];
  $record->description = $form_values['values']['description'];
  $record->country = $form_values['values']['country'];
  $record->ftp_url = $form_values['values']['ftp_url']; 
  $record->ftp_user = $form_values['values']['ftp_user'];  
  $record->ftp_pass = $form_values['values']['ftp_pass'];
  $record->ftp_dir = $form_values['values']['ftp_dir'];
  $record->ftp_passive = $form_values['values']['ftp_passive'];
  $record->ftp_port = $form_values['values']['ftp_port'];
  $record->url_prefix = $form_values['values']['url_prefix'];
  $record->url_deamon = $form_values['values']['url_deamon'];
  $record->uid = $form_values['values']['uid'];
  $record->mirror_size = $form_values['values']['mirror_size']; 
  $record->bandwidth_limit = $form_values['values']['bandwidth_limit'];
  $record->active = $form_values['values']['active'];
  $record->dynamic = $form_values['values']['dynamic'];
  $record->flat = $form_values['values']['flat'];
  $record->changed = date('Y-m-d H:i:s');
  
  if ($record->fmid) {
     drupal_write_record('file_mirror', $record, array('fmid'));
  }
  else{
     $record->created = date('Y-m-d H:i:s');
     drupal_write_record('file_mirror', $record);
  }
}

function file_mirror_page_view($file_mirror, $teaser = FALSE, $page = FALSE, $links = TRUE) {
     foreach ($file_mirror as $key => $value)
     {
          $rows[] = array(array('data' => $key, 'header' => true),$value); 
     }
     return theme('table', $header, $rows);
}

