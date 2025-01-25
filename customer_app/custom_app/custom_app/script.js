const form = document.getElementById('uploadForm');
const firmwareAction = document.getElementById('firmwareAction');
const statusMessage = document.getElementById('statusMessageUpload');
const xhr = new XMLHttpRequest();
var sending = false;
form.addEventListener('submit', (event) => {
    event.preventDefault();
    if(sending) {
        xhr.abort();
        firmwareAction.innerHTML = "Upload";
        statusMessage.textContent = "";
        sending = false;
        return;
    }
    statusMessage.textContent = 'Uploading firmware... Please wait.';
    xhr.open('POST', '/ota');
    xhr.upload.addEventListener('progress', (event) => {
        if (event.lengthComputable) {
            const percentComplete = Math.round((event.loaded / event.total) * 100);
            statusMessage.textContent = `Uploading firmware... ${percentComplete}% completed.`;
        }
    });
    xhr.onload = () => {
        if (xhr.status >= 200 && xhr.status < 300) {
            statusMessage.textContent = 'Firmware uploaded successfully! The device will reboot.';
            firmwareAction.innerHTML = "Upload";
            sending = false;
        } else {
            statusMessage.textContent = 'Failed to upload firmware. Please try again.';
            firmwareAction.innerHTML = "Upload";
            sending = false;
        }
    };
    xhr.onerror = () => {
        statusMessage.textContent = 'An error occurred. Please check your connection and try again.';
    };
    xhr.send(firmwareSelector.files[0]);
    firmwareAction.innerHTML = "Abort";
    sending = true;
});

const firmwareSelector = document.getElementById("firmwareFile");
const firmwareFileName = document.getElementById("firmwareFileName");
firmwareSelector.addEventListener("change", () => {
    if (firmwareSelector.files.length == 1) {
        firmwareFileName.innerHTML = firmwareSelector.files[0].name;
    }
});

const wifiForm = document.getElementById("wifi-settings");
const ssidInput = document.getElementById("ssid");
const passInput = document.getElementById("pass");
const statusMessageWifi = document.getElementById("statusMessageWifi");
wifiForm.addEventListener('submit', (event) => {
    event.preventDefault();
    ssidInput.value = ssidInput.value.trim();
    passInput.value = passInput.value.trim();
    fetch("/wifi", { method: "POST", body: new FormData(wifiForm)})
        .then(res => statusMessageWifi.textContent = "Successfully send wifi settings. Rebooting...")
        .catch((err) => statusMessageWifi.textContent = "Error, try again");
});


const hostnameForm = document.getElementById("hostname-settings");
const statusMessageHostname = document.getElementById("statusMessageHostname");
hostnameForm.addEventListener('submit', (event) => {
    event.preventDefault();
    fetch("/hostname", { method: "POST", body: new FormData(hostnameForm)})
        .then(res => statusMessageHostname.textContent = "Successfully send new hostname. Rebooting...")
        .catch((err) => statusMessageHostname.textContent = "Error, try again");
});

const pwmForm = document.getElementById("pwm-mapping");
const statusMessagePin = document.getElementById("statusMessagePin");
pwmForm.addEventListener('submit', (event) => {
    event.preventDefault();
    statusMessagePin.textContent = "";
    fetch("/pin_mapping", { method: "POST", body: new FormData(pwmForm)})
        .then(res => statusMessagePin.textContent = "Successfully send pin mapping.")
        .catch((err) => statusMessagePin.textContent = "Error, try again");
});


const statusMessageAction = document.getElementById("statusMessageAction");

function setPinHigh(event, pin) {
    event.preventDefault();
    statusMessagePin.textContent = "";
    const formData = new FormData();
    formData.append("pin", pin);
    fetch("/set_pin_high", {method: "POST", body: formData})
        .then(res => statusMessagePin.textContent = `Successfully set pin ${pin} to high`)
        .catch((err) => statusMessagePin.textContent = `Error, while set pin ${pin} to high. Try again!`);
}

function setPinLow(event, pin) {
    event.preventDefault();
    statusMessagePin.textContent = "";
    const formData = new FormData();
    formData.append("pin", pin);
    fetch("/set_pin_low", {method: "POST", body: formData})
        .then(res => statusMessagePin.textContent = `Successfully set pin ${pin} to low`)
        .catch((err) => statusMessagePin.textContent = `Error, while set pin ${pin} to low. Try again!`);
}

function reboot() {
    statusMessageAction.textContent = "";
    fetch("/reboot", {method: "POST"})
        .then(res => statusMessageAction.textContent = "Rebooting...")
        .catch((err) => statusMessageAction.textContent = "Error while send reboot signal. Try again!");
}

function cleanEasyflash() {
    statusMessageAction.textContent = "";
    fetch("/clean_easyflash", {method: "POST"})
        .then(res => statusMessageAction.textContent = "ALL easyflash variables cleaned")
        .catch((err) => statusMessageAction.textContent = "Error while cleaning easyflash variables. Try again!");
}
