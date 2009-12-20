$(document).ready(
    function() {
	$('.edit').editable('/admin/plm/ajax/payout/save', {
	    type   : 'text',
	    loadurl: '/admin/plm/ajax/payout/load'
	});
    });