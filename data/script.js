// create global http instance for AJAX requests
const xhttp = new XMLHttpRequest();

// legacy function to handle checkbox change events 
function toggleCheckbox(element) {
    if (element.checked) {
        xhttp.open("GET", "/gpio_update?output=" + element.id + "&state=1");
    } else {
        xhttp.open("GET", "/gpio_update?output=" + element.id + "&state=0");
    }
    xhttp.send();
}

function fetchAndDisplayDeviceInfo() {
    console.log("fetching device info stats ...");
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            // display the receceived json encoded device info
            let response = JSON.parse(this.responseText);
            document.getElementById("device-name").innerHTML = response["device-name"];
            document.getElementById("uptime").innerHTML = response["uptime"];
            document.getElementById("wifi-ssid").innerHTML = response["wifi-ssid"];
            document.getElementById("ipv4-address").innerHTML = response["ipv4-address"];
            document.getElementById("mac-address").innerHTML = response["mac-address"];
            document.getElementById("hostname").innerHTML = response["hostname"];
            document.getElementById("cloud-connection-status").innerHTML = response["cloud-connection-status"];
            // ...
        }
    }
    xhttp.open("GET", "/device_info");
    xhttp.send();
}

function fetchAndDisplaySettings() {
    console.log("fetching settings ...");
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            // display the receceived json encoded device info
            let response = JSON.parse(this.responseText);
            document.getElementById("token").innerHTML = response["token"];
            // ...
        }
    }
    xhttp.open("GET", "/settings");
    xhttp.send();
}

// method to update all displayed sensor measurements at once by passing a json object
function setAllDisplayedMeasurements(measurements) {
    console.log("updating measurment displays ...");
    document.getElementById("window-state").innerHTML = measurements['window-state'];
    document.getElementById("air-temperature").innerHTML = measurements['air-temperature'];
    document.getElementById("air-humidity").innerHTML = measurements['air-humidity'];
    document.getElementById("dummy").innerHTML = measurements['dummy'];
    // ...
}

// setup periodic background polling of air temperature using AJAX
let pollingPeriod = 4000;
setInterval(function () {
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {

            // display the receceived json encoded measurements on the webpage
            let response = JSON.parse(this.responseText);
            setAllDisplayedMeasurements(response);
        }
    };
    xhttp.open("GET", "/sensor_read");
    xhttp.send();
}, pollingPeriod);
