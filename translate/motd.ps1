$TargetLanguage = $Args[0]

$text1 = "Thanks for over 13 thousand discord members"

$Uri1 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1”
$Response = Invoke-RestMethod -Uri $Uri1 -Method Get
$Translation1 = $Response[0].SyncRoot | foreach { $_[0] }

write-host $Translation1
