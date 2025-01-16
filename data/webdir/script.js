// create global http instance for AJAX requests
const xhttp = new XMLHttpRequest();

function confirmSetDeviceName() {
    if (window.confirm("Do you really want to configure a new device? This will generate a new device entirely.")) {
        const element_form = /**@type {HTMLFormElement}*/ (document.getElementsById("dev_form"));
        console.log("sending new device name form");
        element_form.submit();
    }
}

function confirmRestart() {
    if (window.confirm("Do you really want to restart? This will soft restart the hardware.")) {
        const element_form = /**@type {HTMLFormElement}*/ (document.getElementsById("restart_form"));
        console.log("sending restarting request");
        element_form.submit();
    }
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
            document.getElementById("device-name").innerHTML = response["device-name"];
            // generate and display table of available networks
            var table_row_elements = [];
            var table_col_elements = [];
            var table_element = /**@type {HTMLTableElement}*/ (document.getElementById("wifi-networks-table"));
            available_network_names = response["available-wifi-networks"];

            for (var i = 0; i < available_network_names.length; i++) {
                table_row_elements[i] = /**@type {HTMLTableRowElement}*/ (document.createElement("tr"));

                table_col_elements[0 + i * 2] = /**@type {HTMLTableColElement}*/ (document.createElement("td"));
                table_col_elements[1 + i * 2] = /**@type {HTMLTableColElement}*/ (document.createElement("td"));
                table_col_elements[0 + i * 2].innerHTML = i;
                table_col_elements[1 + i * 2].innerHTML = available_network_names[i];

                table_row_elements[i].append(table_col_elements[0 + i * 2]);
                table_row_elements[i].append(table_col_elements[1 + i * 2]);

                table_element.append(table_row_elements[i]);
            }
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

function fetchAndDisplayCloudConnectionStatus() {
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            // display the receceived json encoded measurements on the webpage
            let response = JSON.parse(this.responseText);
            console.log("updating cloud connection status ...");
            document.getElementById("cloud-connection-status").innerHTML = response['cloud-connection-status'];
            document.getElementById("configured-hostname").innerHTML = response['configured-hostname'];
            if (response['configured-hostname'] != "") {
                document.getElementById("configured-hyperlink-message").innerHTML = "Make sure to save the new hyperlink as the old hyperlink will be reused for new devices";
                document.getElementById("configured-hyperlink").innerHTML = response['configured-hyperlink'];
            }
            document.getElementById("configured-hyperlink-redirect").setAttribute("href", response['configured-hyperlink']);
            // ...
        }
    };
    xhttp.open("GET", "/cloud_connection_status");
    xhttp.send();
}

// setup periodic background polling of sensor values using AJAX
let pollingPeriod = 4000;
setInterval(fetchAndDisplaySensorMeasurements, pollingPeriod);