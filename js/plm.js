$(document).ready(
    function() {

    $('#payouts td.edit').editable('/admin/plm/ajax/payout/save', {
        indicator : '<img src="/' + Drupal.settings.plm.modulePath + '/img/indicator.gif">',
        loadurl   : '/admin/plm/ajax/payout/load',
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
});

function add_entrant(gid, pid) {
    $.ajax({
        type: "GET",
        url: "/plm/ajax/add_entrant",
        data: {
            gid: gid,
            pid: pid
        },
        success: function(msg) {
            //alert(msg);
            var id='#avail-'+pid;
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
    $.ajax({
        type: "GET",
        url: "/plm/ajax/del_entrant",
        data: {
            eid: eid,
        },
        success: function(msg) {
            var idstr = msg.match(/id=\"[^\"]+\"/);
            if (!idstr) {
                alert('Error: ' + msg);
                return;
            }
            var prow = $('#'+idstr[0].split('"')[1]);
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
        }
    });
}
