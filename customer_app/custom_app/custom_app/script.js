const form = document.getElementById('uploadForm');
const firmwareAction = document.getElementById('firmwareAction');
const statusMessage = document.getElementById('statusMessage');
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
        } else {
            statusMessage.textContent = 'Failed to upload firmware. Please try again.';
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
