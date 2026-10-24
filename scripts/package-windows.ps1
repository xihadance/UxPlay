param(
    [Parameter(Mandatory = $true)]
    [string]$RuntimePrefix,
    [Parameter(Mandatory = $true)]
    [ValidateSet('x64', 'arm64')]
    [string]$Architecture,
    [string]$BinaryPath = 'build/uxplay.exe',
    [string]$OutputDirectory = 'build/artifacts'
)

$ErrorActionPreference = 'Stop'
$repoDirectory = Split-Path -Parent $PSScriptRoot
$runtimeDirectory = (Resolve-Path -LiteralPath $RuntimePrefix).Path
$binary = (Resolve-Path -LiteralPath $BinaryPath).Path
$runtimeDlls = @(Get-ChildItem -LiteralPath "$runtimeDirectory/bin" -Filter '*.dll' -File)
$plugins = @(Get-ChildItem -LiteralPath "$runtimeDirectory/lib/gstreamer-1.0" -Filter '*.dll' -File)
$scanner = "$runtimeDirectory/libexec/gstreamer-1.0/gst-plugin-scanner.exe"
$inspector = "$runtimeDirectory/bin/gst-inspect-1.0.exe"

if (!$runtimeDlls.Count -or !$plugins.Count) {
    throw 'MSYS2 runtime DLLs or GStreamer plugins are missing.'
}
foreach ($requiredFile in @($scanner, $inspector)) {
    if (!(Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required GStreamer tool is missing: $requiredFile"
    }
}

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$packageName = "uxplay-windows-$Architecture"
$packageDirectory = Join-Path $output $packageName
$archive = Join-Path $output "$packageName.zip"
if ((Test-Path -LiteralPath $packageDirectory) -or (Test-Path -LiteralPath $archive)) {
    throw "Package already exists in $output; choose a fresh output directory."
}

New-Item -ItemType Directory -Path $packageDirectory | Out-Null
New-Item -ItemType Directory -Path "$packageDirectory/lib/gstreamer-1.0" -Force | Out-Null
New-Item -ItemType Directory -Path "$packageDirectory/libexec/gstreamer-1.0" -Force | Out-Null
Copy-Item -LiteralPath $binary -Destination "$packageDirectory/uxplay.exe"
Copy-Item -LiteralPath $inspector -Destination $packageDirectory
foreach ($dll in $runtimeDlls) {
    Copy-Item -LiteralPath $dll.FullName -Destination $packageDirectory
}
foreach ($plugin in $plugins) {
    Copy-Item -LiteralPath $plugin.FullName -Destination "$packageDirectory/lib/gstreamer-1.0"
}
Copy-Item -LiteralPath $scanner -Destination "$packageDirectory/libexec/gstreamer-1.0"
foreach ($relativeDirectory in @('lib/gio/modules', 'lib/ossl-modules', 'share/glib-2.0/schemas')) {
    $sourceDirectory = Join-Path $runtimeDirectory $relativeDirectory
    if (Test-Path -LiteralPath $sourceDirectory -PathType Container) {
        $destinationDirectory = Join-Path $packageDirectory $relativeDirectory
        New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
        Get-ChildItem -LiteralPath $sourceDirectory | Copy-Item -Destination $destinationDirectory -Recurse
    }
}
Copy-Item -LiteralPath "$runtimeDirectory/share/licenses" -Destination "$packageDirectory/licenses" -Recurse
New-Item -ItemType Directory -Path "$packageDirectory/licenses/llhttp" -Force | Out-Null
Copy-Item -LiteralPath "$repoDirectory/lib/llhttp/LICENSE-MIT" -Destination "$packageDirectory/licenses/llhttp"
Copy-Item -LiteralPath "$repoDirectory/LICENSE", "$repoDirectory/README.md" -Destination $packageDirectory

# Relocate GStreamer explicitly so the ZIP works without an MSYS2 installation.
$launcher = @(
    '@echo off'
    'setlocal'
    'set "PATH=%~dp0;%PATH%"'
    'set "GST_PLUGIN_PATH_1_0="'
    'set "GST_PLUGIN_SYSTEM_PATH_1_0=%~dp0lib\gstreamer-1.0"'
    'set "GST_PLUGIN_SCANNER_1_0=%~dp0libexec\gstreamer-1.0\gst-plugin-scanner.exe"'
    'set "GIO_EXTRA_MODULES=%~dp0lib\gio\modules"'
    'set "GSETTINGS_SCHEMA_DIR=%~dp0share\glib-2.0\schemas"'
    'set "OPENSSL_MODULES=%~dp0lib\ossl-modules"'
    '"%~dp0uxplay.exe" %*'
    'exit /b %errorlevel%'
) -join "`r`n"
[System.IO.File]::WriteAllText("$packageDirectory/start-uxplay.cmd", $launcher + "`r`n", [System.Text.Encoding]::ASCII)

$instructions = @'
Extract the entire ZIP before running start-uxplay.cmd.
Keep the DLLs, lib, and libexec directories next to uxplay.exe.
Use the x64 package on Intel/AMD Windows 10 or 11, and arm64 on Windows 11 ARM.

From a terminal:
  start-uxplay.cmd -h
  start-uxplay.cmd -n "UxPlay Windows" -fs
  start-uxplay.cmd -rc "C:\path\to\uxplayrc"

This build uses UxPlay's internal mDNS implementation; Bonjour is not required.
Allow UxPlay through Windows Firewall and connect both devices to the same LAN.
The full options and Bluetooth beacon instructions are in README.md.
Dependencies retain their licenses in the licenses directory.
'@
[System.IO.File]::WriteAllText("$packageDirectory/README-WINDOWS.txt", $instructions, [System.Text.Encoding]::UTF8)

Compress-Archive -LiteralPath $packageDirectory -DestinationPath $archive
Write-Output "Created $archive"
