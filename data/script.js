// legacy function to handle checkbox change events 
function toggleCheckbox(element) {
    let xhr = new XMLHttpRequest();
    if (element.checked) { xhr.open("GET", "/gpio_update?output=" + element.id + "&state=1", true); }
    else { xhr.open("GET", "/gpio_update?output=" + element.id + "&state=0", true); }
    xhr.send();
}

// setup periodic background polling of air temperature using AJAX
let pollingPeriod = 10000;
setInterval(function () {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("temperature").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
}, pollingPeriod);

// setup periodic background polling of air humidity using AJAX
setInterval(function () {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("humidity").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
}, pollingPeriod);

// setup periodic background polling of window sensor using AJAX
pollingPeriod = 3000;
setInterval(function () {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("window-state").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/window_state", true);
    xhttp.send();
}, pollingPeriod);