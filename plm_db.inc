<?php


function _get_num_plm_players() {
  $sql = "SELECT count(*) FROM {plm_player}";
  return db_result(db_query($sql));
}
function _get_plm_player($pid) {
  $sql = "SELECT * from {plm_player} WHERE pid=%d";
  return db_fetch_object(db_query($sql, $pid));
}
function _get_plm_player_by_name($name) {
  $sql = "SELECT * FROM {plm_player}
            WHERE lower(name)=lower('%s')";
  $res = db_fetch_object(db_query($sql, $name));
  return $res;
}

function _get_plm_players($sort_phrase, $where_phrase = '') {
  if (!$sort_phrase) $sort_phrase = "ORDER BY name";
  $sql = "SELECT p.pid, p.name, SUM(points) AS points, SUM(cash) AS cash, COUNT(points) AS places FROM {plm_player} p LEFT JOIN {plm_result} r ON p.pid=r.pid " . $where_phrase . "GROUP BY p.pid, p.name " . $sort_phrase . " NULLS LAST"; 
  $result = db_query($sql);
  $list = array();
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list;
}

function _get_plm_standings($sid, $limit = NULL) {
  $sort_phrase = "ORDER BY points DESC, cash DESC, places DESC, name";
  if ($limit) $sort_phrase .= " LIMIT $limit";
  $sql = "SELECT p.pid, p.name, e.sid,
      SUM(points) AS points, SUM(cash) AS cash, COUNT(points) AS places 
    FROM {plm_player} p LEFT JOIN {plm_result} r ON p.pid=r.pid 
    JOIN {plm_event} e ON r.eid=e.eid
    GROUP BY e.sid,p.pid, p.name " . $sort_phrase . " NULLS LAST"; 
  $result = db_query($sql);
  $list = array();
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list;
}

function _insert_plm_player($player) {
  $sql = "INSERT INTO {plm_player} (name) VALUES ('%s')";
  if (!db_query($sql, $player->name)) return FALSE;
  return db_last_insert_id(db_prefix_tables('{plm_player}'), 'pid');
}

function _update_plm_player($player) {
  $sql = "UPDATE {plm_player} SET name='%s'
          WHERE pid=%d";
  return db_query($sql, $player->name, $player->pid);
}

function _delete_plm_player($pid) {
  $sql = "DELETE FROM {plm_player} where pid=%d";
  return db_query($sql, $pid);
}

function _insert_plm_season($season) {
  $res =  db_query("INSERT INTO {plm_season} (title, start_date, end_date) 
                    VALUES ('%s', %d, %d)", 
                   $season->title, $season->start_date, $season->end_date);
  if (!$res) return $res;
  return db_last_insert_id(db_prefix_tables('{plm_season}'), 'eid');
  
}
function _update_plm_season($season) {
  return db_query("UPDATE {plm_season} 
                   SET title='%s', start_date=%d, end_date=%d 
                   WHERE sid = %d", 
                  $season->title, $season->start_date, $season->end_date);
}
function _delete_plm_season($sid) {
  // Delete events, results from the season too..
  $res = db_query("DELETE FROM {plm_result} WHERE eid IN 
                   (SELECT eid FROM {plm_event} WHERE sid=%d)", $sid);
  if (!$res) return $res;
  $res = db_query("DELETE FROM {plm_event} where  sid=%d", $sid);
  if (!$res) return $res;
  $res = db_query("DELETE FROM {plm_season} where sid=%d", $sid);
  return $res;
}
function _get_plm_current_season() {
  // Select newest season by default
  $sql = "SELECT s.*, NULLIF(s.end_date,0) AS end_dt 
    FROM {plm_season} s 
    ORDER BY end_dt DESC, s.start_date DESC LIMIT 1";
  return db_fetch_object(db_query($sql));
}

function _get_plm_seasons() {
  $sql = "SELECT *, NULLIF(s.end_date,0) AS end_dt
    FROM {plm_season} s 
    ORDER BY end_dt DESC, s.start_date DESC";
  $result = db_query($sql);
  while ($rec = db_fetch_object($result)) {
    $res[] = $rec;
  }
  return $res;
}

function _get_plm_season_by_title($title) {
  $sql = "SELECT * FROM {plm_season}
            WHERE lower(title)=lower('%s')";
  $res = db_fetch_object(db_query($sql, $title));
  return $res;
}

function _get_plm_season($sid) {
$sql = "SELECT *, NULLIF(s.end_date,0) AS end_dt
    FROM {plm_season} s
    WHERE s.sid=%d";
  return  db_fetch_object(db_query($sql, $sid));
}

function _get_num_plm_events($sid) {
  $sql = "SELECT count(*) as num_events 
          FROM {plm_event}
          WHERE sid=%d";
  return  db_result(db_query($sql, $sid));
}

function _get_plm_events($sid, $sort_phrase=NULL) {
  if (!$sort_phrase) $sort_phrase='ORDER BY g.event_date';
  
  $sql = "SELECT g.eid, g.title, p.name as host, g.event_date, g.hid, 
            g.num_players, g.finished
          FROM {plm_event} g 
          LEFT JOIN {plm_player} p ON p.pid=g.hid 
          WHERE g.sid=%d
          $sort_phrase";
  $result = db_query($sql, $sid);
  while ($row = db_fetch_object($result)){
    //if (!$row->host) $row->host = ' - ';
    $list[] = $row;
  }
  return $list;
}
function _get_plm_event($eid) {
  $sql = "SELECT * from {plm_event} WHERE eid=%d";
  return db_fetch_object(db_query($sql, $eid));
}
function _get_latest_plm_event($sid) {
    $latest_events = _get_plm_events($sid, "ORDER BY event_date LIMIT 1");
    return $latest_events[0];
}
function _get_plm_event_by_title($title, $sid) {
  $sql = "SELECT * from {plm_event} WHERE title=lower('%s') and sid=%d";
  return db_fetch_object(db_query($sql, $title, $sid));
}

// return id of new record or FALSE if error
function _insert_plm_event($rec) {
  $res = db_query("INSERT INTO {plm_event} 
            (title, sid, event_date, hid, num_players, pot_start, buyin, finished) 
            VALUES ('%s', %d, %d, %d, %d, %d, %d, %d)", 
                  $rec->title, $rec->sid,
                  $rec->event_date, $rec->hid, $rec->num_players, 
                  $rec->pot_start, $rec->buyin, $rec->finished);
  if (!$res) return $res;
  return db_last_insert_id(db_prefix_tables('{plm_event}'), 'eid');
}
function _update_plm_event($rec) {
  return db_query("UPDATE {plm_event} 
              SET title='%s', event_date=%d, 
                sid=%d, hid=%d, num_players=%d,
                pot_start=%d, buyin=%d, finished=%d
              WHERE eid = %d", 
                  $rec->title, $rec->event_date, 
                  $rec->sid, $rec->hid, $rec->num_players,
                  $rec->pot_start, $rec->buyin, $rec->finished,
                  $rec->eid);
}

function _delete_plm_event($eid) {
  $res = db_query("DELETE FROM {plm_result} WHERE eid=%d", $eid); 
  if (!$res) return $res;
  return db_query("DELETE FROM {plm_event}  WHERE eid=%d", $rec->eid);
}


function _get_plm_result($eid, $place){
  $sql = "SELECT eid, place, pid, p.name, points, cash 
          FROM {plm_result} r
          LEFT JOIN {plm_player} p USING(pid)
          WHERE r.eid=%d and r.place=%d";
  $res = db_fetch_object(db_query($sql, $eid, $place));
  return $res;
}

function _get_plm_results($eid){
  $sql = "SELECT eid, place, pid, p.name, points, cash 
          FROM {plm_result} r
          LEFT JOIN {plm_player} p USING(pid)
          WHERE r.eid=%d
          ORDER BY place";
  $result = db_query($sql, $eid);
  while ($row = db_fetch_object($result)) {
    $res[$row->place] = $row;
  }
  return $res;
}

function _get_plm_results_by_player($pid, $sid=NULL) {
  $sql = "SELECT s.sid, g.eid, g.title, g.hid, p.name AS host, r.place, r.points, r.cash
          FROM {plm_result} r 
          LEFT JOIN ({plm_event} g LEFT JOIN  {plm_player} p ON g.hid=p.pid) USING (eid)
          LEFT JOIN {plm_season} s USING (sid)
          WHERE r.pid=%d ";
  if ($sid) $sql .= "AND g.sid=%d ";
  $sql .= "ORDER BY s.start_date, g.event_date";
  $result = db_query($sql, $pid, $sid);
  while ($row = db_fetch_object($result)) {
    $res[] = $row;
  }
  return $res;
  
  
}

function _insert_plm_result($rec) {
  $sql = "INSERT INTO {plm_result} (eid, place, pid, points, cash)
            VALUES (%d, %d, %d, %d, %d)";
  return db_query($sql, $rec->eid, $rec->place, $rec->pid, $rec->points, $rec->cash);
}

function _update_plm_result($rec) {
    $sql = "UPDATE {plm_result} 
          SET pid=%d, points=%d, cash=%d
          WHERE eid=%d AND place=%d";
    $res = db_query($sql,
             $rec->pid, $rec->points, $rec->cash,
             $rec->eid, $rec->place);
    return $res;
}

function _delete_plm_result($eid, $place) {
    $sql = "DELETE FROM {plm_result} where eid=%d AND $place=%d";
    return db_query($sql, $eid,$place);
}