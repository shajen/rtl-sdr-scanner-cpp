'use strict';

var spectrums = new Map();
var client;

function createWaterfall(start, stop, id) {
    var element = '<div class="col"><p class="text-center fs-3">' + start + " MHz - " + stop + " MHz" + '</p><div><canvas id="' + id + '" style="height: 100%; width: 100%"></canvas></div></div>';
    $("#waterfalls").append(element);
}

function main() {
    $("#connect").click(connect);
    $("#hostname").val('test.mosquitto.org');
    $("#port").val('8080');
    $("#username").val('');
    $("#password").val('');
}

function connect() {
    spectrums.clear();
    if (client) {
        client.end();
    }
    $("#waterfalls").empty();

    if ($("#username").val() != '' && $("#password").val() != '') {
        client = mqtt.connect("mqtt://" + $("#hostname").val() + ":" + $("#port").val(), { username: $("#username").val(), password: $("#password").val(), rejectUnauthorized: false, clean: true, });
    }
    else {
        client = mqtt.connect("mqtt://" + $("#hostname").val() + ":" + $("#port").val(), { rejectUnauthorized: false, clean: true, });
    }


    client.subscribe("sdr/spectrogram");
    client.on('connect', function (event) {
        console.log('mqtt connected');
    })
    client.on('close', function (event) {
        console.log('mqtt disconnected');
    })
    client.on('message', function (topic, payload) {
        if (topic == 'sdr/spectrogram') {
            var timestamp = new BigUint64Array(payload.buffer.slice(0, 8));
            var timestamp = Number(timestamp[0]);
            var dt = new Date(timestamp)
            var tmp = new Uint32Array(payload.buffer.slice(8, 24));
            var begin_frequency = Number(tmp[0]);
            var end_frequency = Number(tmp[1]);
            var step_frequency = Number(tmp[2]);
            var samples = Number(tmp[3]);

            var powers = new Int8Array(payload.buffer.slice(24, 24 + samples));
            var bandwidth = end_frequency - begin_frequency;
            var center = (begin_frequency + Math.round(bandwidth / 2));
            var id = 'waterfall_' + begin_frequency.toString() + "_" + end_frequency.toString() + "samples: " + powers.length;
            var spectrum;

            console.log(dt.toLocaleString('en-GB'));
            console.log('spectrogram', begin_frequency, end_frequency, id);

            if (spectrums.has(id)) {
                spectrum = spectrums.get(id);
            }
            else {
                console.log('create new');
                createWaterfall(begin_frequency, end_frequency, id);
                spectrum = new Spectrum(
                    id, {
                    spectrumPercent: 40
                });
                // window.addEventListener("keydown", function (e) {
                //     spectrum.onKeypress(e);
                // });
                spectrum.setCenterHz(center);
                spectrum.setSpanHz(bandwidth);
                spectrum.setRange(-60, 40);
                spectrums.set(id, spectrum);
            }
            spectrum.addData(powers);
        }
    })
}

window.onload = main;
