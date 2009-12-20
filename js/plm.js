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
    });