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
            var samples = new Uint32Array(payload.buffer.slice(8, 12));
            var samples = Number(samples[0]);
            var frequencies = new Uint32Array(payload.buffer.slice(12, 12 + samples * 4));
            var powers = new Float32Array(payload.buffer.slice(12 + samples * 4, 12 + samples * 4 + samples * 4));
            var bandwidth = frequencies[frequencies.length - 1] - frequencies[0];
            var center = (frequencies[0] + Math.round(bandwidth / 2));
            var id = 'waterfall_' + frequencies[0].toString() + "_" + frequencies[frequencies.length - 1].toString() + "samples: " + frequencies.length;
            var spectrum;

            console.log(dt.toLocaleString('en-GB'));
            console.log('spectrogram', frequencies[0], frequencies[frequencies.length - 1], id);

            if (spectrums.has(id)) {
                spectrum = spectrums.get(id);
            }
            else {
                console.log('create new');
                createWaterfall(frequencies[0], frequencies[frequencies.length - 1], id);
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
