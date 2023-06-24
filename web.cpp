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

        .scheduler {
            border: 1px solid grey;
        }

        #duration-dialog-form {
            display: none;
        }

    </style>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.0/jquery.min.js"></script>
    <link rel="stylesheet" href="https://ajax.googleapis.com/ajax/libs/jqueryui/1.13.2/themes/smoothness/jquery-ui.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jqueryui/1.13.2/jquery-ui.min.js"></script>
    <script>

        // https://stackoverflow.com/a/71184881/1735931
        function secToTime(seconds, separator) {
            return [
                parseInt(seconds / 60 / 60),
                parseInt(seconds / 60 % 60),
                parseInt(seconds % 60)
            ].join(separator ? separator : ':').replace(/\b(\d)\b/g, "0$1").replace(/^00\:/,'')
        }

        $(document).ready(function() {

             //TODO: get all this from server
            let durationOptions = [0,1,2,3,4,5,10,15,20,30,45,60,120,240,480]

            // saved state for override duration edit dialog:
            let durationChoice = 20;
            let controlName = '';
            let controlValue = 0;

            $.each(durationOptions, function(index, duration) {
                let option = $('<button type="button"/>');
                option.text(secToTime(duration))
                    .attr('value', duration)
                    .on('click', function() {
                        durationChoice = duration;
                        $.ajax({
                            type: "POST",
                            url: '/override',
                            data: {
                                control:  controlName,
                                start: 0,
                                duration: durationChoice * 60,
                                value: controlValue,
                            },
                            success: function() {
                                $(this).removeClass('error');
                                updateStatus();
                            },
                            error: function () {
                                $(this).addClass('error')
                            },
                            dataType: 'json'
                        });
                    });
                $('#duration-options').append(option);
            });


            let durationChoiceHandler = function(event) {
                controlName = $(this).attr('name');
                controlValue = $(this).attr('value');

                event.stopPropagation();
                let durationChoice = 20;
                dialog = $( "#duration-dialog-form" ).dialog({
//                  autoOpen: false,
                  height: 400,
                  width: 350,
                  modal: true,
                  buttons: {
                    'Close': function() {
                        $(this).dialog('close');
                    }
                  },

                  close: function() {

                  }
                });
            }

            let toggleClickHandler = function() {
                $.ajax({
                    type: "POST",
                    url: '/toggle',
                    data: { control:  $(this).attr('name') },
                    success: function() {
                        $(this).removeClass('error');
                        updateStatus();
                    },
                    error: function () {
                        $(this).addClass('error')
                    },
                    dataType: 'json'
                });
            }
            let updateStatus = function () {
                $.getJSON( "status", function( data ) {
                    if ($('.spacontrol').length == 0) {
                        //first load, create controls
                        $.each(data.controls, function(name) {
                            let control = $(`
                                <div class="spacontrol">
                                   <button type="button" class="control">
                                       <span class="control-name"></span>
                                       <span class="control-state"></span>
                                   </button>
                                   <button type="button" class="scheduler">
                                       <span class="scheduler-on">&#9201;</span>
                                       <span class="scheduler-ort"></span>
                                   </button>
                                 </div> `);
                            control.find('.control')
                                .on('click', toggleClickHandler)
                                .attr('value', data.controls[name]['value'])
                                .attr('name', name);
                            control.find('.scheduler')
                                .on('click', durationChoiceHandler)
                                .attr('value', data.controls[name]['value'])
                                .attr('name', name);
                            $('#spacontrols').append(control);
                        });
                    }

                    $('.spacontrol').each(function () {
                        let control = $(this);
                        let controlButton = control.find('.control');

                        let name = controlButton.attr('name');
                        let controlState = data.controls[name];

                        control.find('.control').attr('value', controlState['value']);
                        control.find('.scheduler').attr('value', controlState['value']);
                        let valNames = ["OFF", "ON"];
                        if (controlState['type'] ==  'off-low-high') {
                            valNames = ["OFF", "LOW", "HIGH"];
                        }
                        control.find('.control-name').text(name);
                        control.find('.control-state').text(valNames[controlState['value']]);
                        control.find('.scheduler-ort').text(secToTime(controlState['ORT']));
                        control.find('.scheduler').toggle(controlState['ORT'] > 0);
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
    <div id="duration-dialog-form" title="Select Duration">
        Select how long this control should remain in this mode (Hours:Minutes)
        <div id="duration-options">
        </div>
      </form>
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
