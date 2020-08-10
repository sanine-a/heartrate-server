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
    $('#fileUploadInput').change( () => { let files = document.getElementById('fileUploadInput').files;
                                          if (files.length > 0) {
                                              uploadFile(files[0]);
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

function pickFile()
{
    $('#fileUploadInput').trigger("click");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function uploadFile(file)
{
    var reader = new FileReader();
    reader.readAsDataURL(file);
    reader.onload = function () {
        $.post('/post',
               { callback: 'uploadAudio',
                 audio: reader.result.replace('data:audio/x-wav;base64,', '') });
   };
   reader.onerror = function (error) {
     console.log('Error: ', error);
   };
}
