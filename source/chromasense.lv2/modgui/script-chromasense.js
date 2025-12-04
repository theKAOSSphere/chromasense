function (event) {

    function notename (note) {
        const notes = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
        return notes[Math.abs(Math.trunc(note)) % 12];
    }

    // NEW CENT VISUALIZATION LOGIC
    function cent_visualization (dpy, cent) {
        const meterWidth = 21; // Must be an odd number
        const centerIndex = Math.floor(meterWidth / 2);
        let meter = Array(meterWidth).fill('\u00A0'); // Use non-breaking spaces for empty slots

        let flatIndicator = event.icon.find('[mod-role=flat-indicator]');
        let sharpIndicator = event.icon.find('[mod-role=sharp-indicator]');
        let adjustDisplay = event.icon.find('[mod-role=adjust]');

        // Show/Hide flat/sharp indicators
        flatIndicator.css('opacity', cent < -2 ? '1' : '0');
        sharpIndicator.css('opacity', cent > 2 ? '1' : '0');

        let position;

        if (cent > -2 && cent < 2) {
            adjustDisplay.addClass('good');
            position = centerIndex;
        } else {
            adjustDisplay.removeClass('good');
            // Map cent value to meter position
            // Clamp value to +/- 50 cents
            let clampedCent = Math.max(-50, Math.min(50, cent));
            // The center of the 3-block meter will be at this position
            position = Math.round(centerIndex * (clampedCent / 50)) + centerIndex;
        }

        // Ensure the 3 blocks stay within bounds
        position = Math.max(1, Math.min(meterWidth - 2, position));
        
        // Place the three blocks
        meter[position - 1] = '\u2588';
        meter[position] = '\u2588';
        meter[position + 1] = '\u2588';

        // Set the text of the adjust display
        adjustDisplay.text(meter.join(''));
    }

    function handle_event (symbol, value) {
        const dpy = event.icon.find('[mod-role=tuner-display]');
        switch (symbol) {
            case 'freq':
                if (value < 20) {
                    dpy.addClass('nosignal');
                    event.icon.find('[mod-role=freq]').text("--- Hz");
                    event.icon.find('[mod-role=adjust]').text('\u2588').removeClass('good');
                    event.icon.find('[mod-role=flat-indicator]').css('opacity', '0');
                    event.icon.find('[mod-role=sharp-indicator]').css('opacity', '0');
                } else {
                    dpy.removeClass('nosignal');
                    event.icon.find('[mod-role=freq]').text(value.toFixed(1) + " Hz");
                }
                break;
            case 'octave':
                event.icon.find('[mod-role=octave]').text(Math.trunc(value));
                break;
            case 'note':
                event.icon.find('[mod-role=note]').text(notename(value));
                break;
            case 'cent':
                var sign = value >= 0 ? "+" : "";
                event.icon.find('[mod-role=cent]').text(sign + value.toFixed(1) + " ct");
                cent_visualization(dpy, value);
                break;
        }
    }

    if (event.type == 'start') {
        const ports = event.ports;
        for (let p in ports) {
            handle_event(ports[p].symbol, ports[p].value);
        }
    } else if (event.type == 'change') {
        handle_event(event.symbol, event.value);
    }
}
