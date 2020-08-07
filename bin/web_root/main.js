let plot;
const MAX_DATA_POINTS = 8192;
let lastIndex;

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
    window.setInterval(updatePlot, 50);
});

function updatePlot() {
    $.post('/post',
           { callback: 'getData',
             signal: 0,
             index: lastIndex,
           },
           (data) => { console.log(data);
                       const dataArray = JSON.parse(data);
                       lastIndex += dataArray.length;
                       Plotly.extendTraces(plot,
                                           { y: [dataArray] },
                                           [0], MAX_DATA_POINTS);
                     }
          );
}

