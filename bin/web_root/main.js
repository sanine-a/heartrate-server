let plot;
const MAX_DATA_POINTS = 8192;
let lastIndex = 0;
let status = "ADC not connected";

$(document).ready( () => {
    plot = document.getElementById('plot');
    let basic = [];
    for (let i=0; i<MAX_DATA_POINTS; i++) {
        basic.push(0);
    }
    window.setInterval(updateStatus, 500);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updateStatus()
{
    $.post('/post',
           { callback: 'checkStatus' },
           (data) => { status = data; });
    $('#status').text(`Current Status: ${status}`);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
