<?php


function _get_num_players() {
  $sql = "SELECT count(*) FROM {poker_league_players}";
  return db_result(db_query($sql));
}
function _get_poker_player($pid) {
  $sql = "SELECT * from {poker_league_players} WHERE pid=%d";
  return db_fetch_object(db_query($sql, $pid));
}
function _get_poker_player_by_name($name) {
  $sql = "SELECT * FROM {poker_league_players}
            WHERE lower(name)=lower('%s')";
  $res = db_fetch_object(db_query($sql, $name));
  return $res;
}

function _get_poker_players($sort_phrase) {
  if (!$sort_phrase) $sort_phrase = "ORDER BY name";
  $sql = "SELECT p.pid, p.name, SUM(points) AS points, SUM(cash) AS cash, COUNT(points) AS places FROM {poker_league_players} p LEFT JOIN {poker_league_results} r ON p.pid=r.pid GROUP BY p.pid, p.name " . $sort_phrase . " NULLS LAST"; 
  $result = db_query($sql);
  $list = array();
  while ($row = db_fetch_object($result)){
    $list[] = $row;
  }
  return $list;
}

function _insert_poker_player($player) {
  $sql = "INSERT INTO {poker_league_players} (name) VALUES ('%s')";
  return db_query($sql, $player->name);
}

function _update_poker_player($player) {
  $sql = "UPDATE {poker_league_players} SET name='%s'
          WHERE pid=%d";
  return db_query($sql, $player->name, $player->pid);
}

function _delete_poker_player($pid) {
    $sql = "DELETE FROM {poker_league_players} where pid=%d";
    db_query($sql, $pid);
}

function _insert_poker_league_season($node) {
  $start_dt = strtotime($node->start_date);
  $end_dt = 'NULL';
  if ($node->end_date) $end_dt = strtotime($node->end_date);
  return db_query("INSERT INTO {poker_league} (nid, vid, start_date, end_date) 
            VALUES (%d, %d, %d, %d)", 
                  $node->nid, $node->vid, $start_dt, $end_dt);
  
}
function _update_poker_league_season($node) {
  $start_dt = strtotime($node->start_date);
  $end_dt = 'NULL';
  if ($node->end_date) $end_dt = strtotime($node->end_date);
  db_query("UPDATE {poker_league} 
            SET start_date=%d, end_date= %d 
            WHERE nid = %d", 
           $start_dt, $end_dt, $node->nid);
  
}
function _delete_poker_league_season($sid) {
  db_query("DELETE FROM {poker_league_game} where sid=%d", $sid);
  db_query("DELETE FROM {poker_league} where nid=%d", $sid);
  // Delete game results, comments from the season too?
}

function _get_poker_league_season_ext($node) {
  $additions = db_fetch_object(
    db_query(
      "SELECT start_date, end_date FROM {poker_league} WHERE nid=%d AND vid=%d",
      $node->nid, $node->vid));
  
  return $additions;
}

function _get_num_games($sid) {
  $sql = "SELECT count(*) as num_games from  {node} n 
          LEFT JOIN {poker_league_game} g ON g.nid=n.nid AND g.vid=n.vid 
          WHERE sid=%d";
  return  db_result(db_query($sql, $sid));
}
function _get_poker_game_ext($node) {
  $additions = db_fetch_object(
    db_query(
      "SELECT g.*, s.title as season_title
       FROM {poker_league_game} g
       LEFT JOIN {node} s
       ON g.sid=s.nid
       WHERE g.nid=%d AND g.vid=%d",
      $node->nid, $node->vid));
  
  return $additions;
}
function _get_poker_games($sid) {
  $sql = "SELECT n.title, p.name as host, g.game_date, n.nid, g.hid, 
            g.num_players
          FROM  {node} n 
          LEFT JOIN {poker_league_game} g ON g.nid=n.nid AND g.vid=n.vid
          LEFT JOIN {poker_league_players} p ON p.pid=g.hid 
          WHERE g.sid=%d
          ORDER BY g.game_date";
  $result = db_query($sql, $sid);
  $list = array();
  while ($row = db_fetch_object($result)){
    if (!$row->host) $row->host = 'TBA';
    $list[] = $row;
  }
  return $list;
}
function _insert_poker_game($node) {
  return db_query("INSERT INTO {poker_league_game} 
            (nid, vid, sid, game_date, hid, num_players, pot_start, buyin) 
            VALUES (%d, %d, %d, %d, %d, %d, %d, %d)", 
                  $node->nid, $node->vid, $node->sid,
                  $node->game_date, $node->hid, $node->num_players, 
                  $node->pot_start, $node->buyin);
  
}
function _update_poker_game($node) {
  return db_query("UPDATE {poker_league_game} 
              SET game_date=%d, 
                sid=%d, hid=%d, num_players=%d
                pot_start=%d, buyin=%d
              WHERE vid = %d", 
                  $node->game_date, 
                  $node->sid, $node->hid, $node->num_players,
                  $node->pot_start, $node->buyin,
                  $node->vid);
}

function _delete_poker_game($node) {
  db_query("DELETE FROM {poker_league_game} where nid=%d", $node->nid);
  // Also delete comments, etc?
}


function _get_poker_result($gid, $place){
  $sql = "SELECT gid, place, pid, p.name, points, cash 
          FROM {poker_league_results} r
          LEFT JOIN {poker_league_players} p USING(pid)
          WHERE r.gid=%d and r.place=%d";
  $res = db_fetch_object(db_query($sql, $gid, $place));
  return $res;
}

function _get_poker_results($gid){
  $sql = "SELECT gid, place, pid, p.name, points, cash 
          FROM {poker_league_results} r
          LEFT JOIN {poker_league_players} p USING(pid)
          WHERE r.gid=%d
          ORDER BY place";
  $result = db_query($sql, $gid);
  $res = array();
  
  while ($row = db_fetch_object($result)) {
    $res[$row->place] = $row;
  }
  return $res;
}

function _get_poker_results_by_player($pid, $sid) {
  $sql = "SELECT";

}

function _get_poker_league_current_sid() {
  // Select newest active season by default
  $sql = "SELECT n.nid FROM {node} AS n
    LEFT JOIN {poker_league} AS pl ON n.vid=pl.vid
    WHERE n.type='poker_league' AND pl.end_date >= %d
    ORDER BY pl.end_date DESC, pl.start_date DESC LIMIT 1";
  return db_result(db_query($sql, gmmktime()));
  
}