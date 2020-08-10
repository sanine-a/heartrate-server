$(document).ready( () => {
    $.post('/post',
           { callback: 'getParams' },
           (data) => updateParams(JSON.parse(data)) );
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

function updateParams(params)
{
    $('#signal_lowpass').val(params.signal_lowpass);
    $('#derivative_lowpass').val(params.derivative_lowpass);
    $('#n_avg_samples').val(params.n_avg_samples);
    $('#n_max_samples').val(params.n_max_samples);
    $('#max_persistence_time').val(params.max_persistence_time);
    $('#max_scale').val(params.max_scale);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function submit()
{
    $.post('/post',
           { callback: 'updateParams',
             signal_lowpass: $('#signal_lowpass').val(),
             derivative_lowpass: $('#derivative_lowpass').val(),
             n_avg_samples: $('#n_avg_samples').val(),
             n_max_samples: $('#n_max_samples').val(),
             max_persistence_time: $('#max_persistence_time').val(),
             max_scale: $('#max_scale').val() });
    $.post('/post',
           { callback: 'getParams' },
           (data) => updateParams(JSON.parse(data)) );
}
           
             

