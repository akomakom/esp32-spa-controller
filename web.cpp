#include "web.h"


const char *WEB_RESPONSE_OK = R"({ "success": true })";
const char *WEB_RESPONSE_FAIL = R"({ "success": false })";

// ensure there are no )" sequences in quoted text:
const char *WEBPAGE_CONTROLS = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Control</title>
    <style>
        .error {
            color: red;
        }

        .spacontrol {
            box-shadow: 1px 1px 4px 1px #8dcbe6;
            background: linear-gradient(to bottom, #33bdef 5%, #019ad2 100%);
            background-color: #33bdef;
            border-radius: 9px;
            border: 3px solid #057fd0;
            display: inline-block;
            cursor: pointer;
            color: #ffffff;
            font-family: Arial;
            font-size: 28px;
            font-weight: bold;
            padding: 15px 51px;
            text-decoration: none;
            text-shadow: 0px -1px 0px #5b6178;
        }

        .spacontrol:hover {
            background: linear-gradient(to bottom, #019ad2 5%, #33bdef 100%);
            background-color: #019ad2;
        }

        .spacontrol:active {
            position: relative;
            top: 1px;
        }

    </style>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.0/jquery.min.js"></script>
    <script>
        $(document).ready(function() {
            let toggleClickHandler = function() {
                $.ajax({
                    type: "POST",
                    url: '/toggle',
                    data: { control:  $(this).attr('name') },
                    success: function() {
                        $(this).removeClass('error');
                        updateStatus();
                    },
                    // SUCCESS WINDS UP HERE???
                    error: function () {
                        $(this).addClass('error')
                    },
                    dataType: 'json'
                });
            }
            let updateStatus = function () {
                $.getJSON( "status", function( data ) {
                    if ($('button.spacontrol').length == 0) {
                        //first load, create controls
                        $.each(data.controls, function(name) {

                            let button = $('<button type="button" class="spacontrol"></button>');
                            button
                                .attr('value', data.controls[name])
                                .attr('name', name)
                                .text(name)
                                .on('click', toggleClickHandler);
                            $('#spacontrols').append(button);
                        });
                    }

                    $('button.spacontrol').each(function () {
                        let button = $(this);
                        let name = button.attr('name');
                        button.attr('value', data.controls[name]);
                        button.text(name + "(" + data.controls[name] + ") "); //space for C R syntax
                    })
                });
            };

            updateStatus();
            setInterval(updateStatus, 5000);
        });

    </script>
</head>
<body>
    <div id="spacontrols">
    </div>
</body>
</html>
)";


/*
 * Login page
 */

const char *WEBPAGE_LOGIN =
        "<form name='loginForm'>"
        "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
        "<td colspan=2>"
        "<center><font size=4><b>ESP32 Login Page AKOM</b></font></center>"
        "<br>"
        "</td>"
        "<br>"
        "<br>"
        "</tr>"
        "<tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
        "<td>Password:</td>"
        "<td><input type='Password' size=25 name='pwd'><br></td>"
        "<br>"
        "<br>"
        "</tr>"
        "<tr>"
        "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
        "</table>"
        "</form>"
        "<script>"
        "function check(form)"
        "{"
        "if(form.userid.value=='admin' && form.pwd.value=='admin')"
        "{"
        "window.open('/serverIndex')"
        "}"
        "else"
        "{"
        " alert('Error Password or Username')/*displays error message*/"
        "}"
        "}"
        "</script>";

/*
 * Server Index Page
 */

const char *WEBPAGE_UPDATE =
        "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
        "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
        "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
        "</form>"
        "<div id='prg'>progress: 0%</div>"
        "<script>"
        "$('form').submit(function(e){"
        "e.preventDefault();"
        "var form = $('#upload_form')[0];"
        "var data = new FormData(form);"
        " $.ajax({"
        "url: '/update',"
        "type: 'POST',"
        "data: data,"
        "contentType: false,"
        "processData:false,"
        "xhr: function() {"
        "var xhr = new window.XMLHttpRequest();"
        "xhr.upload.addEventListener('progress', function(evt) {"
        "if (evt.lengthComputable) {"
        "var per = evt.loaded / evt.total;"
        "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
        "}"
        "}, false);"
        "return xhr;"
        "},"
        "success:function(d, s) {"
        "console.log('success!')"
        "},"
        "error: function (a, b, c) {"
        "}"
        "});"
        "});"
        "</script>";


void sendJSONResponse(const char *content) {
    sendJSONResponse(content, 200);
}

void sendJSONResponse(const char *content, int code) {
    server.sendHeader("Connection", "close");
    server.send(code, "application/json", content);
}

void sendHTMLResponse(const char *content) {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", content);
}

void setupServerDefaultActions() {
    server.on("/serverIndex", HTTP_GET, []() {
        sendHTMLResponse(WEBPAGE_UPDATE);
    });
    server.on("/update", HTTP_GET, []() {
        sendHTMLResponse(WEBPAGE_LOGIN);
    });
    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            /* flashing firmware to ESP*/
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { //true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
}


WebServer server(80);
