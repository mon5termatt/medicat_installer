$TargetLanguage = $Args[0]
$today = Get-Date

if ($today.Month -eq 4 -and $today.Day -eq 1) {
$text1 = "Happy April Fools Day"    
$Uri1 = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1"
$Response = Invoke-RestMethod -Uri $Uri1 -Method Get
$Translation1 = $Response[0].SyncRoot | foreach { $_[0] }

# The extra April Fools Joke.
Start-Process "https://coin.medicatusb.com/"

} else {

$text1 = "Welcome Flipper nerds!"
$Uri1 = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1"
$Response = Invoke-RestMethod -Uri $Uri1 -Method Get
$Translation1 = $Response[0].SyncRoot | foreach { $_[0] }
} 

write-host $Translation1
#write-host "https://discord.gg/medicat"    
