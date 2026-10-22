param(
    [ValidatePattern('^v\d+\.\d+\.\d+$')][string]$Version = 'v1.0.1',
    [string]$LoaderPackage
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$output = Join-Path $root "build/release/$Version"
New-Item -ItemType Directory -Path $output -Force | Out-Null

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
function Assert-Win32([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($bytes, 60)
    if ([BitConverter]::ToUInt32($bytes, $pe) -ne 0x4550 -or
        [BitConverter]::ToUInt16($bytes, $pe + 4) -ne 0x14c -or
        [BitConverter]::ToUInt16($bytes, $pe + 24) -ne 0x10b) { throw "Not a Win32 PE: $Path" }
}
$includes = "/I$(Join-Path $root 'RetroInput')"
$resource = Join-Path $output 'ButtonImages.res'
Push-Location (Join-Path $root 'RetroInput')
try { Invoke-Checked 'rc.exe' @('/nologo', "/fo$resource", 'ButtonImages.rc') }
finally { Pop-Location }
$shared = @('dllmain.cpp','Controller.cpp','DualSense.cpp','DualShock4.cpp','Shield.cpp','Backbone.cpp','Vx2.cpp','JoyCon.cpp','Core/ButtonImages.cpp')
$games = @(
    @{ Name='The-Suffering'; Folder='TheSuffering'; Dll='SufferingXInput.dll'; Exe='Suffering.exe'; Define=@(); Extra=@('Games/TheSuffering/Prompts.cpp') },
    @{ Name='The-Suffering-Ties-That-Bind'; Folder='TheSufferingTTB'; Dll='SufferingTTBInput.dll'; Exe='SufferingTTB.exe'; Define=@('/DRETROINPUT_TTB'); Extra=@() }
)
foreach ($game in $games) {
    $directory = Join-Path $output $game.Name
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    $sources = @($shared + @("Games/$($game.Folder)/GameHooks.cpp") + $game.Extra | ForEach-Object { Join-Path "$root/RetroInput" $_ })
    Push-Location $directory
    try {
        $compile = @('/nologo','/std:c++20','/W4','/EHsc','/O2','/MT','/LD','/DNDEBUG',$includes) + $game.Define + $sources + @($resource,"/Fe$($game.Dll)",'/link','user32.lib')
        Invoke-Checked 'cl.exe' $compile
        Assert-Win32 (Join-Path $directory $game.Dll)
    } finally { Pop-Location }
}

$testDirectory = Join-Path $output 'checks'
New-Item -ItemType Directory -Path $testDirectory -Force | Out-Null
Push-Location $testDirectory
try {
    $checks = @('AtlasUpload','TheSufferingWalk','TheSufferingHook','TheSufferingControls','TTBControls','ControllerSelector','DualShock4','Shield','Backbone','Vx2','JoyCon','TTBPrompts','TTBRenderingSupport','TTBHook','PromptImageRouting')
    foreach ($name in $checks) {
        $sources = @(Join-Path $root "tests/${name}Tests.cpp")
        if ($name -eq 'TheSufferingControls') { $sources += Join-Path $root 'RetroInput/Core/ButtonImages.cpp' }
        Invoke-Checked 'cl.exe' (@('/nologo','/std:c++20','/EHsc','/MT',$includes) + $sources + @("/Fe${name}Tests.exe",'/link','user32.lib'))
        Invoke-Checked (Join-Path $testDirectory "${name}Tests.exe") @()
    }
    Invoke-Checked 'cl.exe' @('/nologo','/std:c++20','/EHsc','/MT',$includes,"$root/tests/IconPackTests.cpp","$root/RetroInput/Core/ButtonImages.cpp",$resource,'/FeIconPackTests.exe','/link','user32.lib')
    foreach ($pack in @('auto','ps4','dualshock4','dualsense','xboxseries','nintendo','xbox','playstation','missing')) {
        Invoke-Checked (Join-Path $testDirectory 'IconPackTests.exe') @($pack)
    }
} finally { Pop-Location }

# Reuse the exact licensed loader already shipped and tested with v1.0.0.
$loaderZip = Join-Path $output 'loader-source.zip'
if ($LoaderPackage) { Copy-Item -LiteralPath $LoaderPackage -Destination $loaderZip }
else { Invoke-WebRequest 'https://github.com/Tiger-Fury/RetroInput-Public/releases/download/v1.0.0/RetroInput-The-Suffering-v1.0.0-Win32.zip' -OutFile $loaderZip }
if ((Get-FileHash $loaderZip -Algorithm SHA256).Hash -ne 'e640f69e935edda8169bad49ecf5cd42cda630f1e0b90bd4c6071c1fa0eac31e') { throw 'Loader source package checksum mismatch.' }
$loader = Join-Path $output 'loader'
Expand-Archive $loaderZip $loader -Force
if ((Get-FileHash "$loader/winmm.dll" -Algorithm SHA256).Hash -ne 'ec2f4824eca58dd40f425756a4a7cec77b8e381f21d11d7c846ec4b339b617ab') { throw 'Loader DLL checksum mismatch.' }
$checksums = @()
foreach ($game in $games) {
    $package = Join-Path $output "package-$($game.Name)"
    New-Item -ItemType Directory -Path $package -Force | Out-Null
    Copy-Item (Join-Path "$output/$($game.Name)" $game.Dll) $package
    Copy-Item "$loader/winmm.dll","$loader/Ultimate-ASI-Loader-LICENSE.txt" $package
    [IO.File]::WriteAllText("$package/winmm.ini", "[GlobalSets]`nLoadPlugins=1`nLoadExtraPlugins=$($game.Dll)`nDontLoadFromDllMain=1`nDebugLog=1`n")
    $readme = @"
# RetroInput $Version - $($game.Name.Replace('-', ' '))

Controller input and automatic Xbox, PlayStation and Nintendo button images.
This release fixes rendering corruption when switching controller artwork packs.

## Installation

Close the game. Read these instructions before replacing files. For a new loader setup,
copy $($game.Dll), winmm.dll and winmm.ini beside $($game.Exe).
Preserve existing winmm.dll/winmm.ini files and other game fixes. If Ultimate ASI Loader
is already installed, keep it and add $($game.Dll) to its LoadExtraPlugins setting.
Keep PC keyboard and mouse bindings at their defaults, then launch normally.
An existing RetroInput installation usually only needs the updated game DLL.

## Controls and support

Console-style layouts, automatic controller artwork and stick walking are supported.
In Ties That Bind, tap the north face button to cycle weapons and hold it to pick up.
See the supported-games, supported-controllers and controls pages in docs:
https://github.com/Tiger-Fury/RetroInput-Public/tree/$Version/docs

The individual image packs remain embedded in the DLL. One native texture is reused
when the selected pack changes; no external image files or injector are needed.
Rumble and motion controls are not implemented.

## Uninstall

Close the game and remove its RetroInput DLL. Remove only loader files introduced
by this package; preserve shared loaders and restore any files you backed up.

## Credits

- ThirteenAG: thank you for Ultimate ASI Loader (Win32 9.7.4). Its MIT licence is included.
  https://github.com/ThirteenAG/Ultimate-ASI-Loader
- JulioCacko: thank you for the controller icons (CC0).
  https://juliocacko.itch.io/
"@
    [IO.File]::WriteAllText("$package/RetroInput-README.md", $readme)
    $zip = Join-Path $output "RetroInput-$($game.Name)-$Version-Win32.zip"
    Compress-Archive "$package/*" $zip -Force
    $archive = [IO.Compression.ZipFile]::OpenRead($zip)
    try {
        $expected = @($game.Dll,'winmm.dll','winmm.ini','RetroInput-README.md','Ultimate-ASI-Loader-LICENSE.txt')
        if (@(Compare-Object ($expected | Sort-Object) ($archive.Entries.FullName | Sort-Object)).Count) { throw 'Unexpected package contents.' }
        foreach ($entry in $archive.Entries) {
            $stream = $entry.Open()
            $algorithm = [Security.Cryptography.SHA256]::Create()
            try { $digest = [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-','') }
            finally { $stream.Dispose(); $algorithm.Dispose() }
            if ($digest -ne (Get-FileHash (Join-Path $package $entry.FullName) -Algorithm SHA256).Hash) { throw "ZIP entry mismatch: $($entry.FullName)" }
        }
    } finally { $archive.Dispose() }
    $checksums += "$((Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant())  $([IO.Path]::GetFileName($zip))"
}
[IO.File]::WriteAllLines((Join-Path $output 'SHA256SUMS.txt'), $checksums)
$summary = 'Built both Win32 DLLs; 15 regression suites and nine icon-pack configurations passed. Packages include the verified Ultimate ASI Loader and licence.'
if ($env:GITHUB_STEP_SUMMARY) { Add-Content $env:GITHUB_STEP_SUMMARY $summary }
Write-Output $summary
