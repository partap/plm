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

function _plm_rules_default_buyin($sid) {
  return variable_get('plm_default_buyin', 10);
}


// Default point distribution for a event 
// Could be dependent on ruleset for season, etc, but hardcoded for now
function _plm_rules_default_points($event, $place) {
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


function _plm_rules_default_payout($event, $place) {
  $num_players = $event->num_players;
  $buyin = $event->buyin;
  $pot_total  = $event->pot_start + $num_players*$buyin;
  if ($pot_total <= 0) return 0;
  $pot_left = $pot_total;
  if ($buyin == 0) $buyin = $pot_total/$num_players;
  $round_units = $buyin/2;
  for ($i=1;$i<=$place;$i++){
    $remainder = $pot_left % ($round_units*2);
    // don't give last 2 places the same payout
      if ($pot_left == $round_units*2) $remainder = $round_units*2;
    $pot_left -= $remainder;
    $pot_left /= 2;
    $payout = $pot_left + $remainder;
  }
  return $payout;
    
}