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

// updates device infos display in the background
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

// updates displayed device settings in the background
function fetchAndDisplaySettings() {
    console.log("fetching settings ...");
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            // display the receceived json encoded device info
            let response = JSON.parse(this.responseText);
            document.getElementById("token").innerHTML = response["token"];
            document.getElementById("device_name").innerHTML = response["device_name"];
            document.getElementById("customer_name").innerHTML = response["customer_name"];
            // ...
        }
    }
    xhttp.open("GET", "/settings");
    xhttp.send();
}

// updates displayed sensor measurements in the background
function fetchAndDisplaySensorMeasurements() {
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            // display the receceived json encoded measurements on the webpage
            let response = JSON.parse(this.responseText);
            console.log("updating measurment displays ...");
            document.getElementById("window-state").innerHTML = response['window-state'];
            document.getElementById("air-temperature").innerHTML = response['air-temperature'];
            document.getElementById("air-humidity").innerHTML = response['air-humidity'];
            // ...
        }
    };
    xhttp.open("GET", "/sensor_read");
    xhttp.send();
}

// setup periodic background polling of sensor values using AJAX
let pollingPeriod = 4000;
setInterval(fetchAndDisplaySensorMeasurements, pollingPeriod);