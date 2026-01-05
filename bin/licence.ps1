$TargetLanguage = $Args[0]


$text1 = "IF YOU PAID FOR THIS SOFTWARE, THEN YOU WERE SCAMMED!"
$text2 = "MediCat USB is released completely free of charge."
$text3 = "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the 'Software'), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the software is furnished to do so, subject to the following conditions:"
$text4 = "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the software."
$text5 = "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."
$text6 = "USE THIS SOFTWARE AT YOUR OWN RISK!"



$Uri1 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1”
$Response = Invoke-RestMethod -Uri $Uri1 -Method Get
$Translation1 = $Response[0].SyncRoot | foreach { $_[0] }

$Uri2 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text2”
$Response = Invoke-RestMethod -Uri $Uri2 -Method Get
$Translation2 = $Response[0].SyncRoot | foreach { $_[0] }

$Uri3 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text3”
$Response = Invoke-RestMethod -Uri $Uri3 -Method Get
$Translation3 = $Response[0].SyncRoot | foreach { $_[0] }

$Uri4 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text4”
$Response = Invoke-RestMethod -Uri $Uri4 -Method Get
$Translation4 = $Response[0].SyncRoot | foreach { $_[0] }

$Uri5 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text5”
$Response = Invoke-RestMethod -Uri $Uri5 -Method Get
$Translation5 = $Response[0].SyncRoot | foreach { $_[0] }

$Uri6 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text6”
$Response = Invoke-RestMethod -Uri $Uri6 -Method Get
$Translation6 = $Response[0].SyncRoot | foreach { $_[0] }


write-host "MIT License"
write-host ""
write-host "#####################################################"
write-host $Translation1
write-host "#####################################################"
write-host ""
write-host $Translation2
write-host ""
write-host $Translation3
write-host ""
write-host $Translation4
write-host ""
write-host $Translation5
write-host ""
write-host $Translation6
write-host ""
write-host "Copyright (c) 2025 MediCat USB"
