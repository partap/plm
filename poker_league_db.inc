<?php
function _get_poker_player($pid) {
  $sql = "SELECT * from {poker_league_players} WHERE pid=%d";
  return db_fetch_object(db_query($sql, $pid));
}

function _get_num_players() {
  $sql = "SELECT count(*) FROM {poker_league_players}";
  return db_result(db_query($sql));
}