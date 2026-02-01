# Requires elevated (Run as Administrator) PowerShell session

# --- Step 1: Download the MSYS2 installer ---
$installerUrl = "github.com"
$installPath = "C:\msys64"
#
$tempInstaller = "$env:TEMP\msys2-installer.exe"
$msysDir = "C:\msys64"
$executablePath = Join-Path -Path $msysDir -ChildPath "msys2.exe"
$toolchainPackage = "mingw-w64-x86_64-gcc" # This package includes gcc, g++, etc.

Write-Host "Testing if MSYS2 is already installed"

if (Test-Path -Path $executablePath) {
    Write-Host "MSYS2 is installed at: $msysDir"
} else {
    Write-Host "MSYS2 is not found at the default location. Starting Install"
    # Step 1 down load the exe
	Write-Host "Downloading MSYS2 installer from $installerUrl..."
	try {
	    (New-Object System.Net.WebClient).DownloadFile($installerUrl, $tempInstaller)
	    Write-Host "Download complete."
	} catch {
	    Write-Error "Failed to download the installer. Check network connection and URL."
	    exit 1
	}
	# --- Step 2: Run the installer silently ---

	Write-Host "Installing MSYS2 to $installPath silently..."
	 The --accept-messages --unattended flags enable silent installation
	try {
	    Start-Process -FilePath $tempInstaller -ArgumentList "--accept-messages", "--unattended" -Wait
	$    Write-Host "MSYS2 installation complete."
	} catch {
	    Write-Error "MSYS2 installation failed."
	    exit 1
	}
	# Clean up the installer executable
	if (Test-Path $tempInstaller) {
	    Remove-Item $tempInstaller
	}
}
# --- Step 3: Update the core system and install MingGW toolchain ---
# Note: MSYS2 requires updates to be run within its bash environment
Write-Host "Updating MSYS2 core packages and installing MingGW toolchain..."

# Define the path to the MSYS2 bash executable
$msysBash = "$installPath\usr\bin\bash.exe"

# 1. Initial core update
Write-Host "Running initial pacman -Syu update..."
& $msysBash -lc 'pacman --noconfirm -Syu'

# 2. Second update (sometimes required to ensure all packages are current)
Write-Host "Running second pacman -Syu update..."
& $msysBash -lc 'pacman --noconfirm -Syu'

# 3. Install the UCRT64 GCC toolchain package
Write-Host "Installing the MINGW64 GCC toolchain..."
$mingw64_pkg = "mingw-w64-x86_64-gcc"
& $msysBash -lc "pacman -S --noconfirm --needed base-devel mingw-w64-x86_64-toolchain"
& $msysBash -lc "pacman --noconfirm -S $mingw64_pkg --needed"

Write-Host "A final packman update update compilier and tools pacman -Syu update..."
& $msysBash -lc 'pacman --noconfirm -Syu'

Write-Host "MSYS2 MINGW64 environment is ready. You can now use the 'MSYS2 MINGW64' shortcut in the Start menu."
