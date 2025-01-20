const form = document.getElementById('uploadForm');
const statusMessage = document.getElementById('statusMessage');
var timeoutTimer;
form.addEventListener('submit', (event) => {
    event.preventDefault();
    statusMessage.textContent = 'Uploading firmware... Please wait.';
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/ota');
    xhr.upload.addEventListener('progress', (event) => {
        if (event.lengthComputable) {
            const percentComplete = Math.round((event.loaded / event.total) * 100);
            statusMessage.textContent = `Uploading firmware... ${percentComplete}% completed.`;
            if(percentComplete >= 100) {
                timeoutTimer = setTimeout(() => {
                    xhr.abort();
                    statusMessage.textContent = `Timeout, please try again.`;
                }, 5000);
            }
        }
    });
    xhr.onload = () => {
        if (xhr.status >= 200 && xhr.status < 300) {
            statusMessage.textContent = 'Firmware uploaded successfully! The device will reboot.';
            clearTimeout(timeoutTimer);
        } else {
            statusMessage.textContent = 'Failed to upload firmware. Please try again.';
        }
    };
    xhr.onerror = () => {
        statusMessage.textContent = 'An error occurred. Please check your connection and try again.';
    };
    xhr.send(firmwareSelector.files[0]);
});

const firmwareSelector = document.getElementById("firmwareFile");
const firmwareFileName = document.getElementById("firmwareFileName");
firmwareSelector.addEventListener("change", () => {
    if (firmwareSelector.files.length == 1) {
        firmwareFileName.innerHTML = firmwareSelector.files[0].name;
    }
});
