<?php
// output place string. eg "1st" for 1, "2nd" for 2, etc...
function _plm_format_place($place) {
  if (!is_numeric($place)) return $place;
  if ($place > 3 && $place < 21) return $place . 'th';
  switch ($place%10){
  case 1:
    $suffix = 'st';
    break;
  case 2:
    $suffix = 'nd';
    break;
  case 3:
    $suffix = 'rd';
    break;
  default:
    $suffix = 'th';
  }
  return $place . $suffix;
}
function _plm_format_time_elapsed($seconds, $short_form=false) {
    $days = (int) ($seconds / (60*60*24));
    if ($days < 7){
        $str1 = $short_form ? 'Day' : 'day';
        $strn = $short_form ? 'Dys' : 'days';
        return format_plural($days, "1 $str1",  "@count $strn");
    }

    $weeks = (int) ($days / 7);
    if ($weeks < 8) {
        $str1 = $short_form ? 'Wk' : 'week';
        $strn = $short_form ? 'Wks' : 'weeks';
        return format_plural($weeks, "1 $str1",  "@count $strn");
    }
    $months = (int) ($weeks / 4);
    if ($months < 12) {
        $str1 = $short_form ? 'Mo' : 'month';
        $strn = $short_form ? 'Mos' : 'months';
        return format_plural($months, "1 $str1",  "@count $strn");
    }
    $years  = (int) ($weeks / 52);
    $months %= 12;

    $elapsed = format_plural($years, '1 Yr', '@count Yr');
    if ($months) $elapsed .= " $months m";
    return $elapsed;    
}

function _plm_rules_default_buyin($sid) {
  return variable_get('plm_default_buyin', 10);
}


// Default point distribution for a game 
// Could be dependent on ruleset for season, etc, but hardcoded for now
function _plm_rules_default_points($game, $place) {
  switch ($place){
  case 1:
    return 10;
  case 2:
    return 7;
  case 3:
    return 5;
  case 4:
    return 3;
  case 5:
    return 1;
  default:
    return 0;
  }
}


function _plm_rules_default_payout($game, $place) {
  $num_players = $game->num_players;
  if ($game->payid){
    $payrule = _plm_get_payrule($game->payid, $num_players, $place);
    if ($payrule) return $payrule->amount;
  }
  $buyin = $game->buyin;
  $pot_total  = $game->pot_start + $num_players*$buyin;
  if ($pot_total <= 0) return 0;
  $pot_left = $pot_total;
  if ($buyin == 0) $buyin = (int)($pot_total/$num_players);
  
  if ($buyin == (int)$buyin){
    $round_units = 1.00;
    //$round_units = (int)($buyin/2);
  }
  $min_payout = (int)($buyin/2);
  for ($i=1;$i<=$place;$i++){
    $remainder = $pot_left % ($round_units*2);
    // don't give last 2 places the same payout
      if ($pot_left == $round_units*2) $remainder = $round_units*2;
    $pot_left -= $remainder;
    if ($pot_left > $min_payout)
      $pot_left /= 2;
    $payout = $pot_left + $remainder;
  }
  return $payout;
    
}

function _plm_get_est_cutoff_points($plm_season, $tenth) {
  $num_games_finished = _plm_get_num_games($plm_season->sid, "finished != 0");
  $num_games_total    = _plm_get_num_games($plm_season->sid);
  return round($tenth*$num_games_total/$num_games_finished);
}

function _plm_get_eoy_est($plm_season, $eoy_total) {
  $num_games_finished = _plm_get_num_games($plm_season->sid, "finished != 0");
  $num_games_total    = _plm_get_num_games($plm_season->sid);
  return round($eoy_total*$num_games_total/$num_games_finished);
}

// Temporarily hardcoded
function _plm_get_eoy_total($plm_season) {
  //$num_games_finished = _plm_get_num_games($plm_season->sid, "finished != 0");
  $games = _plm_get_games($plm_season->sid);

  $total = 0;
  foreach ($games as $g) {
    $total += $g->eoy_rake;
  }
  return $total;
}

// Temporarily hardcoded
//function _plm_get_season_num_games($plm_season) {
//   switch ($plm_season->title) {
//     case 'Season 5': 
//     case 'Season 6': 
//     case 'Season 7': 
//       return 26;
//     default:
//       return 26;
//   }
//}

