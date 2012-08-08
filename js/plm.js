$(document).ready(
    function() {

        $('#payouts td.edit').editable(Drupal.settings.basePath+'admin/plm/ajax/payout/save', {
            indicator : '<img src="/' + Drupal.settings.plm.modulePath + '/img/indicator.gif">',
            loadurl   : Drupal.settings.basePath+'admin/plm/ajax/payout/load',
            type      : 'text',
            width     : 'auto',
            callback : function(value, settings) {
                var row = $(this.parentNode);
                var vals = row.children('td.rowval');
                var pot = row.children('.rowtotal').html();
                var total = 0;
                for (var i=0;i<vals.length;i++) {
                    var n = parseInt(vals[i].innerHTML);
                    if (!isNaN(n))
                        total += n;
                }
                var potRemaining = pot - total;
                var rowchecksum = row.children('.rowchecksum');
                if (potRemaining)
                    rowchecksum.html(potRemaining);
                else
                    rowchecksum.html('');
            }
        });
        
        $('#game-results .btn').live('click',function () {
            var pid = this.id.split('-')[1];
            var gid = $('#edit-gid').val();
            var place = ($('#game-results tbody tr').index(($(this).closest('tr')))) + 1;
            var url;
            if ($(this).hasClass('player-done')) {
                url = Drupal.settings.basePath + 'plm/ajax/game/player_done';
            }
            else if ($(this).hasClass('player-in')) {
                url = Drupal.settings.basePath + 'plm/ajax/game/player_in';
            }
            //alert('Click! ' + url);
            
            $.post(url, {
                gid: gid,
                pid: pid,
                place: place
            },function (d, s, xhr) {
                //$.jGrowl(d);
                //alert(d);
                $('#game-results').replaceWith(d);
            });

        });
    }
);

function add_entrant(gid, pid) {
    var url = Drupal.settings.basePath+"plm/ajax/add_entrant";
    $.ajax({
        type: "POST",
        url: url,
        data: {
            gid: gid,
            pid: pid
        },
        success: function(msg) {
            //alert(msg);
            var id='#pid-'+pid;
            //$(id).remove();
            $(id).addClass('not-avail');
            $('#plm-entrants-list tbody').append(msg);
            var num_entrants = $('#plm-entrants-list > tbody tr').length;
            $('#plm-entrants-list > caption').html(num_entrants + ' Entrants');
        },
        error: function(xhr,status,errorThrown){
            alert('Error: ' + status + ',' + errorThrown);
        }
    });
}

function del_entrant(eid) {
    var url = Drupal.settings.basePath+"plm/ajax/del_entrant";
    $.ajax({
        type: "POST",
        url: url,
        data: {
            eid: eid,
        },
        success: function(msg) {
            //var idstr = msg.match(/id=\"[^\"]+\"/);
            var idstr = 'pid-' + msg.pid
            if (!idstr) {
                //alert('Error: ' + msg);
                console.log('AJAX Error: ' + msg);
                return;
            }
            //var prow = $('#'+idstr[0].split('"')[1]);
            var prow = $('#'+idstr);
            prow.removeClass('not-avail');

            var delid='#ent-'+eid;
            $(delid).remove();
            $('#plm-entrants-list > tbody > tr').each(function(index) {
                this.cells[0].innerHTML=index+1;
            });
            var num_entrants = $('#plm-entrants-list > tbody tr').length;
            $('#plm-entrants-list > caption').html(num_entrants + ' Entrants');

        },
        error: function(xhr,status,errorThrown){
            alert('Server Error: ' + status + ',' + errorThrown);
            console.log('Server Error: ' + status + ',' + errorThrown);
        }
    });
}
