$studioFile = '.\garrysmod_common\sourcesdk-minimal\public\studio.h'
$lineToFind = '#include "utlsymbol.h"'
$contentToAdd = 'inline int max(int a, int b) { return (a > b) ? a : b; }'

$studioContent = Get-Content $studioFile
$index = $studioContent.IndexOf($lineToFind)
if ($index -ne -1) {
    $before = $studioContent[0..$index]
    $after = $studioContent[($index + 1)..($studioContent.Length - 1)]
    $studioContent = $before + $contentToAdd + $after
}
Set-Content $studioFile -Value $studioContent

$premakeFile = '.\garrysmod_common\premake\generator.lua'
$premakeContent = Get-Content $premakeFile | ForEach-Object {
    if ($_ -match '^\s*linktimeoptimization\s*\(\s*\)') { '--' + $_ } else { $_ }
}
Set-Content $premakeFile -Value $premakeContent
