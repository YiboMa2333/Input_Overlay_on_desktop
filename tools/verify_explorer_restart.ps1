# SPDX-License-Identifier: GPL-2.0-only
# Explicit, opt-in desktop test. May close File Explorer windows.
param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$Output,
    [switch]$RestartExplorer
)
$ErrorActionPreference='Stop'
if(-not $RestartExplorer) {throw 'Actual Explorer restart requires -RestartExplorer and user authorization.'}
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Security.Principal;
public static class ExplorerVerification {
    public delegate bool WindowCallback(IntPtr w, IntPtr p);
    [DllImport("user32.dll")] static extern bool EnumWindows(WindowCallback c,IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr w,out uint p);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr w,StringBuilder s,int n);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string c,string t);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
    [StructLayout(LayoutKind.Sequential)] public struct Rect {public int left,top,right,bottom;}
    [StructLayout(LayoutKind.Sequential)] public struct IconId {public uint size;public IntPtr window;public uint id;public Guid guid;}
    [DllImport("shell32.dll")] static extern int Shell_NotifyIconGetRect(ref IconId id,out Rect rect);
    public static IntPtr Overlay(uint pid) {
        IntPtr result=IntPtr.Zero;
        EnumWindows((w,p)=>{uint process;GetWindowThreadProcessId(w,out process);var name=new StringBuilder(128);GetClassName(w,name,128);
            if(process==pid && name.ToString()=="InputOverlayDesktopWindow") {result=w;return false;} return true;},IntPtr.Zero);
        return result;
    }
    public static uint ShellPid() {uint p;GetWindowThreadProcessId(FindWindow("Shell_TrayWnd",null),out p);return p;}
    public static Guid IconGuid(string profile) {
        string identity=(WindowsIdentity.GetCurrent().User.Value+"|"+System.IO.Path.GetFullPath(profile)).ToLowerInvariant();
        ulong hash=14695981039346656037UL;
        unchecked {foreach(char c in identity) {hash^=c;hash*=1099511628211UL;}}
        return new Guid(unchecked((int)0x77dcab11),unchecked((short)0x8b75),0x4dc1,BitConverter.GetBytes(hash));
    }
    public static bool HasIcon(IntPtr window,Guid guid) {
        var id=new IconId {size=(uint)Marshal.SizeOf(typeof(IconId)),window=window,id=1,guid=guid};Rect rect;
        return Shell_NotifyIconGetRect(ref id,out rect)>=0 && rect.right>rect.left && rect.bottom>rect.top;
    }
}
'@
function Wait-Until([scriptblock]$Condition,[int]$Seconds) {
    $deadline=[DateTime]::UtcNow.AddSeconds($Seconds)
    do {if(& $Condition) {return $true};Start-Sleep -Milliseconds 150} while([DateTime]::UtcNow -lt $deadline)
    return $false
}
$exe=(Resolve-Path -LiteralPath $Executable).Path
$directory=[System.IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$profile=Join-Path $directory ('profile-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $profile | Out-Null
$previousProfile=$env:LOCALAPPDATA
$application=$null
try {
    $env:LOCALAPPDATA=$profile
    $application=Start-Process -FilePath $exe -ArgumentList '--tray' -WindowStyle Hidden -PassThru
    if(-not (Wait-Until { [ExplorerVerification]::Overlay($application.Id) -ne [IntPtr]::Zero } 15)) {throw 'Test overlay did not start'}
    $window=[ExplorerVerification]::Overlay($application.Id)
    $guid=[ExplorerVerification]::IconGuid($profile)
    if(-not (Wait-Until { [ExplorerVerification]::HasIcon($window,$guid) } 15)) {throw 'No initial tray icon'}
    $shellProcessId=[ExplorerVerification]::ShellPid()
    $shellProcess=Get-Process -Id $shellProcessId
    $expectedExplorer=Join-Path $env:WINDIR 'explorer.exe'
    if($shellProcess.ProcessName -ne 'explorer' -or $shellProcess.Path -ne $expectedExplorer) {throw 'Shell PID is not the expected Windows Explorer executable'}
    if([ExplorerVerification]::ShellPid() -ne $shellProcessId) {throw 'Shell changed before the test; aborting'}
    Write-Output "Restarting verified shell PID $shellProcessId; test overlay PID $($application.Id)"
    Stop-Process -Id $shellProcessId -Force
    $shellProcess.WaitForExit(5000) | Out-Null
    if(-not (Wait-Until { [ExplorerVerification]::ShellPid() -ne 0 -and [ExplorerVerification]::ShellPid() -ne $shellProcessId } 10)) {
        Start-Process -FilePath $expectedExplorer -WindowStyle Hidden | Out-Null
        if(-not (Wait-Until { [ExplorerVerification]::ShellPid() -ne 0 -and [ExplorerVerification]::ShellPid() -ne $shellProcessId } 20)) {throw 'Explorer did not return'}
    }
    $newShellProcessId=[ExplorerVerification]::ShellPid()
    $application.Refresh()
    if($application.HasExited) {throw 'Overlay exited when Explorer restarted'}
    if(-not (Wait-Until { [ExplorerVerification]::HasIcon($window,$guid) } 20)) {throw 'Overlay tray icon was not restored'}
    $result=[ordered]@{real_explorer_restart=$true;before_explorer_pid=$shellProcessId;after_explorer_pid=$newShellProcessId;
        overlay_pid=$application.Id;overlay_survived=$true;tray_restored=$true;windows=[Environment]::OSVersion.Version.ToString()}
    $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $directory 'results.json') -Encoding UTF8
    $result | ConvertTo-Json
} finally {
    if([ExplorerVerification]::ShellPid() -eq 0) {Start-Process -FilePath (Join-Path $env:WINDIR 'explorer.exe') -WindowStyle Hidden | Out-Null}
    if($null -ne $application) {
        $w=[ExplorerVerification]::Overlay($application.Id)
        if($w -ne [IntPtr]::Zero) {[ExplorerVerification]::PostMessage($w,0x10,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null}
        if(-not $application.WaitForExit(5000)) {Stop-Process -Id $application.Id -Force}
    }
    $env:LOCALAPPDATA=$previousProfile
}
