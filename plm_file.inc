<?php
function plm_import_form(){

  $form['#attributes'] = array('enctype' => "multipart/form-data");
  $form['import_file'] = array(
    '#type' => 'file',
    '#title' => t('Local File'),
    '#size' => 40,
  );
  
  $form['sid'] = array(
    '#type' => 'select',
    '#title' => t('Season'),
    '#description' => 'Season into which to import results.',
    '#default_value' => variable_get('plm_current_season', 0),
    '#options' => array(
      0 => t('None'),
    ),
  );
  $seasons = _get_plm_seasons();
  if (isset($seasons)) {
    foreach ($seasons as $s) {
      $form['sid']['#options'][$s->sid] = $s->title;
    }
  }
  $form['buyin'] = array(
    '#type'          => 'textfield',
    '#title'         => 'Buy-In',
    '#required'      => TRUE,
    '#size' => 5,
    '#maxlength' => 5,
    '#default_value' => variable_get('plm_default_buyin', 10),
  );
  $form['overwrite'] = array(
    '#type' => 'checkbox', 
    '#title' => t('Overwrite existing'),
    '#description' => 'If imported data matches an existing result, overwrite it.',
    '#default_value' => FALSE,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );
  return $form;
}

function plm_import_form_validate($form, &$form_state) {
  $sid = $form_state['values']['sid'];
  if (!$sid) {
    form_set_error('sid', 'You must select a season into which to import these results');
  }
}
function plm_import_form_submit($form, &$form_state)  {
  $overwrite = $form_state['values']['overwrite'];
  $sid = $form_state['values']['sid'];
  $buyin = $form_state['values']['buyin'];
  $file = file_save_upload('import_file');
  if(!$file) {
    drupal_set_message('ERROR uploading the file:' . $file);
    return;
  }
  drupal_set_message(
    'The file: <strong>"' . $file->filename . '"</strong> is successfully uploaded');

  $filedata = file($file->filepath, FILE_IGNORE_NEW_LINES);
  $res = array();
  $curgame = NULL;
  $linenum=0;
  foreach ($filedata as $line) {
    //drupal_set_message('<pre>' . print_r($line, TRUE) . '</pre>');
    $linenum++;
    $line = trim($line);
    if (!strlen($line)) {
      // empty line...between records
      if ($rec) {
        //drupal_set_message("<b>rec</b><br><pre>" . print_r($rec, TRUE) . "</pre>");
        $rec->finished = 1;
        _import_plm_event($rec, $overwrite);
        $rec = NULL;
        $curplace = 0;
      }
      continue;
    }
    $fields = explode("\t", $line);
    //drupal_set_message(
    //  '<pre> (' . count($fields) . ') '.print_r(implode('[TAB]', $fields), TRUE) . '</pre>');
    if (!$rec) {
      $rec = new stdClass;
      $rec->sid   = $sid;
      $rec->buyin = $buyin;
      $split = strpos($fields[0], ' ');
      $dstr = substr($fields[0], 0, $split);
      //$rec->game_date = date($dstr);
      $rec->event_date = strtotime($dstr);
      $rec->title = trim(substr($fields[0], $split+1));
      $rec->num_players = trim($fields[2]);

      // season 4 is different format
      // m/d/yyyy Week X (EOY)
      if (!$rec->num_players) {
        $p1 = strpos($rec->title, ' (');
        if ($p1 > 0) {
          $p2 = strpos($rec->title, ')', $p1 + 2);
          $rec->num_players = substr($rec->title, $p1+2, $p2-($p1+2))/2;
          $rec->title = substr($rec->title, 0, $p1);
        }
      }
      
      continue;
    }
    if (!strncmp($fields[0], 'EOY', 3)){
      continue;
    }
    if (!strncmp($fields[0], 'Equip', 5)){
      continue;

    }
    if (!strncmp($fields[0], 'Host', 4)){
      $rec->hostname = trim($fields[1], '. ');
      continue;
    }
    if (!strncmp($fields[0], 'Player', 6)){
      $curplace = 1;
      $rec->place[] = NULL;
      $rec->points[] = NULL;
      $rec->cash[] = NULL;
      continue;
    }
    $rec->place[$curplace]  = trim($fields[0], '. ');
    $rec->points[$curplace] = $fields[1];
    $rec->cash[$curplace]   = $fields[2];
    $curplace++;
  }

}


function _import_plm_event($rec, $overwrite) {
  if ($rec->hostname){
    $host = _get_plm_player_by_name($rec->hostname);
    if (!$host) {
      $host = new stdClass;
      $host->name = $rec->hostname;
      $host->pid = _insert_plm_player($host);
    }
    $rec->hid = $host->pid;
  }

  $events = _get_plm_events_by_date($rec->sid, $rec->event_date);
  if ($events) {
    // Match this event
    $event_exists = 1;
    $rec->eid = $events[0]->eid;
    //$rec->['event'] = 1;
    if ($overwrite) {
      _update_plm_event($rec);
    }
  }
  else {
    $event_exists = 0;
    $rec->eid = _insert_plm_event($rec);
  }

  

  $content = "<b>Event Rec: (exists=$event_exists, eid=$rec->eid)</b><br>";

  $rows[]  = array( 
    array('data' => 'Date', 'header' => 1),
    array('data' => date('D, m/d/Y', $rec->event_date), 'colspan' => 3),
  );
  $rows[]  = array( 
    array('data' => 'Event', 'header' => 1),
    array('data' => $rec->title, 'colspan' => 3),
  );
  $rows[]  = array( 
    array('data' => 'Entrants', 'header' => 1),
    array('data' => $rec->num_players, 'colspan' => 3),
  );
  $rows[]  = array( 
    array('data' => 'Place', 'header' => 1),
    array('data' => 'Player', 'header' => 1),
    array('data' => 'Points', 'header' => 1),
    array('data' => '$$$', 'header' => 1),
  );

  // Import results and add new players, if necessary
  for ($i=1; $i<=5 ; $i++) {
    $player = _get_plm_player_by_name($rec->place[$i]);
    if (!$player) {
      $player = new stdClass;
      $player->name = $rec->place[$i];
      $player->pid = _insert_plm_player($player);
      // handle errors!
    }
    $newres->eid    = $rec->eid;
    $newres->place  = $i;
    $newres->cash   = $rec->cash[$i];
    $newres->points = $rec->points[$i];
    $newres->pid    = $player->pid;
    
    $oldres = _get_plm_result($rec->eid, $i);
    if ($oldres) {
      if ($overwrite) _update_plm_result($newres);
    }
    else{
      _insert_plm_result($newres);
    }
    $rows[]  = array( 
      array('data' => _plm_format_place($i), 'header' => 1),
      array('data' => $rec->place[$i]),
      array('data' => $rec->points[$i]),
      array('data' => $rec->cash[$i]),
    );
  }
  $content .= theme('table', NULL, $rows);
  drupal_set_message($content);

}