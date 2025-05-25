$TargetLanguage = $Args[0]
$today = Get-Date

# Check Holiday
# https://nationaltoday.com/category/world/
# Next Up Easter (4/20)

if ($today.Month -eq 4 -and $today.Day -eq 1) {
# Holiday Text (And/Or Action)
$text1 = "Happy April Fools Day"    
Start-Process "https://coin.medicatusb.com/"
} else {
# Normal Text (Updated every once in a while.)
$text1 = "If you or a loved one has been diagnosed with a broken computer, you are in the right place."
} 

$Uri1 = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1"
$Response = Invoke-RestMethod -Uri $Uri1 -Method Get
$Translation1 = $Response[0].SyncRoot | foreach { $_[0] }

write-host $Translation1
#write-host "https://discord.gg/medicat"    
