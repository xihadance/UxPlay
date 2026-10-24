param(
    [Parameter(Mandatory = $true)]
    [string]$PackageDirectory
)

$ErrorActionPreference = 'Stop'
$package = (Resolve-Path -LiteralPath $PackageDirectory).Path
$savedPath = $env:PATH
$savedPluginPath = $env:GST_PLUGIN_PATH_1_0
$savedSystemPluginPath = $env:GST_PLUGIN_SYSTEM_PATH_1_0
$savedScanner = $env:GST_PLUGIN_SCANNER_1_0
$savedRegistry = $env:GST_REGISTRY_1_0

try {
    # Exclude MSYS2 from PATH to detect missing DLLs in the portable package.
    $env:PATH = "$package;$env:SystemRoot\System32;$env:SystemRoot"
    $env:GST_PLUGIN_PATH_1_0 = ''
    $env:GST_PLUGIN_SYSTEM_PATH_1_0 = "$package/lib/gstreamer-1.0"
    $env:GST_PLUGIN_SCANNER_1_0 = "$package/libexec/gstreamer-1.0/gst-plugin-scanner.exe"
    $env:GST_REGISTRY_1_0 = Join-Path $env:TEMP "uxplay-registry-$PID.bin"

    & "$package/uxplay.exe" -v
    if ($LASTEXITCODE -ne 0) { throw 'The packaged UxPlay executable could not start.' }

    foreach ($plugin in @('h264parse', 'avdec_h264', 'avdec_aac', 'avdec_alac', 'd3d11videosink', 'wasapisink')) {
        & "$package/gst-inspect-1.0.exe" $plugin | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "The packaged GStreamer plugin could not load: $plugin" }
    }
    Write-Output 'Portable executable and required GStreamer plugins verified.'
} finally {
    $env:PATH = $savedPath
    $env:GST_PLUGIN_PATH_1_0 = $savedPluginPath
    $env:GST_PLUGIN_SYSTEM_PATH_1_0 = $savedSystemPluginPath
    $env:GST_PLUGIN_SCANNER_1_0 = $savedScanner
    $env:GST_REGISTRY_1_0 = $savedRegistry
}
