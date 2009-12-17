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
  $submit_js = "document.getElementById('plm-select-payset-form').submit();";
  $options = array(0 => 'Select a Payout');
  $paysets = _plm_get_paysets();
  if ($paysets) {
    foreach ($paysets as $item) {
      $options[$item->payid] = $item->title;
    }
    //$output .= theme('item_list', $list, 'Payouts');
  }
  if (!$oldrec->gid && !$oldrec->payid)
    $oldrec->payid = variable_get('plm_default_payset', 0);
  $form['payset_sel'] = array(
    '#type' => 'select',
    '#title' => 'Payout Structure',
    '#default_value' => $oldrec->payid,
    '#options' => $options,
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
    $results = _plm_get_results($oldrec->gid);

    $form['results'] = array(
      '#value' => theme('plm_game_results', $oldrec, $results),
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
  $rec->payid = $form_state['values']['payset_sel'];
  variable_set('plm_default_payset', $rec->payid);
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

function plm_payout_editor($payid=0) {
  //drupal_set_message('plm_payset_editor()');
  if ($payid) {
    $payset = _plm_get_payset($payid);
  }
  //drupal_set_message('payset: <pre>' . print_r($payset, TRUE) . '</pre>');
  
  $recs = _plm_get_paysets();
  // if (!$payset && $recs) {
  //   $payset = $recs[0];
  // }
  //if (!isset($payset))
  $output .= drupal_get_form('plm_select_payset_form', $recs, $payset);
  if ($payset)
  //else
    $output .= drupal_get_form('plm_edit_payset_form', $payset);
  if ($payset) {
    $add_button = l('Add Payout', "admin/plm/payout/addrow/$payid/10");
    $output .= "<div>$add_button</div>";
    $payrules = _plm_get_payrules($payset->payid);
    if ($payrules) {
      $dstr = '';
      $max_places = 0;
      $headers = array('Plyrs', 'Pot');
      $num_headers = count($headers);
      $num_places = array();
      foreach ($payrules as $rule) {

        $row_index = $rule->num_players;
        $place_index = $rule->place + $num_headers-1;
        $num_places[$rule->num_players]++;
        $rowdata[$row_index][0] = $rule->num_players;
        $rowdata[$row_index][1] = $rule->num_players * $payset->buyin + $payset->pot_bonus;
        //$num_places = $rowdata[$row_index][2];
        //if (!$num_places || $rule->place > $num_places)
        //  $rowdata[$row_index][2] = $rule->place;
        $rowdata[$row_index][$place_index] = $rule->amount;
        $headers[$place_index] = _plm_format_place($rule->place);
        if ($rule->place > $max_places) $max_places = $rule->place;        
      }
      //drupal_set_message('<pre>num_places=' . print_r($num_places, 1) . '</pre>');
      //drupal_set_message("<pre>$dstr</pre>");
      $this_uri = request_uri();
      //drupal_set_message($this_uri);
      foreach ($rowdata as $rownum => $row) {
        $num_players = $row[0];
        $total = $payset->pot_bonus;
        for ($i=$num_headers; $i <= $max_places+$num_headers; $i++) {
          $place=$i-$num_headers+1;
          if (!$row[$i]) $rowdata[$rownum][$i] = '---' ;
          else $total += $row[$i];
          $rowdata[$rownum][$i] = l($rowdata[$rownum][$i],
                                  "admin/plm/payrule/$payset->payid/$num_players/$place/edit",
                                  array('query' => drupal_get_destination()));

        }
        if ($total == $row[1]) {
          $rowdata[$rownum][$max_places+$num_headers+1] = '';
        }
        else{
          $rowdata[$rownum][$max_places+$num_headers+1] = array( 'data' => $row[1]-$total);
          $rowdata[$rownum][$max_places+$num_headers+1]['class'] = 'error';
        }
        ksort($rowdata[$rownum], SORT_NUMERIC);
      }
//       $num_players = 0;
//       foreach($rowdata as $i => $row) {
//         if ($row[0] > 100) break;
//         if ($row[0] != $num_players) {
//           if ($num_players && $row[0] > $num_players+1) {
//             $newrow = array($num_players+1);
//             $rowdata[$num_players+1] = $newrow;
//           }
//           $num_players = $row[0];
//         }
//       }
      //$headers[$max_places+$num_headers] = array('data' => '', 'colspan' => 2);
      $headers[$max_places+$num_headers] = array(
        'data' => _plm_format_place($max_places+1),
        'class' => 'table-cmd'
      );
      //$headers[$max_places+$num_headers+1] = '';        
      //$headers[$max_places+$num_headers+1] = t('Total');        
      ksort($headers, SORT_NUMERIC);
      $attr = array('class' => 'numgrid');

      $curr_uri = check_plain(request_uri());
      //$curr_uri = $_GET['q'];
      
      $submit_js = "document.getElementById('plm-select-payset-form').submit();";
      
      $addcmd = array();
      foreach($rowdata as $i => $row) {
        $nexti = $i+1;
        if (!isset($rowdata[$nexti])){
          $num_players = $row[0];
          $places = $num_places[$num_players];
          $addcmd[$nexti] = l('add',  "admin/plm/payout/addrow/$payid/$nexti/$places",
                  array('query' => array("destination" => $curr_uri)));
        }
      }
      array_unshift($headers, '');
      foreach($rowdata as $i => $row) {
        $delcmd =  l('del', "admin/plm/payout/delrow/$payid/$row[0]",
                array('query' => array("destination" => $curr_uri)));
        array_unshift($rowdata[$i],
          array(
            //'data' => l('del', $curr_uri, array('query' => array('del' =>'bar'))),
            'data' => "$delcmd",
                    //array('query' => array('del' =>'bar'))),
            'class' => 'table-cmd'
          ));
      }
      foreach($addcmd as $newi => $cmd) {
        $rowdata[$newi][] = array(
          'data' => $cmd,
          'class' => 'table-cmd');
        $rowdata[$newi][] = $newi;
        //$rowdata[$newi][] = $newi*$payset->buyin + $payset->pot_bonus;
      }
      ksort($rowdata, SORT_NUMERIC);

      //drupal_set_message('<pre>' . print_r($rowdata, TRUE) . '</pre>');
      $output .= theme('table', $headers, $rowdata, $attr, $caption);
    }
    
  }
  return $output;
} 

// function plm_payout_new_form(&$form_state) {
//       return drupal_get_form('plm_edit_payset_form');
// }

function plm_select_payset_form(&$form_state, $recs, $payset=NULL) {
  
  // $form['new'] = array(
  //   '#type'  => 'submit',
  //   '#value' => 'New Payout Structure',
    
  // );
  // $form['delete'] = array(
  //   '#type'  => 'submit',
  //   '#value' => 'Delete',
  // );
  $payid = $payset->payid;
  if (!isset($payid)) $payid = 0;
  //drupal_set_message("payid: $payid");
  $options = array(0 => 'Select a Payout');
  if ($recs) {
    foreach ($recs as $item) {
      $options[$item->payid] = $item->title;
    }
    //$output .= theme('item_list', $list, 'Payouts');
  }
  $submit_js = "document.getElementById('plm-select-payset-form').submit();";
  $form['payset_sel'] = array(
    '#type' => 'select',
    '#default_value' => $payid,
    '#options' => $options,
    '#attributes' => array(
      'onChange' => $submit_js, 
      'style' => 'display: inline',
    ),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Select',
    '#attributes' => array('style' => 'display: inline')
  );
  return $form;
}
function plm_select_payset_form_submit($form, &$form_state) {
  $payid = $form_state['values']['payset_sel'];
  $form_state['redirect'] = "admin/plm/payout/edit/$payid";
}

function plm_edit_payset_form(&$form_state, $oldrec=NULL) {
  //drupal_set_message('plm_edit_payset_form()');
  
   if (isset($oldrec)){
//      $form_state['storage']['oldrec'] = array(
//        '#value' => $oldrec,
//      );
     $form['payid'] = array(
       '#type' => 'hidden',
       '#value' => $oldrec->payid,
     );
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

  $form['buyin'] = array(
    '#type' => 'textfield',
    '#title' => t('Buy-in'),
    '#description' => t('Player contribution'),
    '#required' => TRUE,
    '#default_value' => $oldrec->buyin,
    '#size' => 10,
    '#maxlength' => 10,
  );

  $form['pot_bonus'] = array(
    '#type' => 'textfield',
    '#title' => t('Pot Bonus'),
    '#description' => t('Extra added to prize pool'),
    '#required' => TRUE,
    '#default_value' => $oldrec->pot_bonus,
    '#size' => 10,
    '#maxlength' => 10,    
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save Structure',
  );
  $form['gen_rules'] = array(
    '#type' => 'submit',
    '#value' => 'Generate Payouts',
  );

  if ($oldrec->payid){
    $form['delete'] = array(
      '#value' => l('Delete Structure', "admin/plm/payout/delete/$oldrec->payid"),
      '#weight' => 1002,
    );
  }

  return $form;
}
function plm_edit_payset_form_submit($form, &$form_state) {
  //drupal_set_message('<pre>form:' . print_r($form, TRUE) . '</pre>');
  //drupal_set_message('<pre>form_state:' . print_r($form_state, TRUE) . '</pre>');
  //drupal_set_message('<pre>form_state=' . print_r($form_state, 1) . '</pre>');
  $values = $form_state['values'];
  //$oldrec = $form_state['storage']['oldrec'];
  //$isnew = !$oldrec;
  $payid = $values['payid'];
  $isnew = !isset($payid);
  //$rec->payid = $oldrec->payid;
  $rec->payid = $payid;
  $rec->title = $values['title'];
  $rec->buyin = $values['buyin'];
  $rec->pot_bonus = $values['pot_bonus'];
  if ($isnew){
    $rec->payid = _plm_insert_payset($rec);
    $gen_rules = true;
  }
  else
    _plm_update_payset($rec);
  if ($form_state['values']['op'] == 'Generate Payouts') $gen_rules = true;

  if ($gen_rules){
    for ($num_players=10;$num_players<=36;$num_players++) {
      if ($num_players < 20) $num_places = 4;
      else $num_places = 5;
      $payrules = _plm_generate_payrules($num_players, $num_places, $rec->buyin, $rec->pot_bonus);
      _plm_delete_payrules($payid, $num_players);
      foreach ($payrules as $place => $amount) {
        //drupal_set_message("Adding payrule: $payid, $num_players, $place, $amount");
        _plm_insert_payrule($rec->payid, $num_players, $place, $amount);
      }
    }
  }
  $form_state['redirect'] = "admin/plm/payout/edit/$rec->payid";
  //$form_state['redirect'] = "admin/plm/payout/edit/$payid";
}

function plm_payout_confirm_delete(&$form_data,$payid) {
//  drupal_set_message('plm_payout_confirm_delete($payid)<pre>' . print_r($payid, TRUE) . '</pre>');

  $form['payid'] = array('#type' => 'hidden', '#value' => $payid);
  return confirm_form(
    $form,
    "Are you sure you want to delete this payout structure?",
    "admin/plm/payout/edit/$payid",
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}

function plm_payout_confirm_delete_submit($form, &$form_state) {
  $payid       = $form_state['values']['payid'];
  //drupal_set_message("payid: $payid, num_players: $num_players");
  _plm_delete_payset($payid);
  $form_state['redirect'] = "admin/plm/payout/list";
}

function plm_payout_delete_row(&$form_data, $payid, $num_players) {
  $form['payid'] = array('#type' => 'hidden', '#value' => $payid);
  $form['num_players'] = array('#type' => 'hidden', '#value' => $num_players);
  return confirm_form(
    $form,
    "Are you sure you want to delete this row?",
    "admin/plm/payout/edit/$payid",
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}

function plm_payout_delete_row_submit($form, &$form_state) {
  $payid       = $form_state['values']['payid'];
  $num_players = $form_state['values']['num_players'];
  //drupal_set_message("payid: $payid, num_players: $num_players");
  _plm_delete_payrules($payid, $num_players);
}
function plm_payrule_form(&$form_data, $payid, $num_players, $place, $payset=NULL) {
  //drupal_set_message('plm_payrule_form()');
  // $payrules = _plm_get_payrules($payid, $num_players);
  // if ($payrules) {
  //   foreach ($payrules as $rule) {
  //     if ($rule->place == $place) $payrule = $rule;
  //   }
  // }
  if (!$payrule) $payrule = _plm_get_payrule($payid, $num_players, $place);
  if ($payset == NULL) $payset = _plm_get_payset($payid);

  $form['payid'] = array(
    '#type' => 'value',
    '#value' => $payid,
  );
  $form['num_players'] = array(
    '#type' => 'value',
    '#value' => $num_players,
  );
  $form['place'] = array(
    '#type' => 'value',
    '#value' => $place,
  );
  $title = $payset->title . ': ' 
    . $num_players . ' Players, ' 
    . _plm_format_place($place) . ' Place';
  drupal_set_title($title);
  
  $form['amount'] = array(
    '#title'         => t('Amount'),
    '#type'          => 'textfield',
    '#size'          => 10,
    '#maxlength'     => 10,
    '#default_value' => $payrule->amount,
    '#required'      => TRUE,
   // Set initial focus
    '#attributes'    => array('class' => 'focus'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
    '#weight' => 1000,
  );

  $dest = $_REQUEST['destination'];
  if (!isset($dest))
    $dest = 'admin/plm/payouts/$payid';
  $form['cancel'] = array(
    '#value' => l(t('Cancel'), $dest ),
    '#weight' => 2000,
  );

  // $form['cancel'] = array(
  //   '#type' => 'submit',
  //   '#value' => 'Cancel',
  //   '#submit' => array('plm_cancel_form'),
  //   '#weight' => 1001,
  // );
  $form['delete'] = array(
    '#type' => 'submit',
    '#value' => 'Delete',
    '#submit' => array('plm_payrule_delete_form'),
    '#weight' => 1002,
  );
  $selector = '.focus';
  $jquery_snippet = 
    '$(document).ready(function(){
     var i = $("'. $selector .'").not("[@type=\'hidden\']");
     if (i.val() != "") {}
     i[0].focus();});'; 
  drupal_add_js($jquery_snippet, 'inline'); 
  return $form;
}
function plm_payrule_form_submit($form, &$form_state) {
  $payid       = $form_state['values']['payid'];
  $num_players = $form_state['values']['num_players'];
  $place       = $form_state['values']['place'];
  $amount      = $form_state['values']['amount'];
  //drupal_set_message("plm_payrule_form_submit($payid, $num_players, $place, $amount)");
  $rec = _plm_get_payrule($payid, $num_players, $place);
  if ($rec){ 
    //drupal_set_message('updating');
   _plm_update_payrule($payid, $num_players, $place, $amount);
  }
  else {
    // drupal_set_message('inserting');
    _plm_insert_payrule($payid, $num_players, $place, $amount);
  }
}

function plm_cancel_form($form, &$form_state) {
  //drupal_set_message("plm_cancel_form()");
}

function plm_payrule_delete_form($form, &$form_state) {
  $payid       = $form_state['values']['payid'];
  $num_players = $form_state['values']['num_players'];
  $place       = $form_state['values']['place'];
  //drupal_set_message("plm_payrule_delete_form($payid, $num_players, $place)");
  _plm_delete_payrule($payid, $num_players, $place);
}

function plm_add_payrule_form(&$form_data, $payset, $num_players, $place, $amount=0) { 
  $form['amount'] = array(
    '#title'         => t('Amount'),
    '#default_value' => $amount,
    '#required' => TRUE,
  );  
}

function plm_payout_add_row_form(&$form_state, $payid, $num_players=10, $num_places=0) {
  $payset = _plm_get_payset($payid);
  $form['payid'] = array(
    '#type' => 'hidden',
    '#value' => $payid,
  );
  $form['buyin'] = array(
    '#type' => 'hidden',
    '#value' => $payset->buyin,
  );
  $form['pot_bonus'] = array(
    '#type' => 'hidden',
    '#value' => $payset->pot_bonus,
  );
  $form['num_players'] = array(
    '#type' => 'textfield',
    '#title' => '# Players',
    '#default_value' => $num_players
  );
  if (!$num_places) $num_places = $num_players/10;
  $form['num_places'] = array(
    '#type' => 'textfield',
    '#title' => '# Places',
    '#default_value' => round($num_places),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit'
  );
  return $form;
}

function plm_payout_add_row_form_submit($form, &$form_state) {
  $payid = $form_state['values']['payid'];
  $num_players = $form_state['values']['num_players'];
  $num_places = $form_state['values']['num_places'];
  $buyin = $form_state['values']['buyin'];
  $pot_bonus = $form_state['values']['pot_bonus'];
  
  $payouts = _plm_generate_payrules($num_players, $num_places, $buyin, $pot_bonus);
  _plm_delete_payrules($payid, $num_players);
  foreach ($payouts as $place => $amount) {
    //drupal_set_message("Adding payrule: $payid, $num_players, $place, $amount");
    _plm_insert_payrule($payid, $num_players, $place, $amount);
  }
  $form_state['redirect'] = "admin/plm/payout/edit/$payid";
}

function _plm_generate_payrules($num_players, $num_places, $buyin, $pot_bonus) {
  $payouts = array();
  $total = $buyin * $num_players + $pot_total;
  $remaining = $total;
  //for ($i=0;$i<$num_places;$i++) {
  $i = $num_places;
  while ($i > 0) {
    $start_amount = $remaining;
    $stop_amount = floor($start_amount*($i-1)/$i);
    $rem = $stop_amount % 5;
    $stop_amount -= $rem;
    //foreach ($payouts as $place => $amount) {
    //  drupal_set_message("payrule: " . _plm_format_place($place) . " $place place: $amount");
    // }
    //drupal_set_message("start_amount=$start_amount, stop_amount=$stop_amount, rem=$rem");
    while ($remaining > 0) {
      for ($j=1;$j<=$i && $remaining > $stop_amount;$j++) {
        $amount = 5;
        if ($remaining % $amount && $remaining <= $stop_amount+$amount) {
          $amount = $remaining-$stop_amount;
          //drupal_set_message("Remainder: " . _plm_format_place($j) . " place: amount=$amount");
        }
        $payouts[$j] += $amount;
        $remaining   -= $amount;
        //error_log("i=$i, j=$j, "._plm_format_place($j)." place $j=$payouts[$j], remaining=$remaining, amount=$amount");
      }
      if ($remaining <= $stop_amount) {
        $i--;
        break;
      }
    }
  }
  return $payouts;
}
function plm_add_payset_row($payid) {
  $payset = _plm_get_payset($payid);
  $payrules = _plm_get_payrules($payid);
  
  foreach ($payrules as $rule) {
    if ($rule->num_players){
      $output .= '<div>Insert</div>';
    }
    $num_players = $rule->num_players;
    
  }
}