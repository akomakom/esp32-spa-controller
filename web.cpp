#include "web.h"


const char *WEB_RESPONSE_OK = R"({ "success": true })";
const char *WEB_RESPONSE_FAIL = R"({ "success": false })";

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

//holds the current upload
File fsUploadFile;

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

void returnFail(String msg) {
    server.send(500, "text/plain", msg + "\r\n");
}

String getContentType(String filename){
    if(server.hasArg("download")) return "application/octet-stream";
    else if(filename.endsWith(".htm")) return "text/html";
    else if(filename.endsWith(".html")) return "text/html";
    else if(filename.endsWith(".css")) return "text/css";
    else if(filename.endsWith(".js")) return "application/javascript";
    else if(filename.endsWith(".png")) return "image/png";
    else if(filename.endsWith(".gif")) return "image/gif";
    else if(filename.endsWith(".jpg")) return "image/jpeg";
    else if(filename.endsWith(".ico")) return "image/x-icon";
    else if(filename.endsWith(".xml")) return "text/xml";
    else if(filename.endsWith(".pdf")) return "application/x-pdf";
    else if(filename.endsWith(".zip")) return "application/x-zip";
    else if(filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

bool handleFileRead(String path){
    Serial.println("handleFileRead: " + path);
    if(path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
        if(SPIFFS.exists(pathWithGz))
            path += ".gz";
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void handleFileUpload(){
    if(server.uri() != "/edit") return;
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
        String filename = upload.filename;
        if(!filename.startsWith("/")) filename = "/"+filename;
        DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");
        filename = String();
    } else if(upload.status == UPLOAD_FILE_WRITE){
        //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
        if(fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END){
        if(fsUploadFile)
            fsUploadFile.close();
        DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
    }
}

void handleFileDelete(){
    if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
    if(path == "/")
        return server.send(500, "text/plain", "BAD PATH");
    if(!SPIFFS.exists(path))
        return server.send(404, "text/plain", "FileNotFound");
    SPIFFS.remove(path);
    server.send(200, "text/plain", "");
    path = String();
}

void handleFileCreate(){
    if(server.args() == 0)
        return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
    if(path == "/")
        return server.send(500, "text/plain", "BAD PATH");
    if(SPIFFS.exists(path))
        return server.send(500, "text/plain", "FILE EXISTS");
    File file = SPIFFS.open(path, "w");
    if(file)
        file.close();
    else
        return server.send(500, "text/plain", "CREATE FAILED");
    server.send(200, "text/plain", "");
    path = String();
}

void handleFileList() {
    if(!server.hasArg("dir")) {
        returnFail("BAD ARGS");
        return;
    }
    String path = server.arg("dir");
    if(path != "/" && !SPIFFS.exists((char *)path.c_str())) {
        returnFail("BAD PATH");
        return;
    }
    File dir = SPIFFS.open((char *)path.c_str());
    path = String();
    if(!dir.isDirectory()){
        dir.close();
        returnFail("NOT DIR");
        return;
    }
    dir.rewindDirectory();

    String output = "[";
    for (int cnt = 0; true; ++cnt) {
        File entry = dir.openNextFile();
        if (!entry)
            break;

        if (cnt > 0)
            output += ',';

        output += "{\"type\":\"";
        output += (entry.isDirectory()) ? "dir" : "file";
        output += "\",\"name\":\"";
        // Ignore '/' prefix
//        output += entry.name()+1;
        output += entry.name();
        output += "\"";
        output += "}";
        entry.close();
    }
    output += "]";
    server.send(200, "text/json", output);
    dir.close();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
    DBG_OUTPUT_PORT.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        DBG_OUTPUT_PORT.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        DBG_OUTPUT_PORT.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            DBG_OUTPUT_PORT.print("  DIR : ");
            DBG_OUTPUT_PORT.println(file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            DBG_OUTPUT_PORT.print("  FILE: ");
            DBG_OUTPUT_PORT.print(file.name());
            DBG_OUTPUT_PORT.print("  SIZE: ");
            DBG_OUTPUT_PORT.println(file.size());
        }
        file = root.openNextFile();
    }
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

    //load editor
    server.on("/edit", HTTP_GET, [](){
        if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
    });
    //create file
    server.on("/edit", HTTP_PUT, handleFileCreate);
    //delete file
    server.on("/edit", HTTP_DELETE, handleFileDelete);
    //first callback is called after the request has ended with all parsed arguments
    //second callback handles file uploads at that location
    server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);


    server.on("/list", HTTP_GET, handleFileList);

    //called when the url is not defined here
    //use it to load content from SPIFFS
    server.onNotFound([](){
        if(!handleFileRead(server.uri()))
            server.send(404, "text/plain", "FileNotFound");
    });
}




WebServer server(80);
