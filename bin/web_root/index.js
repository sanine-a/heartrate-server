console.log('hello from index.js');

let plot = document.getElementById('plot');
Plotly.newPlot( plot,
                    [{ x: [1, 2, 3, 4, 5],
                       y: [1, 4, 9, 16, 25] }],
                    { margin: { t: 0 },
                      staticPlot: true, } );
