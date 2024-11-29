// legacy function to handle checkbox change events 
function toggleCheckbox(element) {
    let xhr = new XMLHttpRequest();
    if (element.checked) { 
        xhr.open("GET", "/gpio_update?output=" + element.id + "&state=1", true); 
    } else { 
        xhr.open("GET", "/gpio_update?output=" + element.id + "&state=0", true); 
    }
    xhr.send();
}

// method to update all displayed sensor measurements at once by passing a json object
function setAllDisplayedMeasurements(measurements) {
    document.getElementById("window-state").innerHTML = measurements['window-state'];
    document.getElementById("air-temperature").innerHTML = measurements['air-temperature'];
    document.getElementById("air-humidity").innerHTML = measurements['air-humidity'];
    document.getElementById("dummy").innerHTML = measurements['dummy'];
    // ...
}

// setup periodic background polling of air temperature using AJAX
let pollingPeriod = 4000;
setInterval(function () {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {

            // display the receceived json encoded measurements on the webpage
            let response = JSON.parse(this.responseText);
            setAllDisplayedMeasurements(response);
        }
    };
    xhttp.open("GET", "/sensor_read", true);
    xhttp.send();
}, pollingPeriod);