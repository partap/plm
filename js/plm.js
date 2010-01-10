$(document).ready(
    function() {

	$('#payouts td.edit').editable('/admin/plm/ajax/payout/save', {
        indicator : '<img src="/' + Drupal.settings.plm.modulePath + '/img/indicator.gif">',
	    loadurl   : '/admin/plm/ajax/payout/load',
	    type      : 'text',
	    callback : function(value, settings) {
	        //console.log('callback:');
		    //console.log('this:');
		    //console.log(this);
		    //console.log('value: ' + value.toString());
	        var row = $(this.parentNode);
		    var vals = row.children('td.rowval');
		    var pot = row.children('.rowtotal').html();
		    //var total = parseInt(value);
		    var total = 0;
		    for (var i=0;i<vals.length;i++) {
		        //console.log(vals[i]);
		        //console.log(vals[i].innerHTML);
		        total += parseInt(vals[i].innerHTML);
		    }
		    var potRemaining = pot - total;
		    var rowchecksum = row.children('.rowchecksum');
		    if (potRemaining)
		        rowchecksum.html(potRemaining);
	        else
		        rowchecksum.html('');
		    //console.log('expected: ' + pot);
		    //console.log('total: ' + total);
        }
	});

	$('#plm-entrants-form');
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
            //alert(msg);
            var idstr = msg.match(/id="[^"]+"/);
            if (!idstr) {
                alert('Error: ' + msg);
                return;
            }
            var a = idstr[0].split('"');
            var prowid = a[1];
            var prow = $('#'+prowid);
            prow.removeClass('not-avail');

            var delid='#ent-'+eid;
            $(delid).remove();
            $('#plm-entrants-list > tbody > tr').each(function(index) {
                this.cells[0].innerHTML=index+1;
            });

        },
        error: function(xhr,status,errorThrown){
            alert('Server Error: ' + status + ',' + errorThrown);
        }
    });
}
