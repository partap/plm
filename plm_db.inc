<?php


function _plm_get_num_players() {
  $sql = "SELECT count(*) FROM {plm_player}";
  return db_result(db_query($sql));
}
function _plm_get_player($pid) {
  $sql = "SELECT * from {plm_player} WHERE pid=%d";
  return db_fetch_object(db_query($sql, $pid));
}
function _plm_get_player_by_name($name) {
  $sql = "SELECT * FROM {plm_player}
            WHERE lower(name)=lower('%s')";
  $res = db_fetch_object(db_query($sql, $name));
  return $res;
}
function _plm_get_player_by_uid($uid) {
  if ($uid == 0) return NULL;
  $sql = "SELECT * FROM {plm_player}
            WHERE uid=%d";
  $res = db_fetch_object(db_query($sql, $uid));
  return $res;
}

function _plm_get_players($sort_phrase, $where_phrase = '', $max=-1) {
  if (!$sort_phrase) $sort_phrase = "ORDER BY name";
//   $sql = "SELECT p.pid, p.name, p.uid,
//           SUM(points) AS points, SUM(cash) AS cash, COUNT(points) AS places,
//           SUM(CASE WHEN r.place=1 THEN 1 ELSE 0 END) AS wins
//           FROM {plm_player} p 
//           LEFT JOIN {plm_result} r ON p.pid=r.pid "
  $sql = "SELECT p.pid, p.name, p.uid,
          SUM(points) AS points, SUM(cash) AS cash, COUNT(points) AS places,
          SUM(CASE WHEN r.place=1 THEN 1 ELSE 0 END) AS wins,
          Max(g.game_date) as last_date
          FROM {plm_game} g
          LEFT JOIN {plm_entrant} e ON e.gid=g.gid
          LEFT JOIN {plm_player} p ON p.pid = e.pid
          LEFT JOIN {plm_result} r ON p.pid=r.pid AND r.gid=g.gid
          WHERE g.finished != 0"
          . $where_phrase . " GROUP BY p.pid, p.name, p.uid " . $sort_phrase . " NULLS LAST";  
  //drupal_set_message("SQL1: $sql");
  if ($max > 0){
    $count_qry = 'SELECT count(*) from {plm_player}';
    $result = pager_query($sql, $max, 0, $count_qry);
    //$result = pager_query($sql, $max, 0);
    //drupal_set_message("SQL2: $sql");
  }
  else{
    $result = db_query($sql);
  }
  $list = array();
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list;
}

function _plm_get_rankings($sid, $limit = NULL) {
  $sort_phrase = "ORDER BY points DESC, cash DESC, places DESC, 
  latest_place DESC, name NULLS LAST";
  if ($limit) $sort_phrase .= " LIMIT $limit";
  $sql = "SELECT p.pid, p.name, g.sid,
    SUM(points) AS points, SUM(cash) AS cash, 
    COUNT(points) AS places, 
    EXTRACT('epoch' FROM 
      DATE_TRUNC('day', NOW()) - DATE_TRUNC( 'day', 
      (TIMESTAMP 'epoch' + MAX(g.game_date) * INTERVAL '1 second'))
    )/86400 
    AS latest_place,
    date_trunc('day', NOW() - DATE_TRUNC( 'day', (TIMESTAMP WITH TIME ZONE 'epoch' + MAX(g.game_date) * INTERVAL '1 second'))) AS latest_interval
    FROM {plm_player} p LEFT JOIN {plm_result} r ON p.pid=r.pid 
    JOIN {plm_game} g ON r.gid=g.gid
    WHERE g.sid=%d
    GROUP BY g.sid,p.pid, p.name " . $sort_phrase ; 
  $result = db_query($sql, $sid);
  $list = array();
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list;
}
function _plm_get_active_players_options($game) {
  $gid = $game->gid;
  $game_date = $game->game_date;
  //$now = time();
  $sql = "SELECT p.pid AS pid, p.name AS name, Max(g.game_date) AS last
          FROM {plm_player} p
          LEFT JOIN plm_entrant e ON p.pid=e.pid 
          LEFT JOIN plm_game g ON (e.gid=g.gid AND g.game_date < $game_date)
          WHERE p.pid NOT IN (SELECT x.pid FROM plm_entrant x WHERE x.gid=$gid) 
          GROUP BY p.pid,p.name
          ORDER BY name";
  //ORDER BY last DESC NULLS LAST,name";
  $result = db_query($sql, $gid);
  $list = array();
  while ($row = db_fetch_object($result)) {
    $weeks = (int) (($game_date - $row->last) / (60 * 60 * 24 * 7));
    $label = "$row->name";
    if ($row->last && $weeks > 0) 
      $label .= " ($weeks weeks)";
    $list[$row->pid] = $label;
  }
  return $list;
}

function _plm_get_active_players($game) {
  $gid = $game->gid;
  $game_date = $game->game_date;
  //$now = time();
  $sql = "SELECT p.pid AS pid, p.name AS name, Max(g.game_date) AS last,
          (CASE WHEN p.pid IN (SELECT x.pid FROM {plm_entrant} x WHERE x.gid=%d) 
             THEN 1 ELSE 0 END) AS registered
          FROM {plm_player} p
          LEFT JOIN {plm_entrant} e ON p.pid=e.pid 
          LEFT JOIN {plm_result} r ON p.pid=r.pid
          LEFT JOIN {plm_game} g ON ((e.gid=g.gid OR r.gid=g.gid) AND g.game_date < %d)
          GROUP BY p.pid,p.name
          ORDER BY name;";
  $result = db_query($sql, $gid, $game_date);
  $list = array();
  while ($row = db_fetch_object($result)) {
    $list[] = $row;
  }
  return $list;
}

function _plm_get_prev_game($pid, $gid){
  $sql = "SELECT g.gid, g.game_date 
          FROM plm_game g 
          WHERE g.gid=(
            SELECT g.gid FROM plm_entrant e LEFT JOIN plm_game g ON (g.gid=e.gid AND e.pid=%d) 
            WHERE g.game_date < (SELECT l.game_date FROM plm_game l WHERE l.gid=%d) 
            ORDER BY g.game_date DESC LIMIT 1)";
  $result = db_query($sql, $pid, $gid);
  return db_fetch_object($result);
}

function _plm_insert_player($player) {
  $sql = "INSERT INTO {plm_player} (name, uid) VALUES ('%s', %d)";
  if (!db_query($sql, $player->name, $player->uid)) return FALSE;
  return db_last_insert_id(db_prefix_tables('{plm_player}'), 'pid');
}

function _plm_update_player($player) {
  $sql = "UPDATE {plm_player} 
          SET name='%s', uid=%d, picture='%s'
          WHERE pid=%d";
  return db_query($sql, 
    $player->name, $player->uid, $player->picture,
    $player->pid);
}

function _plm_delete_player($pid) {
  $sql = "DELETE FROM {plm_player} where pid=%d";
  return db_query($sql, $pid);
}

function _plm_get_players_autocomplete($string) {
  $sql = "SELECT name FROM {plm_player} 
          WHERE LOWER(name) LIKE LOWER('%s%%')";
  $result = db_query_range($sql, $string, 0, 10);
  $matches = array();
  while ($player = db_fetch_object($result)) {
    $matches[$player->name] = check_plain($player->name);
  }
  return $matches;
}
function _plm_insert_season($season) {
  $res =  db_query("INSERT INTO {plm_season} (title, start_date, end_date) 
                    VALUES ('%s', %d, %d)", 
                   $season->title, $season->start_date, $season->end_date);
  if (!$res) return $res;
  return db_last_insert_id(db_prefix_tables('{plm_season}'), 'sid');
  
}
function _plm_update_season($season) {
  return db_query("UPDATE {plm_season} 
                   SET title='%s', start_date=%d, end_date=%d 
                   WHERE sid = %d", 
                  $season->title, $season->start_date, $season->end_date, $season->sid);
}
function _plm_delete_season($sid) {
  // Delete games, results from the season too..
  $res = db_query("DELETE FROM {plm_result} WHERE gid IN 
                   (SELECT gid FROM {plm_game} WHERE sid=%d)", $sid);
  if (!$res) return $res;
  $res = db_query("DELETE FROM {plm_game} where  sid=%d", $sid);
  if (!$res) return $res;
  $res = db_query("DELETE FROM {plm_season} where sid=%d", $sid);
  return $res;
}
function _plm_get_current_season() {
  // Select newest season by default
  $sql = "SELECT s.*, NULLIF(s.end_date,0) AS end_dt 
    FROM {plm_season} s 
    ORDER BY end_dt DESC, s.start_date DESC LIMIT 1";
  return db_fetch_object(db_query($sql));
}

function _plm_get_seasons() {
  $sql = "SELECT *, NULLIF(s.end_date,0) AS end_dt
    FROM {plm_season} s 
    ORDER BY end_dt DESC, s.start_date DESC";
  $result = db_query($sql);
  $res = array();
  while ($rec = db_fetch_object($result)) {
    $res[] = $rec;
  }
  return $res;
}

function _plm_get_season_by_title($title) {
  $sql = "SELECT * FROM {plm_season}
            WHERE lower(title)=lower('%s')";
  $res = db_fetch_object(db_query($sql, $title));
  return $res;
}

function _plm_get_season($sid) {
$sql = "SELECT *, NULLIF(s.end_date,0) AS end_dt
    FROM {plm_season} s
    WHERE s.sid=%d";
  return  db_fetch_object(db_query($sql, $sid));
}

function _plm_get_num_games($sid, $where_phrase=NULL) {
  $sql = "SELECT count(*) as num_games 
          FROM {plm_game}
          WHERE sid=%d";
  if (isset($where_phrase)) $sql .= " AND $where_phrase";
  return  db_result(db_query($sql, $sid));
}

function _plm_get_games($sid, $sort_phrase=NULL) {
  if (!isset($sort_phrase)) $sort_phrase='ORDER BY g.game_date';
  
  $sql = "SELECT g.gid, g.title, p.name as host, g.game_date, g.hid, 
            g.num_players, r.pid AS wid, p2.name AS winner, g.announced, g.finished
          FROM {plm_game} g 
          LEFT JOIN {plm_player} p ON p.pid=g.hid 
          LEFT JOIN {plm_result} r ON g.gid=r.gid
          LEFT JOIN {plm_player} p2 ON r.pid=p2.pid
          WHERE g.sid=%d AND (r IS NULL OR r.place=1)
          $sort_phrase";
  $result = db_query($sql, $sid);
  while ($row = db_fetch_object($result)){
    // Temp hardcode!!!
    $row->eoy_rake = $row->num_players * 3;

    $list[] = $row;
  }
  return $list;
}

// Get all games in this season on this day
function _plm_get_games_by_date($sid, $game_date) {
  $sort_phrase='ORDER BY g.game_date';
  $sql = "SELECT g.gid, g.title, p.name as host, g.game_date, g.hid, 
          g.num_players, g.announced, g.finished
          FROM {plm_game} g 
          LEFT JOIN {plm_player} p ON p.pid=g.hid 
          WHERE g.sid=%d AND game_date BETWEEN %d AND %d
          $sort_phrase";
  $result = db_query($sql, $sid,  $game_date, $game_date + 86400);
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list;
}

function _plm_get_game($gid) {
  $sql = "SELECT * from {plm_game} WHERE gid=%d";
  return db_fetch_object(db_query($sql, $gid));
}
function _plm_get_latest_game($sid, $finished=FALSE) {
  global $user;
  $qry =  " AND game_date <= " . (time() + $user->timezone);
  if ($finished) $qry = " AND g.finished != 0";
  $qry .= " ORDER BY game_date DESC LIMIT 1";
  $latest_games = _plm_get_games($sid, $qry);
  return $latest_games[0];
}

function _plm_get_upcoming_games($sid) {
  global $user;
  $qry .=  " AND announced != 0 AND g.game_date >= " 
    . (time() + $user->timezone);
  $qry .= " ORDER BY game_date";
  //drupal_set_message("sort_phrase: $qry");
  $upcoming_games = _plm_get_games($sid, $qry);
  return $upcoming_games;
}
function _plm_get_game_by_title($title, $sid) {
  $sql = "SELECT * from {plm_game} WHERE title=lower('%s') and sid=%d";
  return db_fetch_object(db_query($sql, $title, $sid));
}

// return id of new record or FALSE if error
function _plm_insert_game($rec) {
  $res = db_query("INSERT INTO {plm_game} 
                   (title, sid, game_date, hid, num_players, pot_start, buyin, 
                    announced, finished, payid) 
                   VALUES ('%s', %d, %d, %d, %d, %d, %d, %d, %d, %d)", 
         $rec->title, $rec->sid,
         $rec->game_date, $rec->hid, $rec->num_players, 
         $rec->pot_start, $rec->buyin, $rec->announced, $rec->finished, $rec->payid);
  if (!$res) return $res;
  return db_last_insert_id(db_prefix_tables('{plm_game}'), 'gid');
}
function _plm_update_game($rec) {
  return db_query("UPDATE {plm_game} 
                   SET title='%s', game_date=%d, 
                   sid=%d, hid=%d, num_players=%d,
                   pot_start=%d, buyin=%d, announced=%d, finished=%d, list_entrants=%d,
                   payid=%d
                   WHERE gid=%d", 
    $rec->title, $rec->game_date, 
    $rec->sid, $rec->hid, $rec->num_players,
    $rec->pot_start, $rec->buyin, $rec->announced, $rec->finished, $rec->list_entrants,
    $rec->payid,
    $rec->gid);
}

function _plm_delete_game($gid) {
  $res = db_query("DELETE FROM {plm_entrant} WHERE gid=%d", $gid); 
  if (!$res) return $res;
  $res = db_query("DELETE FROM {plm_result} WHERE gid=%d", $gid); 
  if (!$res) return $res;
  return db_query("DELETE FROM {plm_game}  WHERE gid=%d", $rec->gid);
}


function _plm_get_result($gid, $place){
  $sql = "SELECT gid, place, pid, p.name, points, cash 
          FROM {plm_result} r
          LEFT JOIN {plm_player} p USING(pid)
          WHERE r.gid=%d and r.place=%d";
  $res = db_fetch_object(db_query($sql, $gid, $place));
  return $res;
}

function _plm_get_results($gid){
  $sql = "SELECT gid, place, pid, p.name, points, cash 
          FROM {plm_result} r
          LEFT JOIN {plm_player} p USING(pid)
          WHERE r.gid=%d
          ORDER BY place";
  $result = db_query($sql, $gid);
  while ($row = db_fetch_object($result)) {
    $res[$row->place] = $row;
  }
  return $res;
}

function _plm_get_results_by_player($pid, $sid=NULL) {
  $sql = "SELECT s.sid, g.gid, g.title, g.hid, p.name AS host, r.place, r.points, r.cash
          FROM {plm_result} r 
          LEFT JOIN ({plm_game} g LEFT JOIN  {plm_player} p ON g.hid=p.pid) USING (gid)
          LEFT JOIN {plm_season} s USING (sid)
          WHERE r.pid=%d ";
  if ($sid) $sql .= "AND g.sid=%d ";
  $sql .= "ORDER BY s.start_date, g.game_date";
  $result = db_query($sql, $pid, $sid);
  while ($row = db_fetch_object($result)) {
    $res[] = $row;
  }
  return $res;
  
  
}

function _plm_insert_result($rec) {
  $sql = "INSERT INTO {plm_result} (gid, place, pid, points, cash)
            VALUES (%d, %d, %d, %d, %d)";
  return db_query($sql, $rec->gid, $rec->place, $rec->pid, $rec->points, $rec->cash);
}

function _plm_update_result($rec) {
    $sql = "UPDATE {plm_result} 
          SET pid=%d, points=%d, cash=%d
          WHERE gid=%d AND place=%d";
    $res = db_query($sql,
             $rec->pid, $rec->points, $rec->cash,
             $rec->gid, $rec->place);
    return $res;
}

function _plm_delete_result($gid, $place) {
    $sql = "DELETE FROM {plm_result} where gid=%d AND $place=%d";
    return db_query($sql, $gid,$place);
}

function _plm_get_entrant($entid) {
  $sql = "SELECT * FROM {plm_entrant} 
          WHERE entid=%d";
  return db_fetch_object(db_query($sql, $entid));
}

function _plm_find_entrant($gid, $pid) {
  $sql = "SELECT * FROM {plm_entrant} 
          WHERE gid=%d AND pid=%d";
  return db_fetch_object(db_query($sql, $gid, $pid));
}

function _plm_get_game_entrants($gid) {
  $sql = "SELECT e.entid, e.gid, e.pid, p.name  
          FROM {plm_entrant} e
          LEFT JOIN {plm_player} p USING (pid)
          WHERE e.gid=%d
          ORDER BY e.entid";
  $result = db_query($sql, $gid);
  $list = array();
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list;
}
function _plm_get_num_game_entrants($gid) {
  $sql = "SELECT count(*) AS n
          FROM {plm_entrant} e
          WHERE gid=%d";
  $result = db_fetch_object(db_query($sql, $gid));
  if ($result)
    return $result->n;
  return $result;
}
function _plm_update_num_game_entrants($gid) {

  $sql = "UPDATE {plm_game} 
          SET num_players = (SELECT COUNT(*) FROM {plm_entrant} WHERE gid=%d)
          WHERE gid=%d AND list_entrants != 0";
  $result = db_query($sql, $gid, $gid);
  return $result;
}
function _plm_insert_entrant($rec) {
  $sql = "INSERT INTO {plm_entrant} (gid, pid) 
          VALUES (%d, %d)";
  $res = db_query($sql, $rec->gid, $rec->pid);
  if (!$res) return $res;
  return db_last_insert_id(db_prefix_tables('{plm_entrant}'), 'entid');  
}

function _plm_update_entrant($rec) {
  $sql = "UPDATE {plm_entrant} 
          SET gid=%d, pid=%d
          WHERE entid=%d";
  $res = db_query($sql, $rec->gid, $rec->pid, $rec->entid);
  return $res;
}

function _plm_delete_entrant($entid) {
  $sql = "DELETE FROM {plm_entrant} WHERE entid=%d";
  return db_query($sql, $entid);
}

function _plm_get_paysets() {
  $sql = "SELECT * FROM {plm_payset} ORDER BY payid"; 
  $result = db_query($sql);
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list; 
}
function _plm_get_payset($payid) {
  $sql = "SELECT * FROM {plm_payset} WHERE payid=%d"; 
  $result = db_query($sql, $payid);
  return db_fetch_object($result);
}
function _plm_insert_payset($rec) {
  $sql = "INSERT INTO {plm_payset} (title, buyin, pot_bonus)
            VALUES ('%s', %d, %d)";
  $result = db_query($sql, $rec->title, $rec->buyin, $rec->pot_bonus);
  if (!$result) return $result;
  return db_last_insert_id(db_prefix_tables('{plm_payset}'), 'payid');  
  
}
function _plm_update_payset($rec) {
  $sql = "UPDATE {plm_payset} SET title='%s', buyin=%d, pot_bonus=%d WHERE payid=%d";
  $res = db_query($sql, $rec->title, $rec->buyin, $rec->pot_bonus, $rec->payid);
  return $res;
}
function _plm_delete_payset($payid) {
  _plm_delete_payrules($payid);
  $sql = "DELETE FROM {plm_payset} WHERE payid=%d";
  $result = db_query($sql, $payid);
  return db_fetch_object($result);
}

function _plm_get_payrules($payid, $num_players=NULL) {
  $where_phrase = "WHERE payid=%d";
  if ($num_players != NULL)
    $where_phrase .= " AND num_players=%d";
  $sql = "SELECT * from {plm_payrule} $where_phrase order by num_players, place";
  $result = db_query($sql, $payid, $num_players);
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list; 
}

function _plm_get_payrule($payid, $num_players, $place) {
  $sql = "SELECT * from {plm_payrule} WHERE payid=%d AND num_players=%d and place=%d";
  $result = db_query($sql, $payid, $num_players, $place);
  return db_fetch_object($result);
}

function _plm_delete_payrule($payid, $num_players, $place) {
  $sql = "DELETE FROM {plm_payrule} WHERE payid=%d AND num_players=%d and place=%d";
  $result = db_query($sql, $payid, $num_players, $place);
  return db_fetch_object($result);
}
function _plm_delete_payrules($payid, $num_players=0) {
  $sql = "DELETE FROM {plm_payrule} WHERE payid=%d";
  if ($num_players) $sql .= " AND num_players=%d";
  $result = db_query($sql, $payid, $num_players);
  return db_fetch_object($result);
}

function _plm_insert_payrule($payid, $num_players, $place, $amount) {
  $oldrec = _plm_get_payrule($payid, $num_players, $place);
  if ($oldrec) return _plm_update_payrule($payid, $num_players, $place, $amount);
  $sql = "INSERT INTO {plm_payrule} (payid, num_players, place, amount) VALUES (%d, %d, %d, %d)";
  $result = db_query($sql, $payid, $num_players, $place, $amount);
  return $result;
}

function _plm_update_payrule($payid, $num_players, $place, $amount) {
  $sql = "UPDATE {plm_payrule} SET amount=%d 
            WHERE payid=%d AND num_players=%d AND place=%d";
  $result = db_query($sql, $amount, $payid, $num_players, $place );
  return $result;
}