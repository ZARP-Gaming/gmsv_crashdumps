$file = '.\garrysmod_common\sourcesdk-minimal\public\studio.h'
$lineToFind = '#include "utlsymbol.h"'
$contentToAdd = 'inline int max(int a, int b) { return (a > b) ? a : b; }'

$fileContent = Get-Content $file
$index = $fileContent.IndexOf($lineToFind)

if ($index -ne -1) {
    $before = $fileContent[0..$index]
    $after = $fileContent[($index + 1)..($fileContent.Length - 1)]
    $fileContent = $before + $contentToAdd + $after
    Set-Content $file -Value $fileContent
}
