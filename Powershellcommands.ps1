
$URI = "http://192.168.5.29"
# Dir 
Invoke-RestMethod -Uri "$URI/list?dir=/" -Method get
Invoke-RestMethod -Uri "$URI/test2.txt" -Method get
# Delete file
Invoke-RestMethod -Uri "$URI/test2.txt" -Method delete
Invoke-WebRequest -Uri "$URI/test2.txt" -Method delete
curl -XDELETE "$URI/test2.txt"
# Dir /test 
Invoke-RestMethod -Uri "$URI/list?dir=/" -Method get
Invoke-WebRequest -Uri "$URI/list?dir=/" -Method get
Invoke-RestMethod -Uri "$URI/edit?path=/test.txt" -Method delete
# Create dirctory is not supported on the SPIFFS
Invoke-RestMethod -Uri "$URI/edit?path=/test" -Method put
Invoke-RestMethod -Uri "$URI/list?dir=/test" -Method get
# create file
Invoke-RestMethod -Uri "$URI/edit?path=/test.txt" -Method put
# Upload a file 
# create a test file 
"Testing $(get-date) " + [System.Guid]::NewGuid().ToString() > 'test.txt'
# Upload a file
ESPSPIFFSuploadfile "$URI/edit" 'test.txt' '/test2.txt'

ESPSPIFFSuploadfile "$URI/edit" 'web\Mars.jpg' '/Mars2.jpg'
Invoke-WebRequest "$URI/Mars2.jpg" -OutFile 'web\Mars3.jpg'
compare-object (get-content 'web\Mars.jpg') (get-content 'web\Mars3.jpg')
Invoke-RestMethod -Uri "$URI/list?dir=/" -Method get
Invoke-RestMethod -Uri "$URI/edit?path=/Mars2.jpg" -Method delete


Invoke-WebRequest -URI "http://ESP-LittleFS-S3.local/list?dir=/"
# Content           : [{"type":"file","name":"Mars.jpg","size":"236403"},{"type":"file","name":"Settings.txt","size":"140"},{"type":"file","name":"log.txt","size":"1760"},{"type":"file","name":"test2.txt","size":"12"}]
# Invoke-RestMethod will just give the output 
Invoke-RestMethod -URI "http://ESP-LittleFS-S3.local/list?dir=/"

Invoke-WebRequest -URI "http://ESP-LittleFS-S3.local/"
# will return 200 and Index page.
Invoke-WebRequest -URI "http://ESP-LittleFS-S3.local/index.html"
Invoke-WebRequest -Method Post -URI "http://ESP-LittleFS-S3.local/edit?path=/test5.txt"

Invoke-WebRequest -Method delete -URI "http://ESP-LittleFS-S3.local/edit?path=/test5.txt"
Invoke-RestMethod -Method Get -URI "http://ESP-LittleFS-S3.local/list?dir=/"

Invoke-WebRequest -Method put -URI "http://ESP-LittleFS-S3.local/edit?path=/test5.txt"
Invoke-RestMethod -Method Get -URI "http://ESP-LittleFS-S3.local/list?dir=/"

# create dir if the filename does not have a "." dot.
Invoke-WebRequest -Method put -URI "http://ESP-LittleFS-S3.local/edit?path=/test9"
Invoke-RestMethod -Method Get -URI "http://ESP-LittleFS-S3.local/list?dir=/"

Invoke-RestMethod -Method Get -URI "http://ESP-LittleFS-S3.local/diskinfo"

# creates file if the filename does have a "." dot.
Invoke-WebRequest -Method put -URI "http://ESP-LittleFS-S3.local/edit?path=/test9/test.txt"
Invoke-RestMethod -Method Get -URI "http://ESP-LittleFS-S3.local/list?dir=/test9"


Invoke-RestMethod -Method Get -URI "http://ESP-LittleFS-S3.local/list?dir=/"
Invoke-RestMethod -Method Get -URI "http://ESP-LittleFS-S3.local/list?dir=/eeee"


##################################################################################################################################
function ESPSPIFFSuploadfile() {
    param (
        [Parameter(Mandatory = $true)][String] $UploadURL, 
        [Parameter(Mandatory = $true)][String] $File, 
        [Parameter(Mandatory = $false)][String] $Destinaionfilename 
    )
    if ([string]::IsNullOrWhiteSpace($Destinaionfilename)) { $Destinaionfilename = "/$(Split-Path $File -leaf)" }     
    
    $FilePath = Get-Item -Path $File;
    $fileBytes = [System.IO.File]::ReadAllBytes($FilePath);
    $fileEnc = [System.Text.Encoding]::GetEncoding('iso-8859-1').GetString($fileBytes);
    $boundary = [System.Guid]::NewGuid().ToString(); 
    $EOL = "`r`n";

    $bodyLines = ( 
        "--$boundary",
        "Content-Disposition: form-data; name=`"data`"; filename=`"$Destinaionfilename`"",
        "Content-Type: application/octet-stream",
        "",
        $fileEnc,
        "--$boundary", 
        "",
        "$EOL" 
    ) -join $EOL
    Invoke-RestMethod -Uri $UploadURL -Method Post -ContentType "multipart/form-data; boundary=`"$boundary`"" -Body $bodyLines 
}
################################################################################################################################

$URI = "http://ESP-LittleFS-ESP.local"
$URI = "http://ESP-LittleFS-S2.local"
$URI = "http://ESP-LittleFS-S3.local"
$URI = "http://ESP-LittleFS-C3.local"
$URI = "http://ESP-LittleFS-C6.local"
$URI = "http://ESP-LittleFS-12E.local"

# Upload a file
ESPSPIFFSuploadfile "$URI/edit" 'editor.js' '/editor.js'
ESPSPIFFSuploadfile "$URI/edit" 'index.htm' '/index.htm'
ESPSPIFFSuploadfile "$URI/edit" 'editor.css' '/editor.css'
ESPSPIFFSuploadfile "$URI/edit" 'favicon.ico' '/favicon.ico'
ESPSPIFFSuploadfile "$URI/edit" 'Mars.jpg' '/Mars.jpg'

# Douwnload a file
Invoke-WebRequest "$URI/Mars.jpg" -OutFile 'web\Mars3.jpg'

Invoke-WebRequest -Method delete -URI "$URI/edit?path=/editor.js"
Invoke-WebRequest -Method delete -URI "$URI/edit?path=/editor.css"
Invoke-WebRequest -Method delete -URI "$URI/edit?path=/index.htm"
Invoke-WebRequest -Method delete -URI "$URI/edit?path=/favicon.ico"
Invoke-WebRequest -Method delete -URI "$URI/edit?path=/Mars.jpg"

Invoke-WebRequest -Method delete -URI "$URI/edit?path=/test9"



################################################################################################################################
## This upload uses the .net framework 
# not recomended to use
################################################################################################################################
function ESPSPIFFSuploadfile() {
    param (
        [Parameter(Mandatory = $true)][String] $UploadURL, 
        [Parameter(Mandatory = $true)][String] $Filename, 
        [Parameter(Mandatory = $false)][String] $Destinaionfilename 
    )
    if ([string]::IsNullOrWhiteSpace($Destinaionfilename)) { $Destinaionfilename = "/$(Split-Path $File -leaf)" }     
    
    $file = Get-Item $Filename;
    $fileStream = $file.OpenRead()
    $content = [System.Net.Http.MultipartFormDataContent]::new()
    $fileContent = [System.Net.Http.StreamContent]::new($fileStream)
    $fileContent.Headers.ContentDisposition = [System.Net.Http.Headers.ContentDispositionHeaderValue]::new("form-data")
    # Your example had quotes in your literal form-data example so I kept them here
    $fileContent.Headers.ContentDisposition.Name = '"Filedata"'
    $fileContent.Headers.ContentDisposition.FileName = '"{0}"' -f $Destinaionfilename 
    $fileContent.Headers.ContentType = 'application/octet-stream'
    $content.Add($fileContent)
    # Content-Type is set automatically from the FormData to
    # multipart/form-data, Boundary: "..."
    Invoke-RestMethod -Uri $UploadURL -Method Post -Body $content
}
################################################################################################################################
