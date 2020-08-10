let plot;
const MAX_DATA_POINTS = 8192;
let lastIndex = 0;
let status = "Arduino not connected";

$(document).ready( () => {
    plot = document.getElementById('plot');
    let basic = [];
    for (let i=0; i<MAX_DATA_POINTS; i++) {
        basic.push(0);
    }
    Plotly.newPlot( plot,
                    [{ y: basic }],
                    { margin: { t: 0 } },
                    { staticPlot: true, });
    //window.setInterval(updatePlot, 500);
    window.setInterval(updateStatus, 500);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updateStatus()
{
    $.post('/post',
           { callback: 'checkStatus' },
           (data) => { status = data; });
    $('#status').text(status);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updatePlot()
{
    if (status !== "Connected, leads on")
        return;
    $.post('/post',
           { callback: 'getData',
             signal: 0,
             index: lastIndex,
           },
           (data) => { const dataArray = JSON.parse(data);
                       lastIndex += dataArray.length;
                       Plotly.extendTraces(plot,
                                           { y: [dataArray] },
                                           [0], MAX_DATA_POINTS);
                     }
          );
}

