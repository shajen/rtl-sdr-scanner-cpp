'use strict';

function createWaterfall(start, stop, id) {
    var element = '<div class="col"><p class="text-center fs-3">' + start + " MHz - " + stop + " MHz" + '</p><div><canvas id="' + id + '" style="height: 100%; width: 100%"></canvas></div></div>';
    $("#waterfalls").append(element);
}

function main() {
    var spectrums = new Map();

    const socket = new WebSocket('ws://localhost:9999/');
    socket.addEventListener('open', function (event) {
        console.log('ws opened')
        socket.send({ 'command': 'authorize', 'key': '' });
    });

    socket.addEventListener('close', function (event) {
        console.log('ws closed')
    });

    socket.addEventListener('message', function (event) {
        var data = JSON.parse(event.data);

        if (data.type == 'spectrogram') {
            var powers = data.powers;
            var frequencies = data.frequencies;
            var bandwidth = frequencies[frequencies.length - 1] - frequencies[0];
            var center = (frequencies[0] + Math.round(bandwidth / 2));
            var id = 'waterfall_' + frequencies[0].toString() + "_" + frequencies[frequencies.length - 1].toString();
            var spectrum;

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
                window.addEventListener("keydown", function (e) {
                    spectrum.onKeypress(e);
                });
                spectrums.set(id, spectrum);
            }

            spectrum.addData(powers);
            spectrum.setCenterHz(center);
            spectrum.setSpanHz(bandwidth);
            spectrum.setRange(-60, 40);
        }
    });
}

window.onload = main;
