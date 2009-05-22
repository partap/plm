<?php

function plm_admin() {
  $items = plm_menu();
  $content = '<dl class="admin-list">';
  foreach ( $items as $path => $menu ) {
    $t = explode('/', $path);
    if ( count($t) == 3 ) {
      if (($t[0] == 'admin' ) 
          && $t[1] == 'plm'  
          && $menu['type'] == MENU_NORMAL_ITEM) {
        $content .= '<dt>'. l($menu['title'], $path) .'</dt>';
        if ( isset($menu['description']) ) {
          $content .= '<dd>'. $menu['description'] .'</dd>';
        }
      }
    }
  }
  $content .= "</dl>";
  print theme('page', $content);
}

function plm_settings() {
  $form['plm_default_buyin'] = array( 
    '#type' => 'textfield',
    '#title' => t('Default Buy-in'),
    '#default_value' => variable_get('plm_default_buyin', 10),
    '#size' => 3,
    '#maxlength' => 5,
    '#description' => t("The default buy-in for poker events in this league"),
    '#required' => TRUE, );

  $form['plm_default_prefix'] = array( 
    '#type' => 'textfield',
    '#title' => t('Default Event Title'),
    '#default_value' => variable_get('plm_default_prefix', 'Event '),
    '#size' => 10,
    '#maxlength' => 10,
    '#description' => t("The default title for new poker events"),
    '#required' => TRUE, );

  $form['plm_auto_number'] = array( 
    '#type' => 'checkbox',
    '#title' => t('Auto-Number Events'),
    '#default_value' => variable_get('plm_auto_number', TRUE),
    '#size' => 10,
    '#maxlength' => 10,
    '#description' => t("If enabled, the default event title will be automatically numbered (eg. \"Week 1\", \"Week 2\" ...)"),
    '#required' => TRUE, );

  $form['plm_default_season_length'] = array( 
    '#type' => 'textfield',
    '#title' => t('Default Season Length'),
    '#default_value' => variable_get('plm_default_season_length', NULL),
    '#size' => 3,
    '#maxlength' => 5,
    '#description' => t("The default duration of a season, in weeks"),
  );

  return system_settings_form($form);
}

function plm_settings_validate($form, &$form_state) {
  $buyin = $form_state['values']['plm_default_buyin'];
  if (!is_numeric($buyin))
    form_set_error('plm_default_buyin', 
		   t('You must enter a value for the default buy-in.'));
  else if ($buyin <= 0) 
    form_set_error('plm_default_buyin', 
		   t('The default buy-in must be a non-negative whole number.'));


  $season_length = $form_state['values']['plm_default_season_length'];
  if (strlen($season_length) > 0 
      && (!is_numeric($season_length) 
          || ($season_length < 0) || ($season_length > 52))) {
    form_set_error('plm_default_season_length', 
                   t('The default season length must be between 0 and 52 weeks.'));
  }
}
function plm_settings_submit($form, &$form_state) {
  $buyin = $form_state['values']['plm_default_buyin'];
  variable_set('plm_default_buyin', $buyin);
  $prefix = check_plain($form_state['values']['plm_default_prefix']);
  variable_set('plm_default_prefix', $prefix);
  $season_length = $form_state['values']['plm_default_season_length'];
  variable_set('plm_default_season_length', $season_length);
  $auto_number = $form_state['values']['plm_auto_number'];
  variable_set('plm_auto_number', $auto_number);  
  //print_r('auto_number: ' . $auto_number);
}

function plm_season_form(&$form_state, $rec = NULL) {

  $sid = $rec ? $rec->sid : 0;
  //dprint_r($rec);
  $form['_sid'] = array(
    '#type'  => 'value',
    '#value' => $sid,
  );
 
  $title = $rec->title;
  //if (!isset($title)) $title = variable_get('plm_default_prefix', 'Event ');
  $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#required' => TRUE,
      '#default_value' => $title,
      '#weight' => -5
    );


  $format = 'n/j/Y';
  $form['start_date'] = array(
    '#type'          => 'date_popup',
    '#title'         => t('Start Date'),
    '#required'      => TRUE,
    '#date_format' => $format,
  );
  if ($rec->start_date) 
    $form['start_date']['#default_value'] = date('Y-m-d H:i:s', $rec->start_date);
    
  $form['end_date'] = array(
    '#type'          => 'date_popup',
    '#title'         => t('End Date'),
    '#required'      => FALSE,
    '#date_format' => $format,
  );
  $end_date = $rec->end_date;
  if ($end_date)
    $form['end_date']['#default_value'] = date('Y-m-d H:i:s', $end_date);
  
  // Generate blank schedule for a new season
  $form['sched'] = array(
    '#type' => 'fieldset',
  ); 
  $autonum       = variable_get('plm_auto_number', FALSE);
  $season_length = variable_get('plm_default_season_length', FALSE);
  $generate = $autonum && $season_length && !$sid;

  $form['sched']['autogen'] = array(
    '#type' => 'checkbox',
    '#title' => t('Generate Weekly Schedule'),
    '#required' => TRUE,
    '#default_value' => $autogen,
    '#weight' => -5
  );
  $prefix = variable_get('plm_default_prefix', 'Event');
  $form['sched']['event_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Event Title'),
    '#required' => TRUE,
    '#default_value' => $prefix,
    '#weight' => -5
  );
  $form['sched']['num_weeks'] = array( 
    '#type' => 'textfield',
    '#title' => t('Number of Weeks'),
    '#default_value' => variable_get('plm_default_season_length', 26),
    '#size' => 3,
    '#maxlength' => 5,
    '#description' => t("Number of weeks to generate for this season."),
  );

  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );

  $dest = 'plm/seasons';
  if ($sid) $dest = "plm/seasons/$sid";
  $form['cancel'] = array(
    '#value' => l(t('Cancel'), $dest ),
  );

  return $form;
}


function plm_season_form_validate($form, &$form_state) {

  $title = trim($form_state['values']['title']);
  $sid  = $form_state['values']['_sid'];
  
  if (strlen($title) == 0) 
    form_set_error('title', 'Season Title is required');

  $rec = _get_plm_season_by_title($title);

  if ($rec && $rec->sid != $sid) 
    form_set_error(
      'title', t("There is already a season called <i>@title</i>.", 
                array('@title' => $rec->title)));
  $autogen = $form_state['values']['autogen'];
  if (!$autogen) return;

  $season_length = $form_state['values']['num_weeks'];
  if (strlen($season_length) > 0 && !is_numeric($season_length) 
      || ($season_length <= 0 || $season_length >52)) {
    form_set_error('num_weeks', 
                   t('Schedule must be 1 to 52 weeks long.'));
  }

}

function plm_season_form_submit($form, &$form_state) {
  $sid  = $form_state['values']['_sid'];
  $season->title = trim($form_state['values']['title']);
  $season->sid = $sid;
  $season->start_date = strtotime($form_state['values']['start_date']);
  // End date is optional
  $endstr = $form_state['values']['end_date'];
  if ($endstr)
    $season->end_date = strtotime($endstr);


  if ($sid) $res = _update_plm_season($season);
  else $res = _insert_plm_season($season);

  if ($res) {
    drupal_set_message(
      t("Saved <i>@title</i>.", array('@title' => $season->title)));
  }
  else return;
  if (!$sid) $sid = $res;
  $form_state['redirect'] = "plm/seasons/$sid";

  $autogen = $form_state['values']['autogen'];
  if (!$autogen) return;
  
  $prefix = check_plain($form_state['values']['event_prefix']);
  $num_weeks = $form_state['values']['num_weeks'];
  
  $event_num = (_get_num_plm_events($sid)+1);

  // add an hour in case we pass DST boundary
  $base_date = $season->start_date + 3600; 
  for ($i=0;$i<$num_weeks;$i++){
    $rec = new stdClass();
    $rec->sid        = $sid;
    $rec->title      = $prefix . ' ' . ($event_num+$i);
    $rec->event_date = $base_date + (60*60*24*7 * $i);
    $rec->buyin      = variable_get('plm_default_buyin', 10);
    _insert_plm_event($rec);
  }
  drupal_set_message(
    t("Created @num_weeks new events for <i>@title</i>.", 
      array('@num_weeks' => $num_weeks, '@title' => $season->title)));
  
}
function plm_season_confirm_delete(&$form_state, $season) {
  $form['_season'] = array('#type' => 'value', '#value' => $season);
  // Should probably not be allowed to delete season with event results
  // Delete them first, if really sure
  return confirm_form(
    $form,
    t('Are you sure you want to delete <i>%title</i>?', 
      array('%title' => $season->title)),
    'plm/seasons/'. $season->sid,
    t('All event results for this season will be deleted. '
      . 'This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}
function plm_season_confirm_delete_submit($form, &$form_state) {
  $season = $form_state['values']['_season'];
  plm_season_delete($season->sid);
  drupal_set_message(t("<i>@name</i> has been deleted.", 
                       array('@name' => $season->title)));
  $form_state['redirect'] = 'plm/seasons';
}
function plm_season_delete($sid) {
  _delete_plm_season($sid);
}

function plm_player_form(&$form_state, $player=NULL) {
  if ($player)
    $form['_player'] = array('#type' => 'value', '#value' => $player);
  
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => 'Player Name',
    '#default_value' => $player->name,
    '#required' => TRUE,
  );

  

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );
  return $form;  
}
function plm_player_form_validate($form, &$form_state) {
  $name = trim($form_state['values']['name']);
  $pid  = $form_state['values']['_player']->pid;
  
  if (strlen($name) == 0) 
    form_set_error('name', 'Player Name is required');

  $rec = _get_plm_player_by_name($name);
  //$str="Yubba yubba name=$name pid=$pid rec->pid=$rec->pid";
  //form_set_error('name', $str);

  if ($rec && $rec->pid != $pid) 
    form_set_error(
      'name', t("There is already a player called <i>@name</i>.", 
                array('@name' => $rec->name)));
}

function plm_player_form_submit($form, &$form_state) {
  $pid  = $form_state['values']['_player']->pid;
  $player->name = trim($form_state['values']['name']);
  $player->pid = $pid;

  if ($pid) _update_plm_player($player);
  else _insert_plm_player($player);

  drupal_set_message(
    t("Saved player <i>@name</i>.", array('@name' => $player->name)));
  $form_state['redirect'] = 'plm/players';
}


function plm_player_confirm_delete(&$form_state, $player) {
  $form['_player'] = array('#type' => 'value', '#value' => $player);

  return confirm_form(
    $form,
    t('Are you sure you want to delete the player %name?', 
      array('%name' => $player->name)),
    'plm/players/pid/'. $player->pid,
    t('All event results for this player will be deleted. '
      . 'This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}
function plm_player_confirm_delete_submit($form, &$form_state) {
  $player = $form_state['values']['_player'];
  plm_player_delete($player->pid);
  drupal_set_message(t("The player <i>@name</i> has been deleted.", 
                       array('@name' => $player->name)));
  $form_state['redirect'] = 'plm/players';
}
function plm_player_delete($pid) {
  _delete_plm_player($pid);
}

function plm_event_form(&$form_state, $oldrec=NULL, $season=NULL) {
  if (isset($oldrec)) {
    drupal_set_title("Edit Event");
    $season = plm_season_load($oldrec->sid);
  }
  else {
    drupal_set_title("New Event");
    $oldrec->buyin = _plm_rules_default_buyin($season->sid);
  }
  $season_title = $season->title;
  $sid = $season->sid;
  $form['season'] = array(
    '#title'         => t('Season'),
    '#value' => "<h3>$season_title</h3>",
    "#weight" => -20,
  );
  
  $form['sid'] = array(
    '#type'          => 'value',
    '#value' => $sid,
  );
  $form['eid'] = array(
    '#type'          => 'value',
    '#value' => $oldrec->eid,
  );

  if (!$oldrec->title) {
    $prefix = variable_get('plm_default_prefix', 'Event');
    $oldrec->title = $prefix . ' ' . (_get_num_plm_events($sid)+1);
  }
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#required' => TRUE,
    '#default_value' => $oldrec->title,
    '#size' => 32,
    '#maxlength' => 32,
    '#weight' => -5
  );


  $fmt = 'D, M j Y';
  $event_date = $oldrec->event_date;
  if (!$event_date){
    //$event_date = gmmktime();
    $latest_event = _get_latest_plm_event($sid);
    $last_date = $latest_event->event_date;
    if (!$last_date) $event_date = $season->start_date;
    else {
      //$last_date_str = date($fmt, $last_date);
      //$event_date = strtotime("$last_date_str + 1 week");
      $event_date = $last_date + (60*60*24*7); // Add one week by default
    }
  }
  $form['event_date'] = array(
    '#type'          => 'date_popup',
    '#title'         => t('Event Date'),
    '#required'      => TRUE,
    '#date_format' => $fmt,
    '#default_value' => date('Y-m-d H:i:s', $event_date),
    //'#default_value' => date_make_date($event_date, NULL, DATE_UNIX),
  );
  
  /* $date_str = date($fmt, $event_date); */
  /* $content = "<p>default date: $event_date ($date_str)</p>"; */
  /* $form['new_date'] = array( */
  /*   '#value' => $content, */
  /* ); */
  if ($oldrec->hid){
    $rec = _get_plm_player($oldrec->hid);
    $host = $rec->name;
  }
  $form['host_name'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Event Host'),
    '#default_value' => $host,
    '#size' => 16,
    '#maxlength' => 16,

  );
  $form['num_players'] = array(
    '#type'          => 'textfield',
    '#title'         => 'Num Players',
    '#required'      => TRUE,
    '#size' => 4,
    '#maxlength' => 4,
    '#default_value' => $oldrec->num_players+0,
  );
  $form['pot_start'] = array(
    '#type'          => 'textfield',
    '#title'         => 'Initial Pot',
    '#required'      => TRUE,
    '#size' => 5,
    '#maxlength' => 5,
    '#default_value' => $oldrec->pot_start+0,
  );
  $form['buyin'] = array(
    '#type'          => 'textfield',
    '#title'         => 'Buy-In',
    '#required'      => TRUE,
    '#size' => 5,
    '#maxlength' => 5,
    '#default_value' => $oldrec->buyin+0,
  );
   $form['finished'] = array(
    '#type' => 'checkbox',
    '#title' => t('Finished'),
    '#required' => TRUE,
    '#default_value' => $oldrec->finished,
    '#description' => 'Check this to finalize the results.',
    '#weight' => 1,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save',
    '#weight' => 10,
  );

  return $form;
}
function plm_event_form_validate($form, &$form_state) {

  $host = check_plain(trim($form_state['values']['host_name']));
  if (strlen($host) > 0){ 
    $rec = _get_plm_player_by_name($host);
    if (!$rec)
      form_set_error(
        'host', t("There is no registered player named <i>@name</i>.", 
                  array('@name' => $host)));
  }
  $title = check_plain(trim($form_state['values']['title']));
  
  if (strlen($title) == 0) 
    form_set_error('title', 'Event Title is required');

  $sid = $form_state['values']['sid'];
  $rec = _get_plm_event_by_title($title, $sid);

  $eid  = $form_state['values']['eid'];
  if ($rec && $rec->eid != $eid) 
    form_set_error(
      'title', t("There is already a event called <i>@title</i> in this season.", 
                 array('@title' => $title)));
  
  
}
function plm_event_form_submit($form, &$form_state) {
  $rec->sid          = $form_state['values']['sid'];
  $rec->eid          = $form_state['values']['eid'];
  $rec->title        = check_plain(trim($form_state['values']['title']));
  $rec->event_date   = strtotime($form_state['values']['event_date']);
  $rec->num_players  = $form_state['values']['num_players'];
  $rec->pot_start    = $form_state['values']['pot_start'];
  $rec->buyin        = $form_state['values']['buyin'];
  $rec->finished     = $form_state['values']['finished'];
  
  $host = check_plain(trim($form_state['values']['host_name']));
  if (strlen($host) > 0){ 
    $p = _get_plm_player_by_name($host);
    $rec->hid = $p->pid;
  }

  if ($rec->eid) $res = _update_plm_event($rec);
  else $res = _insert_plm_event($rec);
  
  if ($res) {
    drupal_set_message(
      t("Saved event <i>@title</i>.", array('@title' => $rec->title)));
    
    // Go back to season view if it was a new event
    // Otherwise go to event view (if we just edited it)
    // ...nah, just go to event view always
    if ($rec->eid) 
      $form_state['redirect'] = "plm/events/$rec->eid";
    else 
      $form_state['redirect'] = "plm/events/$res";
  }
}


function plm_result_form(&$form_state, $result) {
  $event = plm_event_load($result->eid);
  $title = $event->title . ': ' . _plm_format_place($result->place) . ' Place';
  drupal_set_title($title);
  $form['_eid'] = array('#type' => 'value', '#value' => $result->eid);
  $form['_place'] = array('#type' => 'value', '#value' => $result->place);
  $form['pid'] = array('#type' => 'value', '#value' => $result->pid);

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => 'Player Name',
    '#default_value' => $result->name,
  );
  $form['_new_player'] = array(
    '#type' => 'checkbox',
    '#title' => 'New Player',
    '#default_value' => FALSE,
    '#description' => 'Check this box to add a new player to the roster.',
  );
  $form['points'] = array(
    '#type' => 'textfield',
    '#title' => 'Points',
    '#default_value' => $result->points,
  );
  $form['cash'] = array(
    '#type' => 'textfield',
    '#title' => 'Cash',
    '#default_value' => $result->cash,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save',
  );
  $form['cancel'] = array(
    '#value' => l("Cancel", "plm/events/$result->eid"),
  );
  $form['delete'] = array(
    '#type' => 'button',
    '#value' => 'Delete',
    '#submit' => 'plm_result_delete_submit',
  );
  return $form;  
}
function plm_result_form_validate($form, &$form_state) {
  $eid        = $form_state['values']['_eid'];
  $place      = $form_state['values']['_place'];
  $new_player = $form_state['values']['_new_player'];

  $name   = trim($form_state['values']['name']);
  $points = trim($form_state['values']['points']);
  $cash   = trim($form_state['values']['cash']);

  
  if (strlen($name) > 0) 
    $rec = _get_plm_player_by_name($name);
  $pid = $rec->pid;
  if (!$pid) {
    if (!$new_player) {
      form_set_error(
        'name', t("There is no registered player named <i>@name</i>.", 
                  array('@name' => $name)));
    }
  }
  else {
    if ($new_player) {
      form_set_error(
        'name', t("There is already a player named <i>@name</i>.", 
                  array('@name' => $name)));      
    }
  }
  $form_state['values']['pid'] = $pid;
  if (!is_numeric($points) || $points < 0)
    form_set_error(
      'points', t("Points must be a non-negative value"));
  if (!is_numeric($cash) || $cash < 0)
    form_set_error(
      'cash', t("Cash must be a non-negative value"));
  
}
function plm_result_form_submit($form, &$form_state) {
  $r->eid     = $form_state['values']['_eid'];
  $r->place   = $form_state['values']['_place'];
  $r->pid     = $form_state['values']['pid'];
  $r->points  = trim($form_state['values']['points']);
  $r->cash    = trim($form_state['values']['cash']);
  $name       = trim($form_state['values']['name']);
  
  $new_player = $form_state['values']['_new_player'];
  if ($new_player){
    $rec = new stdClass();
    $rec->name = $name;
    $r->pid = _insert_plm_player($rec);
  }
  $oldrec = _get_plm_result($r->eid, $r->place);
  if (!$oldrec) {
    if (_insert_plm_result($r)) {
      drupal_set_message(
        t("New result: %name in %place place (%eid)", 
          array('%name' => $name,
                '%place' => _plm_format_place($r->place),
                '%eid' => $r->eid )));
    }
  }
  else {
    if (_update_plm_result($r)) {
      drupal_set_message(
        t("Saved updated result for %name in %place place (%eid)", 
          array('%name' => $name,
                '%place' => _plm_format_place($r->place),
                '%eid' => $r->eid )));
    }      
  }
  $form_state['redirect'] = 'plm/events/' . $r->eid;

}

function plm_result_delete_submit($form, &$form_state) {
  $eid = $form_state['values']['_eid'];
  $place = $form_state['values']['_place'];
  $form_state['redirect'] = 'plm/results/' . $eid . '/' . $place. '/delete';
}

function plm_result_confirm_delete(&$form_state, $result) {
  $form['_result'] = array('#type' => 'value', '#value' => $result);

  return confirm_form(
    $form,
    t('Are you sure you want to delete this result?'),
    'plm/results/'. $result->eid . '/' . $result->place . '/edit',
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}
function plm_result_confirm_delete_submit($form, &$form_state) {
  $result = $form_state['values']['_result'];
  if (plm_result_delete($result->eid, $result->place))
    drupal_set_message(t("The result for event:%eid place: %place has been deleted.", 
                         array('%eid'   => $result->eid,
                               '%place' => $result->place)));
  $form_state['redirect'] = "plm/events/$result->eid";
}
function plm_result_delete($eid,$place) {
  _delete_plm_result($eid, $place);
}

