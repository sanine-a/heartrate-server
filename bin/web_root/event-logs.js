$(document).ready( () => {
    $.post('/post',
           { callback: 'getLogList' },
           (data) => { console.log(data);
                       let logArray = JSON.parse(data);
                       for (let i=0; i<logArray.length; i++) {
                           let fileName = logArray[i];
                           let downloadHref = `/logs/${fileName}`;
                           $('#logs').append(`<li>${fileName} [<a href="view.html?log=${fileName}">view</a>] [<a href="#" onclick="console.log(getLogFile('${fileName}'))">download</a>]</li>`);
                       }
                     });
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

function getLogFile(fileName)
{
    let fileString;
    $.post('/post',
           { callback: 'getLogFile',
             file: fileName },
           (data) => download(fileName, data));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function download(filename, text) {
  var element = document.createElement('a');
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
  element.setAttribute('download', filename);

  element.style.display = 'none';
  document.body.appendChild(element);

  element.click();

  document.body.removeChild(element);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
