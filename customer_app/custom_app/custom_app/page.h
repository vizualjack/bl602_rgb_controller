const char *html_page = "\
<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
    <meta charset=\"UTF-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <title>BL602 Firmware Upload</title>\
    <style>\
        body {\
            font-family: Arial, sans-serif;\
            margin: 2em;\
        }\
        .container {\
            max-width: 500px;\
            margin: 0 auto;\
            padding: 2em;\
            border: 1px solid #ddd;\
            border-radius: 8px;\
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);\
        }\
        h1 {\
            font-size: 1.5em;\
            text-align: center;\
        }\
        input[type=\"file\"] {\
            display: block;\
            margin: 1em 0;\
        }\
        button {\
            display: block;\
            width: 100%;\
            padding: 0.7em;\
            font-size: 1em;\
            color: #fff;\
            background-color: #007bff;\
            border: none;\
            border-radius: 4px;\
            cursor: pointer;\
        }\
        button:hover {\
            background-color: #0056b3;\
        }\
    </style>\
</head>\
<body>\
    <span>v0.0.19</span>\
    <div class=\"container\">\
        <h1>Firmware Upload</h1>\
        <form id=\"uploadForm\" action=\"/ota\" method=\"POST\" enctype=\"multipart/form-data\">\
            <label for=\"firmwareFile\">Select Firmware File:</label>\
            <input type=\"file\" id=\"firmwareFile\" name=\"file\" accept=\".bin.xz\" required>\
            <button type=\"submit\">Upload Firmware</button>\
        </form>\
        <p id=\"statusMessage\" style=\"text-align: center; margin-top: 1em;\"></p>\
    </div>\
\
    <div class=\"container\">\
        <h1>Wifi settings</h1>\
        <form id=\"wifi-settings\" action=\"/wifi\" method=\"POST\" enctype=\"multipart/form-data\">\
            <label for=\"ssid\">SSID:</label>\
            <input type=\"text\" id=\"ssid\" name=\"ssid\" required/>\
            <label for=\"pass\">Password:</label>\
            <input type=\"password\" id=\"pass\" name=\"pass\" required/>\
            <button type=\"submit\">Send wifi settings</button>\
        </form>\
    </div>\
\
    <div class=\"container\">\
        <h1>PWM change</h1>\
        <form id=\"pwm-start\" action=\"/pwm_start\" method=\"POST\" enctype=\"multipart/form-data\">\
            <label for=\"pin\">Pin id (0-3):</label>\
            <input type=\"number\" id=\"pin_id\" name=\"pin_id\" required/>\
            <input type=\"number\" id=\"duty\" name=\"duty\" required/>\
            <button type=\"submit\">Start</button>\
        </form>\
    </div>\
\
    <script>\
        const form = document.getElementById('uploadForm');\
        const statusMessage = document.getElementById('statusMessage');\
\
        form.addEventListener('submit', async (event) => {\
            event.preventDefault(); // Prevent default form submission\
\
            // Show a loading message\
            statusMessage.textContent = 'Uploading firmware... Please wait.';\
\
            const formData = new FormData(form);\
\
            try {\
                const response = await fetch('/ota', {\
                    method: 'POST',\
                    body: firmwareFile.files[0],\
                });\
\
                if (response.ok) {\
                    statusMessage.textContent = 'Firmware uploaded successfully! The device will reboot.';\
                } else {\
                    statusMessage.textContent = 'Failed to upload firmware. Please try again.';\
                }\
            } catch (error) {\
                console.error('Upload error:', error);\
                statusMessage.textContent = 'An error occurred. Please check your connection and try again.';\
            }\
        });\
    </script>\
</body>\
</html>\
";
