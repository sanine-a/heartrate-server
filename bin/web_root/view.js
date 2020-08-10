let graph;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

$.urlParam = function(name){
    var results = new RegExp('[\?&]' + name + '=([^&#]*)').exec(window.location.href);
    if (results==null) {
       return null;
    }
    return decodeURI(results[1]) || 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

$(document).ready( () => {
    let logFileName = $.urlParam('log');
    $('#title').text(`Plot of ${logFileName}`);
    $.post('/post',
           { callback: 'getLogFile',
             file: logFileName },
           (data) => buildPlot(data));
    window.setInterval(updateStatus, 500);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function buildPlot(data)
{
    let csv = data.replace('# ', '').replace(/[ ]/gi, ',');
    console.log(csv);
    graph = new Dygraph(
        document.getElementById('graph'),
        csv);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updateStatus()
{
    $.post('/post',
           { callback: 'checkStatus' },
           (data) => { status = data; });
    $('#status').text(`Current Status: ${status}`);
}


                   
