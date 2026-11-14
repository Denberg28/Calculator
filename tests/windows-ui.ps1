param([Parameter(Mandatory=$true)][string]$Exe)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class Native {
  [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr w, int id);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr w, StringBuilder b, int n);
  [DllImport("user32.dll")] public static extern IntPtr SendMessageTimeout(IntPtr w, uint m, IntPtr a, IntPtr b, uint f, uint t, out IntPtr r);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr w, uint m, IntPtr a, IntPtr b);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr w, out Rect r);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr w, out Rect r);
  [DllImport("user32.dll")] public static extern int MapWindowPoints(IntPtr from, IntPtr to, ref Rect r, uint n);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr w, IntPtr z, int x, int y, int width, int height, uint f);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr w, IntPtr dc, uint f);
  [DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr w);
  [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr w, out uint process);
  [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] private static extern bool AttachThreadInput(uint from, uint to, bool attach);
  [DllImport("user32.dll")] private static extern IntPtr SetActiveWindow(IntPtr w);
  [DllImport("user32.dll")] private static extern bool SetForegroundWindow(IntPtr w);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr GetProp(IntPtr w, string name);
  [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr w);
  [DllImport("user32.dll")] public static extern int GetDlgCtrlID(IntPtr w);
  public static void Activate(IntPtr w) {
    uint process;
    uint target = GetWindowThreadProcessId(w, out process), own = GetCurrentThreadId();
    bool attached = AttachThreadInput(own, target, true);
    try { SetForegroundWindow(w); SetActiveWindow(w); }
    finally { if (attached) AttachThreadInput(own, target, false); }
  }
}
'@
function Send([IntPtr]$Window, [uint32]$Message, [int]$A = 0, [int]$B = 0) {
    $result = [IntPtr]::Zero
    if ([Native]::SendMessageTimeout($Window, $Message, [IntPtr]$A, [IntPtr]$B, 2, 3000, [ref]$result) -eq [IntPtr]::Zero) {
        throw "Window failed to respond: $Message"
    }
}
function Text([int]$Id) {
    $buffer = New-Object Text.StringBuilder 512
    [void][Native]::GetWindowText([Native]::GetDlgItem($script:window, $Id), $buffer, 512)
    return $buffer.ToString()
}
function Click([int]$Index) {
    $button = [Native]::GetDlgItem($script:window, (100 + $Index))
    if ($button -eq [IntPtr]::Zero) { throw "Missing button $Index" }
    Send $button 0xF5
    $rect = New-Object Native+Rect
    [void][Native]::GetWindowRect($button,[ref]$rect)
    Write-Output "Click $Index id $([Native]::GetDlgCtrlID($button)) enabled $([Native]::IsWindowEnabled($button)) rect $($rect.Left),$($rect.Top),$($rect.Right),$($rect.Bottom) command $([Native]::GetProp($script:window,'Calculator.LastCommand')) click $([Native]::GetProp($script:window,'Calculator.LastClick')) token $([Native]::GetProp($script:window,'Calculator.LastToken')) length $([Native]::GetProp($script:window,'Calculator.InputLength')) -> expression '$(Text 200)', result '$(Text 201)', status '$(Text 202)'"
}
function Expect([int]$Id, [string]$Expected) {
    $actual = Text $Id
    if ($actual -ne $Expected) { throw "Control ${Id}: expected '$Expected', got '$actual'" }
}
function Type([string]$Expression) {
    foreach ($character in $Expression.ToCharArray()) {
        if (![Native]::PostMessage($script:window, 0x102, [IntPtr][int]$character, [IntPtr]::Zero)) { throw 'PostMessage failed' }
    }
    # Synchronize with a posted UI barrier by polling observable state (bounded).
    $deadline = [DateTime]::UtcNow.AddSeconds(3)
    do { Start-Sleep -Milliseconds 20 } until ((Text 200) -eq $Expression -or [DateTime]::UtcNow -gt $deadline)
    Expect 200 $Expression
}
function Key([int]$Code) {
    if (![Native]::PostMessage($script:window, 0x100, [IntPtr]$Code, [IntPtr]::Zero)) { throw 'Key failed' }
    Start-Sleep -Milliseconds 100
}
$process = Start-Process -FilePath (Resolve-Path $Exe) -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do { Start-Sleep -Milliseconds 100; $process.Refresh() } until ($process.MainWindowHandle -ne 0 -or $process.HasExited -or [DateTime]::UtcNow -gt $deadline)
    if ($process.HasExited -or $process.MainWindowHandle -eq 0) { throw 'Calculator did not open' }
    $script:window = $process.MainWindowHandle
    # BM_CLICK needs an active parent; CI's PowerShell owns a different input thread.
    [Native]::Activate($window)
    Write-Output "Calculator window $window; foreground $([Native]::GetForegroundWindow())"
    Expect 201 '0'
    Type '1'; Key 0x1B; Expect 200 ''
    # All mouse buttons; arithmetic, precedence, decimal and negative results.
    Click 4; Click 19; Click 10; Click 18; Expect 201 '13'
    Click 0; Click 1; Click 13; Click 19; Click 14; Click 2; Click 11; Click 8; Click 18; Expect 201 '20'
    Click 0; Click 5; Click 15; Click 12; Click 18; Expect 201 '7'
    Click 0; Click 8; Click 15; Click 6; Click 18; Expect 201 '-5'
    Click 0; Click 4; Click 7; Click 16; Click 18; Expect 202 'Cannot divide by zero'
    Click 3; Click 13; Click 18; Expect 201 '3.5'; Expect 202 ''
    Click 0; Click 16; Click 17; Click 9; Click 11; Click 8; Click 18; Expect 201 '2'
    # Keyboard events travel through the application's message loop.
    Key 0x1B; Expect 200 ''; Expect 201 '0'
    Type '2+3*4'; Key 0x0D; Expect 201 '14'
    Key 0x0D; Expect 201 '14'
    Key 0x2E; Type '12'; Key 0x08; Expect 200 '1'
    Key 0x1B; Type '(2+3'; Key 0x0D; Expect 202 'Close the parentheses'
    Key 0x1B; Type '7/2'; Key 0x0D; Expect 201 '3.5'
    # The application constrains small window sizes and keeps every key in the client area.
    $dpi = [Native]::GetDpiForWindow($window)
    foreach ($size in @(@(320,480), @(360,540), @(720,900))) {
        [void][Native]::SetWindowPos($window, [IntPtr]::Zero, 0, 0, [int]($size[0]*$dpi/96), [int]($size[1]*$dpi/96), 6)
        Send $window 0
        $client = New-Object Native+Rect
        [void][Native]::GetClientRect($window, [ref]$client)
        foreach ($id in 100..119) {
            $rect = New-Object Native+Rect
            [void][Native]::GetWindowRect([Native]::GetDlgItem($window,$id), [ref]$rect)
            [void][Native]::MapWindowPoints([IntPtr]::Zero,$window,[ref]$rect,2)
            if ($rect.Left -lt 0 -or $rect.Top -lt 0 -or $rect.Right -gt $client.Right -or $rect.Bottom -gt $client.Bottom -or $rect.Right -le $rect.Left -or $rect.Bottom -le $rect.Top) { throw "Button $id clipped at $size" }
        }
        Expect 201 '3.5'
    }
    [void][Native]::SetWindowPos($window, [IntPtr]::Zero, 0, 0, [int](400*$dpi/96), [int](620*$dpi/96), 6)
    Start-Sleep -Milliseconds 150
    $bounds = New-Object Native+Rect
    [void][Native]::GetWindowRect($window, [ref]$bounds)
    $bitmap = New-Object Drawing.Bitmap ($bounds.Right-$bounds.Left), ($bounds.Bottom-$bounds.Top)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    $dc = $graphics.GetHdc()
    try { if (![Native]::PrintWindow($window,$dc,2)) { throw 'Screenshot capture failed' } }
    finally { $graphics.ReleaseHdc($dc) }
    $bitmap.Save((Join-Path (Get-Location) 'ui-smoke.png'),[Drawing.Imaging.ImageFormat]::Png)
    $graphics.Dispose(); $bitmap.Dispose()
    [void][Native]::PostMessage($window,0x10,[IntPtr]::Zero,[IntPtr]::Zero)
    if (!$process.WaitForExit(3000) -or $process.ExitCode -ne 0) { throw 'Unclean shutdown' }
    # Repeat launch/close to catch lifecycle regressions.
    foreach ($cycle in 1..3) {
        $process = Start-Process -FilePath (Resolve-Path $Exe) -PassThru
        if (!$process.WaitForInputIdle(5000)) { throw 'Restart failed' }
        $process.Refresh()
        if ($process.MainWindowHandle -eq 0) { throw 'Restart window missing' }
        [void]$process.CloseMainWindow()
        if (!$process.WaitForExit(3000) -or $process.ExitCode -ne 0) { throw 'Restart shutdown failed' }
    }
    Write-Output 'PASS: Windows launch, mouse, keyboard, arithmetic, errors, recovery, resizing, screenshot and four lifecycle cycles'
} finally {
    if (!$process.HasExited) { $process.Kill(); $process.WaitForExit() }
}
