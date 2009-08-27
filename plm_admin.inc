<?php
function plm_admin() {
  $items = plm_menu();
  $content = '<dl class="admin-list">';
  foreach ( $items as $path => $menu ) {
    $t = explode('/', $path);
    if ( count($t) == 3 ) {
      if (($t[0] == 'admin' ) 
        && ($t[1] == 'plm')
        && ($menu['type'] == MENU_NORMAL_ITEM)) {
        $content .= '<dt>'. l($menu['title'], $path) . '</dt>';
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
  $form['plm_current_season'] = array(
    '#type' => 'select',
    '#title' => t('Current Season'),
    '#description' => 'Season currently underway.  Used for Schedule and Leaderboard.',
    '#default_value' => variable_get('plm_current_season', 0),
    '#options' => array(
      0 => t('None'),
    ),
  );
  $seasons = _plm_get_seasons();
  if (isset($seasons)) {
    foreach ($seasons as $s) {
      $form['plm_current_season']['#options'][$s->sid] = $s->title;
    }
  }
  
  $form['plm_default_buyin'] = array( 
    '#type' => 'textfield',
    '#title' => t('Default Buy-in'),
    '#default_value' => variable_get('plm_default_buyin', 10),
    '#size' => 3,
    '#maxlength' => 5,
    '#description' => t("The default buy-in for poker games in this league"),
    '#required' => TRUE, );

  $form['plm_default_prefix'] = array( 
    '#type' => 'textfield',
    '#title' => t('Default Game Title'),
    '#default_value' => variable_get('plm_default_prefix', 'Game '),
    '#size' => 10,
    '#maxlength' => 10,
    '#description' => t("The default title for new poker games"),
    '#required' => TRUE, );

  $form['plm_auto_number'] = array( 
    '#type' => 'checkbox',
    '#title' => t('Auto-Number Games'),
    '#default_value' => variable_get('plm_auto_number', TRUE),
    '#size' => 10,
    '#maxlength' => 10,
    '#description' => t("If enabled, the default game title will be automatically numbered (eg. \"Week 1\", \"Week 2\" ...)"),
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
  $cur_season = $form_state['values']['plm_current_season'];
  variable_set('plm_current_season', $cur_season);
  
}

function plm_season_form(&$form_state, $rec = NULL) {

  $sid = $rec ? $rec->sid : 0;
  //dprint_r($rec);
  $form['_sid'] = array(
    '#type'  => 'value',
    '#value' => $sid,
  );
  if ($sid) {
    drupal_set_title('Edit Season');
  }
  else{
    drupal_set_title('Create a New Season');
  }
  $title = $rec->title;
  //if (!isset($title)) $title = variable_get('plm_default_prefix', 'Game ');
  $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#required' => TRUE,
      '#default_value' => $title,
      '#weight' => -5
    );


  $format = 'n/j/Y';
  $widget_fmt = 'Y-m-d H:i:s';
  $form['start_date'] = array(
    '#type'          => 'date_popup',
    '#title'         => t('Start Date'),
    '#required'      => TRUE,
    '#date_format' => $format,
  );
  if ($rec->start_date) 
    $form['start_date']['#default_value'] = format_date($rec->start_date, 'custom', 
                                            $widget_fmt, 0);
    
  $form['end_date'] = array(
    '#type'          => 'date_popup',
    '#title'         => t('End Date'),
    '#required'      => FALSE,
    '#date_format' => $format,
  );
  $end_date = $rec->end_date;
  if ($end_date)
    $form['end_date']['#default_value'] = format_date($end_date, 'custom',
                                          $widget_fmt, 0);
  
  // Generate blank schedule for a new season
  $form['sched'] = array(
    '#type' => 'fieldset',
  ); 
  $autonum       = variable_get('plm_auto_number', FALSE);
  $season_length = variable_get('plm_default_season_length', FALSE);
  $generate = $autonum && $season_length && !$sid;

  $form['sched']['autogen'] = array(
    '#type' => 'checkbox',
    '#title' => t('Generate Blank Weekly Schedule'),
    '#required' => TRUE,
    '#default_value' => $autogen,
    '#weight' => -5
  );
  $prefix = variable_get('plm_default_prefix', 'Game');
  $form['sched']['game_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Game Title'),
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

  $rec = _plm_get_season_by_title($title);

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


  if ($sid) $res = _plm_update_season($season);
  else $res = _plm_insert_season($season);

  if ($res) {
    drupal_set_message(
      t("Saved <i>@title</i>.", array('@title' => $season->title)));
  }
  else return;
  if (!$sid) $sid = $res;
  $form_state['redirect'] = "plm/seasons/$sid";

  $autogen = $form_state['values']['autogen'];
  if (!$autogen) return;
  
  $prefix = check_plain($form_state['values']['game_prefix']);
  $num_weeks = $form_state['values']['num_weeks'];
  
  $game_num = (_get_num_plm_games($sid)+1);

  // add an hour in case we pass DST boundary
  $base_date = $season->start_date + 3600; 
  for ($i=0;$i<$num_weeks;$i++){
    $rec = new stdClass();
    $rec->sid        = $sid;
    $rec->title      = $prefix . ' ' . ($game_num+$i);
    $rec->game_date = $base_date + (60*60*24*7 * $i);
    $rec->buyin      = variable_get('plm_default_buyin', 10);
    _plm_insert_game($rec);
  }
  drupal_set_message(
    t("Created @num_weeks new games for <i>@title</i>.", 
      array('@num_weeks' => $num_weeks, '@title' => $season->title)));
  
}
function plm_season_confirm_delete(&$form_state, $season) {
  $form['_season'] = array('#type' => 'value', '#value' => $season);
  // Should probably not be allowed to delete season with game results
  // Delete them first, if really sure
  return confirm_form(
    $form,
    t('Are you sure you want to delete <i>%title</i>?', 
      array('%title' => $season->title)),
    'plm/seasons/'. $season->sid,
    t('All game results for this season will be deleted. '
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
  return _plm_delete_season($sid);
}

function plm_player_form(&$form_state, $player=NULL) {
  //global $user;
  //drupal_set_message('<pre>' . print_r($user, TRUE) . '</pre>');
  if ($player){
    drupal_set_title(t('Edit Player'));
    $form['_player'] = array('#type' => 'value', '#value' => $player);
  }
  else{
    drupal_set_title(t('New Player'));
  }
  $form['#validate'][] = 'plm_player_form_validate';
  $form['#attributes'] = array('enctype' => "multipart/form-data");
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => 'Player Name',
    '#size'               => 20,
    '#maxlength'          => 60,
    '#autocomplete_path'  => 'plm/autocomplete/player',                  
    '#default_value'      => $player->name,
    '#required'           => TRUE,
  );

  if ($player->uid){
    $player_user = user_load($player->uid);
  }
  // Link plm_player to a drupal user account, if it exists
  if (user_access('administer poker league')){
    $form['uid'] = array(
      '#type'  => 'value',
      '#value' => $player->uid
    );
    $form['username'] = array(
      '#type'               => 'textfield',
      '#title'              => 'User Account',
      '#size'               => 20,
      '#maxlength'          => 60,
      '#autocomplete_path'  => 'user/autocomplete',                  
      '#default_value'      => $player_user->name,
    );
  }
  
  // check whether the picture directory exists:
  $picture_path = file_create_path(variable_get('plm_picture_path', 'plm_pictures'));
  file_check_directory($picture_path, 1, 'plm_picture_path');

  // Profile Picture

  $form['plm_picture'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Picture'), 
    '#weight' => 1
  );

  if ($player) {
    // only add a picture to an existing player record
    if ($player->picture){
      //$picture = theme('plm_player_picture', $player->picture);
      $picture = '<div class="edit-player-picture"><img src="' 
        . url($player->picture) . '"/></div>';
      $form['plm_picture']['current_picture'] = array('#value' => $picture);
      $form['plm_picture']['picture_delete'] = array(
        '#type' => 'checkbox', 
        '#title' => t('Delete picture'), 
        '#description' => t('Check this box to delete your current picture.')
      );
    }
    else {
      $form['plm_picture']['picture_delete'] = array('#type' => 'hidden');
    }
    $form['plm_picture']['plm_picture_upload'] = array(
      '#type' => 'file', 
      '#title' => t('Upload picture'), 
      //'#size' => 30, 
      '#description' => t(
        'Your player profile picture. '
        . 'Maximum dimensions are %dimensions and the maximum size is %size kB.', array(
          '%dimensions' => variable_get('plm_picture_dimensions', '250x250'), 
          '%size'       => variable_get('plm_picture_file_size', '100'))
      )
    );
    $form['#validate'][] = 'plm_validate_picture';
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
    '#weight' => 1000,
  );
  //drupal_set_message(print_r('<pre>'.$form['#validate'], TRUE).'</pre>');
  return $form;  
}
function plm_player_form_validate($form, &$form_state) {
  $name = trim($form_state['values']['name']);
  $pid  = $form_state['values']['_player']->pid;

  if (strlen($name) == 0) 
    form_set_error('name', 'Player Name is required');
  $rec = _plm_get_player_by_name($name);

  if ($rec && $rec->pid != $pid) 
    form_set_error(
      'name', t("There is already a player called <i>@name</i>.", 
                array('@name' => $rec->name)));
  $username = trim($form_state['values']['username']);
  if (strlen($username) != 0){
    $account = user_load(array('name' => $username));
    if (!$account) {
      form_set_error('username', 'Invalid user account name');
    }
    else 
      $form_state['values']['uid'] = $account->uid;
  }

}
function plm_validate_picture($form, &$form_state) {
  // If required, validate the uploaded picture.
   $validators = array(
     'file_validate_is_image' => array(),
     'file_validate_image_resolution' => array(
       variable_get('plm_picture_dimensions', '250x250')),
     'file_validate_size' => array(variable_get('plm_picture_file_size', '100') * 1024),
   );
  $myfile = file_save_upload('plm_picture_upload', $validators);
  if ($myfile) {
    // Remove the old picture.
    if (isset($form_state['values']['_player']->picture) 
      && file_exists($form_state['values']['_player']->picture)) {
      file_delete($form_state['values']['_player']->picture);
    }

    // The image was saved using file_save_upload() and was added to the
    // files table as a temporary file. We'll make a copy and let the garbage
    // collector delete the original upload.
    $info = image_get_info($myfile->filepath);
    $pid  = $form_state['values']['_player']->pid;
    $destination = variable_get('plm_picture_path', 'plm_pictures') 
      .'/picture-'. $pid .'.'. $info['extension'];
    if (file_copy($myfile, $destination, FILE_EXISTS_REPLACE)) {
      $form_state['values']['plm_picture'] = $myfile->filepath;
    }
    else {
      form_set_error('picture_upload', t("Failed to upload the picture image; the %directory directory doesn't exist or is not writable.", 
          array('%directory' => variable_get('plm_picture_path', 'plm_pictures'))));
    }
  }
}

function plm_player_form_submit($form, &$form_state) {
  $player  = $form_state['values']['_player'];
  $pid  = $player->pid;
  $player->name = trim($form_state['values']['name']);

  // Delete picture if requested, and if no replacement picture was given.
  if (!empty($form_state['values']['picture_delete'])) {
    if ($player->picture && file_exists($player->picture)) {
      file_delete($player->picture);
    }
    $player->picture = '';
  }
  if (isset($form_state['values']['plm_picture'])){
    $player->picture = $form_state['values']['plm_picture'];
  }

  if (user_access('administer poker league')){
    $player->uid = $form_state['values']['uid'];
    //drupal_set_message("Linking with uid $player->uid");
  }

  if ($pid) _plm_update_player($player);
  else _plm_insert_player($player);
  
  drupal_set_message(
    t("Saved player <i>@name</i>.", array('@name' => $player->name)));
  //$form_state['redirect'] = 'plm/players';
}


function plm_player_confirm_delete(&$form_state, $player) {
  $form['_player'] = array('#type' => 'value', '#value' => $player);

  return confirm_form(
    $form,
    t('Are you sure you want to delete the player %name?', 
      array('%name' => $player->name)),
    'plm/players/pid/'. $player->pid,
    t('All game results for this player will be deleted. '
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
  return _plm_delete_player($pid);
}

function plm_game_form(&$form_state, $oldrec=NULL, $season=NULL) {
  global $user;
  if (isset($oldrec)) {
    drupal_set_title("Edit Game");
    $season = plm_season_load($oldrec->sid);
  }
  else {
    drupal_set_title("New Game");
    $oldrec->buyin = _plm_rules_default_buyin($season->sid);
  }
  $season_title = $season->title;
  $sid = $season->sid;
  $form['season'] = array(
    '#title'         => t('Season'),
    '#value' => '<h3>' . l($season_title, "plm/seasons/$sid") . '</h3>',
    "#weight" => -20,
  );
  
  $form['sid'] = array(
    '#type'          => 'value',
    '#value' => $sid,
  );
  $form['gid'] = array(
    '#type'          => 'value',
    '#value' => $oldrec->gid,
  );

  if (!$oldrec->title) {
    $prefix = variable_get('plm_default_prefix', 'Game');
    $oldrec->title = $prefix . ' ' . (_get_num_plm_games($sid)+1);
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
  $widget_fmt = 'Y-m-d H:i:s';
  $game_date = $oldrec->game_date;
  if (!$game_date){
    //$game_date = gmmktime();
    $latest_game = _plm_get_latest_game($sid);
    $last_date = $latest_game->game_date;
    if (!$last_date) $game_date = $season->start_date;
    else {
      //$last_date_str = date($fmt, $last_date);
      //$game_date = strtotime("$last_date_str + 1 week");
      $game_date = $last_date + (60*60*24*7); // Add one week by default
    }
  }
  $form['game_date'] = array(
    '#type'          => 'date_popup',
    '#title'         => t('Game Date'),
    '#required'      => TRUE,
    '#date_format' => $fmt,
    '#default_value' => format_date($game_date, 'custom', $widget_fmt, 0),
  );
  
  if ($oldrec->hid){
    $rec = _plm_get_player($oldrec->hid);
    $host = $rec->name;
  }
  $form['host_name'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Game Host'),
    '#autocomplete_path'  => 'plm/autocomplete/player',                  
    '#default_value' => $host,
    '#size' => 16,
    '#maxlength' => 16,

  );

  if ($oldrec->gid) 
    $list_entrants = $oldrec->list_entrants;
  else 
    // Use season default
    $list_entrants = $season->list_entrants;

  $form['list_entrants'] = array(
    '#type'          => 'checkbox',
    '#title'         => 'Record All Entrants',
    '#required'      => TRUE,
    '#default_value' => $list_entrants,
  );

  if ($oldrec->gid){
    $form['num_players'] = array(
      '#type'          => 'textfield',
      '#title'         => 'Num Players',
      '#required'      => TRUE,
      '#size' => 4,
      '#maxlength' => 4,
      '#default_value' => $oldrec->num_players+0,
    );
    
    // $form['view_roster'] = array(
    //   '#value' => l('View Roster', "plm/games/$oldrec->gid/roster")
    // );
  }
  else {
    $form['num_players'] = array(
      '#type' => 'hidden',
      '#value' => '0',
    );
    //$form['list_entrants'] = array(
    //   '#type' => 'hidden',
    //   '#value' => '0',
    // );
  }
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
  $announced = TRUE;

  if ($oldrec){
    $announced =  $oldrec->announced;
    if ($oldrec->finished) $announced = TRUE;
  }

  $form['announced'] = array(
    '#type' => 'checkbox',
    '#title' => t('Announced'),
    '#required' => TRUE,
    '#default_value' => $announced,
    '#description' => 'Check this to make the game visible to everyone.',
    '#weight' => 1,
  );
  if ($oldrec->num_players){
    $form['results'] = array(
      '#value' => theme_plm_game_results($oldrec),
      '#weight' => 90,
    );
    $form['finished'] = array(
      '#type' => 'checkbox',
      '#title' => t('Finished'),
      '#required' => TRUE,
      '#default_value' => $oldrec->finished,
      '#description' => 'Check this to finalize the results.',
      '#weight' => 91,
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save',
    '#weight' => 100,
  );

  return $form;
}
function plm_game_form_validate($form, &$form_state) {

  $host = check_plain(trim($form_state['values']['host_name']));
  if (strlen($host) > 0){ 
    $rec = _plm_get_player_by_name($host);
    if (!$rec)
      form_set_error(
        'host', t("There is no registered player named <i>@name</i>.", 
                  array('@name' => $host)));
  }
  $title = check_plain(trim($form_state['values']['title']));
  
  if (strlen($title) == 0) 
    form_set_error('title', 'Game Title is required');

  $sid = $form_state['values']['sid'];
  $rec = _plm_get_game_by_title($title, $sid);

  $gid  = $form_state['values']['gid'];
  if ($rec && $rec->gid != $gid) 
    form_set_error(
      'title', t("There is already a game called <i>@title</i> in this season.", 
                 array('@title' => $title)));
  
  $list_entrants = $form_state['values']['list_entrants'];
  if ($list_entrants){
    $num_entrants = _plm_get_num_game_entrants($gid);
    if ($num_entrants > 0)
      $form_state['values']['num_players'] = $num_entrants;
  }
}
function plm_game_form_submit($form, &$form_state) {
  $rec->sid           = $form_state['values']['sid'];
  $rec->gid           = $form_state['values']['gid'];
  $rec->title         = check_plain(trim($form_state['values']['title']));
  $rec->game_date    = strtotime($form_state['values']['game_date']);
  $rec->num_players   = $form_state['values']['num_players'];
  $rec->pot_start     = $form_state['values']['pot_start'];
  $rec->buyin         = $form_state['values']['buyin'];
  $rec->finished      = $form_state['values']['finished'];
  $rec->announced      = $form_state['values']['announced'];
  $rec->list_entrants = $form_state['values']['list_entrants'];
  if ($rec->finished) $rec->announced = 1;
  $host = check_plain(trim($form_state['values']['host_name']));
  if (strlen($host) > 0){ 
    $p = _plm_get_player_by_name($host);
    $rec->hid = $p->pid;
  }

  if ($rec->gid) $res = _plm_update_game($rec);
  else $res = _plm_insert_game($rec);
  
  if ($res) {
    drupal_set_message(
      t("Saved game <i>@title</i>.", array('@title' => $rec->title)));
    
    // Go back to season view if it was a new game
    // Otherwise go to game view (if we just edited it)
    // ...nah, just go to game view always
    //if ($rec->gid) 
    //  $form_state['redirect'] = "plm/games/$rec->gid";
    //else 
    //  $form_state['redirect'] = "plm/games/$res";
  }
}


function plm_result_form(&$form_state, $result) {
  $game = plm_game_load($result->gid);
  $title = $game->title . ': ' . _plm_format_place($result->place) . ' Place';
  drupal_set_title($title);
  $form['_gid'] = array('#type' => 'value', '#value' => $result->gid);
  $form['_place'] = array('#type' => 'value', '#value' => $result->place);
  $form['pid'] = array('#type' => 'value', '#value' => $result->pid);

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => 'Player Name',
    '#autocomplete_path'  => 'plm/autocomplete/player',                  
    '#default_value' => $result->name,
  );
  $form['_new_player'] = array(
    '#type' => 'checkbox',
    '#title' => 'New Player',
    '#default_value' => FALSE,
    '#description' => 'Check this box if this is a new player to the league.',
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
    '#value' => l("Cancel", "plm/games/$result->gid"),
  );
  $form['delete'] = array(
    '#type' => 'button',
    '#value' => 'Delete',
    '#submit' => 'plm_result_delete_submit',
  );
  return $form;  
}
function plm_result_form_validate($form, &$form_state) {
  $gid        = $form_state['values']['_gid'];
  $place      = $form_state['values']['_place'];
  $new_player = $form_state['values']['_new_player'];

  $name   = trim($form_state['values']['name']);
  $points = trim($form_state['values']['points']);
  $cash   = trim($form_state['values']['cash']);

  
  if (strlen($name) > 0) 
    $rec = _plm_get_player_by_name($name);
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
  $r->gid     = $form_state['values']['_gid'];
  $r->place   = $form_state['values']['_place'];
  $r->pid     = $form_state['values']['pid'];
  $r->points  = trim($form_state['values']['points']);
  $r->cash    = trim($form_state['values']['cash']);
  $name       = trim($form_state['values']['name']);
  
  $new_player = $form_state['values']['_new_player'];
  if ($new_player){
    $rec = new stdClass();
    $rec->name = $name;
    $r->pid = _plm_insert_player($rec);
  }
  $oldrec = _plm_get_result($r->gid, $r->place);
  if (!$oldrec) {
    if (_plm_insert_result($r)) {
//       drupal_set_message(
//         t("New result: %name in %place place (%gid)", 
//           array('%name' => $name,
//                 '%place' => _plm_format_place($r->place),
//                 '%gid' => $r->gid )));
    }
  }
  else {
    if (_plm_update_result($r)) {
      drupal_set_message(
        t("Saved updated result for %name in %place place (%gid)", 
          array('%name' => $name,
                '%place' => _plm_format_place($r->place),
                '%gid' => $r->gid )));
    }      
  }
  //$form_state['redirect'] = 'plm/games/' . $r->gid;

}

function plm_result_delete_submit($form, &$form_state) {
  $gid = $form_state['values']['_gid'];
  $place = $form_state['values']['_place'];
  $form_state['redirect'] = 'plm/results/' . $gid . '/' . $place. '/delete';
}

function plm_result_confirm_delete(&$form_state, $result) {
  $form['_result'] = array('#type' => 'value', '#value' => $result);

  return confirm_form(
    $form,
    t('Are you sure you want to delete this result?'),
    'plm/results/'. $result->gid . '/' . $result->place . '/edit',
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}
function plm_result_confirm_delete_submit($form, &$form_state) {
  $result = $form_state['values']['_result'];
  if (plm_result_delete($result->gid, $result->place))
    drupal_set_message(t("The result for game:%gid place: %place has been deleted.", 
                         array('%gid'   => $result->gid,
                               '%place' => $result->place)));
  $form_state['redirect'] = "plm/games/$result->gid";
}
function plm_result_delete($gid,$place) {
  return _plm_delete_result($gid, $place);
}



function plm_entrant_form(&$form_state, $oldrec=NULL, $game=NULL) {
  if (isset($oldrec)) {
    drupal_set_title("Edit Entrant");
    $game = plm_game_load($oldrec->gid);
    $form['_entrant'] = array('#type' => 'value', '#value' => $oldrec);
    $player = _plm_get_player($oldrec->pid);
  }
  //else {
  //  drupal_set_title("New Entrant");
  //}
  $game_title = $game->title;
  $gid = $game->gid;

  $form['_gid'] = array('#type' => 'value', '#value' => $gid);

  // $form['game'] = array(
  //   '#title'         => t('Game'),
  //   '#value' => '<h3>' . l($game_title, "plm/games/$gid") . '</h3>',
  //   "#weight" => -20,
  // );

  $form['name'] = array(
   '#type' => 'textfield',
   '#title' => 'Player Name',
   '#autocomplete_path'  => 'plm/autocomplete/player',                  
   '#default_value' => $player->name,
   '#required'      => true,
   // Set initial focus
   '#attributes'    => array('class' => 'focus'),
  );
  $form['_new_player'] = array(
    '#type' => 'checkbox',
    '#title' => 'New Player',
    '#default_value' => FALSE,
    '#description' => 'Is this a new player to be added to the league roster?',
  );

  $submit_label = isset($oldrec)?'Save':'Add Player';
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => $submit_label,
  );
  // swiped from focus.module
  //$selector = '#'. $form['#id'] .' input';
  //$selector = '#edit-name';
  $selector = '.focus';
  $jquery_snippet = 
    '$(document).ready(function(){
     var i = $("'. $selector .'").not("[@type=\'hidden\']");
     if (i.val() != "") {}
     i[0].focus();});'; 
  drupal_add_js($jquery_snippet, 'inline'); 
  //$form['delete'] = array(
  //  '#type' => 'button',
  //  '#value' => 'Delete',
  //  '#submit' => 'plm_entrant_delete_submit',
  //);
  return $form;  
  

}

function plm_entrant_form_validate($form, &$form_state) {
  $oldrec        = $form_state['values']['_entrant'];
  $gid           = $form_state['values']['_gid'];
  $new_player = $form_state['values']['_new_player'];
  

  $name   = trim($form_state['values']['name']);
  
  if (strlen($name) > 0) 
    $rec = _plm_get_player_by_name($name);
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
  $entrant = _plm_find_entrant($gid, $pid);
  if ($entrant){
   form_set_error(
        'name', t("<i>@name</i> is already entered in this game.", 
                  array('@name' => $name)));      
  }

  $form_state['values']['pid'] = $pid;
}

function plm_entrant_form_submit($form, &$form_state) {
  $oldrec        = $form_state['values']['_entrant'];
  $r->gid     = $form_state['values']['_gid'];
  $r->pid     = $form_state['values']['pid'];
  $name       = trim($form_state['values']['name']);
  
  $new_player = $form_state['values']['_new_player'];
  if ($new_player){
    $rec = new stdClass();
    $rec->name = $name;
    $r->pid = _plm_insert_player($rec);
  }
  if (!$oldrec) {
    if (_plm_insert_entrant($r)) {
      _plm_update_num_game_entrants($r->gid);
      //drupal_set_message("Registered $name");
    }
  }
  else {
    if (_plm_update_entrant($r)) {
      drupal_set_message("Updated entrant: $name");
    }      
  }
  $form_state['redirect'] = 'plm/games/' . $r->gid . '/entrants';

}

function plm_entrant_cancel_submit($form, &$form_state) {
  $oldrec        = $form_state['values']['_entrant'];
  $form_state['redirect'] = 'plm/games/' . $r->gid . '/entrants';
}

function plm_entrant_delete_submit($form, &$form_state) {
  $oldrec        = $form_state['values']['_entrant'];
  $form_state['redirect'] = 'plm/entrants/' . $entid . '/delete';
}

function plm_entrant_confirm_delete(&$form_state, $entrant) {
  $player = _plm_get_player($entrant->pid);
  $entrant->name = $player->name;
  $form['_entrant'] = array('#type' => 'value', '#value' => $entrant);

  return confirm_form(
    $form,
    "Are you sure you want to unregister <i>$entrant->name</i>?",
    'plm/entrants/'. $entrant->entid . '/edit',
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}
function plm_entrant_confirm_delete_submit($form, &$form_state) {
  $entrant = $form_state['values']['_entrant'];
  if (plm_entrant_delete($entrant->entid)){
    _plm_update_num_game_entrants($r->gid);
    drupal_set_message("</i>$entrant->name</i> has been unregistered.");
  }
  $form_state['redirect'] = "plm/games/$entrant->gid/entrants";
}
function plm_entrant_delete($entid) {
  return _plm_delete_entrant($entid);
}

