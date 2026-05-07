function decodeUplink(input) {
    var bytes = input.bytes;
    var data = {};

    // 1. Status (1 Bit)
    // 0 = Normal Cycle, 1 = Button Pressed, 2 = Fall Detected
    data.status = bytes[0];

    if (data.status === 1) {
        data.event = "Button Pressed";
    } else if (data.status === 2) {
        data.event = "Fall Detected";
    } else {
        data.event = "Normal Update";
    }

    function bytesToFloat(bytes, start) {
        var bits = (bytes[start + 3] << 24) | (bytes[start + 2] << 16) | (bytes[start + 1] << 8) | bytes[start];
        var sign = (bits >>> 31 === 0) ? 1.0 : -1.0;
        var e = (bits >>> 23) & 0xff;
        var m = (e === 0) ? (bits & 0x7fffff) << 1 : (bits & 0x7fffff) | 0x800000;
        return sign * m * Math.pow(2, e - 150);
    }

    data.latitude = bytesToFloat(bytes, 1);

    data.longitude = bytesToFloat(bytes, 5);

    data.altitude = 10;

    data.hour = bytes[9];
    data.minute = bytes[11];
    data.second = bytes[13];
    data.centisecond = bytes[15];

    data.time_string =
        (data.hour < 10 ? '0' : '') + data.hour + ":" +
        (data.minute < 10 ? '0' : '') + data.minute + ":" +
        (data.second < 10 ? '0' : '') + data.second;

    return {
        data: data,
        warnings: [],
        errors: []
    };
}