<?php
function plm_import_form(){
  popups_add_popups();

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
  $seasons = plm_get_seasons();
  if (isset($seasons)) {
    foreach ($seasons as $s) {
      $form['sid']['#options'][$s->sid] = $s->title;
    }
  }
  $form['newseason'] = array(
    '#value' => l(t('Create Season'), 'plm/seasons/new', 
              array('attributes' => array('class' => 'popups-form'))),
  );

  // popups_add_popups(array( 
  //     '#newseason' => array(
  //       'width' => '600px',
  //       'targetSelectors' => '#edit-sid'
  //     )));
  
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

  $form['compress_tabs'] = array(
    '#type' => 'checkbox', 
    '#title' => t('Compress TABs'),
    '#description' => 'Count multiple TABs as a single TAB character (dirty data file).',
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
  $compress_tabs = $form_state['values']['compress_tabs'];
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
    if ($compress_tabs)
      $line = preg_replace('/\t+/',"\t",$line);
    if (!strlen($line)) {
      // empty line...between records
      if ($rec) {
        //drupal_set_message("<b>rec</b><br><pre>" . print_r($rec, TRUE) . "</pre>");
        if (!$rec->num_players) {
          $rec->num_players = $rec->cash_total / $rec->buyin;
        }
        $rec->finished = 1;
        _plm_import_game($rec, $overwrite);
        $rec = NULL;
        $curplace = 0;
      }
      continue;
    }
    $fields = explode("\t", $line);
    drupal_set_message(
      '<pre> (' . count($fields) . ' fields) '
      . print_r(implode('[TAB]', $fields), TRUE) . '</pre>');
    if (!$rec) {
      $rec = new stdClass;
      $rec->sid   = $sid;
      $rec->buyin = $buyin;
      $split = strpos($fields[0], ' ');
      if (!$split) $dstr = $fields[0];
      else $dstr = substr($fields[0], 0, $split);
      //drupal_set_message("<p>fields[0]:".$fields[0]." split: $split</p>");
      $rec->game_date = strtotime($dstr);
      if ($split) {
        $rec->title = trim(substr($fields[0], $split+1));
        $rec->num_players = trim($fields[2]);
      }
      else{
        $rec->title = '';
        $rec->num_players = 0;
      }
      $rec->cash_total = 0;
      
      if (isset($rec->game_date) && !isset($first_game_date)){
        $first_game_date = $rec->game_date;
        //drupal_set_message("<p>First_game_date: $first_game_date ($dstr) </p>");
      }
      if (!$rec->num_players) {
        $p1 = strpos($rec->title, '(');
        drupal_set_message("<p>p1=$p1 title=$rec->title</p>");
        if (($p1 >0 || $p1 === 0)) {
          $p2 = strpos($rec->title, ')', $p1 + 1);
          $rec->num_players = substr($rec->title, $p1+1, $p2-($p1+1))/2;
          // season 3,4:
          // m/d/yyyy Week X (EOY)
          if ($p1 > 1)
            $rec->title = trim(substr($rec->title, 0, $p1));
          // Season 2:
          // m/d/yyyy (EOY)
          // Compute title below
          else $rec->title = '';
        }
      }
      // season 1:
      // m/d/yyyy
      if (!strlen($rec->title)){
        $weeks = (int) (($rec->game_date - $first_game_date) / (86400 * 7.0) + 0.5);
        $rec->title = sprintf("Week %d", $weeks+1);
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
    $rec->points[$curplace] = trim($fields[1]);
    $rec->cash[$curplace]   = trim($fields[2]);
    $rec->cash_total       += $rec->cash[$curplace];
    $curplace++;
  }

}


function _plm_import_game($rec, $overwrite) {
  if ($rec->hostname){
    $host = plm_get_player_by_name($rec->hostname);
    if (!$host) {
      $host = new stdClass;
      $host->name = $rec->hostname;
      $host->pid = _plm_insert_player($host);
    }
    $rec->hid = $host->pid;
  }

  $games = plm_get_games_by_date($rec->sid, $rec->game_date);
  if ($games) {
    // Match this game
    $game_exists = 1;
    $rec->gid = $games[0]->gid;
    //$rec->['game'] = 1;
    if ($overwrite) {
      _plm_update_game($rec);
    }
  }
  else {
    $game_exists = 0;
    $rec->gid = _plm_insert_game($rec);
  }

  

  $content = "<b>Game Rec: (exists=$game_exists, gid=$rec->gid)</b><br>";

  $rows[]  = array( 
    array('data' => 'Date', 'header' => 1),
    array('data' => date('D, m/d/Y', $rec->game_date), 'colspan' => 3),
  );
  $rows[]  = array( 
    array('data' => 'Game', 'header' => 1),
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
  for ($i=1; isset($rec->place[$i]) ; $i++) {
    $player = plm_get_player_by_name($rec->place[$i]);
    if (!$player) {
      $player = new stdClass;
      $player->name = $rec->place[$i];
      $player->pid = _plm_insert_player($player);
      // handle errors!
    }
    $newres->gid    = $rec->gid;
    $newres->place  = $i;
    $newres->cash   = $rec->cash[$i];
    $newres->points = $rec->points[$i];
    $newres->pid    = $player->pid;
    
    $oldres = plm_get_result($rec->gid, $i);
    if ($oldres) {
      if ($overwrite) _plm_update_result($newres);
    }
    else{
      _plm_insert_result($newres);
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